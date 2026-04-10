#include "CodeGenVisitor.h"

// CodeGenVisitor implementation: Generates LLVM IR from AST nodes.
// Each visit method emits the appropriate LLVM instructions for the node type.
// Complex operations like type promotion and control flow are handled here.

CodeGenVisitor::CodeGenVisitor(CodeGenerator& cg)
    : m_cg(cg), m_lastValue(nullptr), m_lastVarType(VarType::INT)
{}

// Generates code for the entire program by visiting each top-level node.
void CodeGenVisitor::generate(const std::vector<std::unique_ptr<ASTNode>>& program)
{
    for (const auto& node : program) node->accept(*this);
}

llvm::Value* CodeGenVisitor::getValue() { return m_lastValue; }

// Converts our VarType enum to LLVM's Type system.
llvm::Type* CodeGenVisitor::varTypeToLLVM(VarType vt)
{
    auto& builder = m_cg.getBuilder();
    auto& context = m_cg.getContext();
    switch (vt) {
        case VarType::INT:    return builder.getInt32Ty();
        case VarType::FLOAT:  return builder.getDoubleTy();
        case VarType::BOOL:   return builder.getInt1Ty();
        case VarType::STRING: return llvm::PointerType::getUnqual(context);
        default:
            throw std::runtime_error("Unsupported element type");
    }
}

// Converts LLVM Type back to our VarType for tracking.
VarType CodeGenVisitor::llvmTypeToVarType(llvm::Type* type)
{
    if (type->isDoubleTy()) return VarType::FLOAT;
    if (type->isIntegerTy(32)) return VarType::INT;
    if (type->isIntegerTy(1)) return VarType::BOOL;
    if (type->isPointerTy()) return VarType::STRING;
    throw std::runtime_error("Unsupported LLVM type");
}


void CodeGenVisitor::visit(const NumberNode& node)
{
    m_lastValue   = llvm::ConstantInt::get(m_cg.getBuilder().getInt32Ty(), node.value);
    m_lastVarType = VarType::INT;
}
void CodeGenVisitor::visit(const FloatNode& node)
{
    m_lastValue   = llvm::ConstantFP::get(m_cg.getContext(), llvm::APFloat(node.value));
    m_lastVarType = VarType::FLOAT;
}
void CodeGenVisitor::visit(const BoolNode& node)
{
    m_lastValue   = llvm::ConstantInt::get(m_cg.getBuilder().getInt1Ty(), node.value);
    m_lastVarType = VarType::BOOL;
}
void CodeGenVisitor::visit(const StringNode& node)
{
    m_lastValue   = m_cg.getBuilder().CreateGlobalString(node.value, ".str");
    m_lastVarType = VarType::STRING;
}
void CodeGenVisitor::visit(const VariableNode& node)
{
    m_lastValue = m_cg.loadVariable(node.name);
    m_lastVarType = llvmTypeToVarType(m_lastValue->getType());
}

void CodeGenVisitor::visit(const CastNode& node)
{
    node.expression->accept(*this);
    auto& builder = m_cg.getBuilder();
    auto& mod     = m_cg.getModule();
    auto& ctx     = m_cg.getContext();

    // Helper to declare a C standard function if not already in the module
    auto declareFunc = [&](const std::string& name, llvm::FunctionType* ft) -> llvm::Function* {
        llvm::Function* f = mod.getFunction(name);
        if (!f) f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, mod);
        return f;
    };

    if (node.targetType == "float") {
        if (m_lastVarType == VarType::INT) {
            m_lastValue   = builder.CreateSIToFP(m_lastValue, builder.getDoubleTy(), "int2float");
            m_lastVarType = VarType::FLOAT;

        } else if (m_lastVarType == VarType::STRING) {
            // double strtod(const char* str, char** endptr)
            llvm::Function* strtodFunc = declareFunc("strtod",
                llvm::FunctionType::get(
                    builder.getDoubleTy(),
                    {builder.getPtrTy(), builder.getPtrTy()},
                    false));

            // char* fprintf / stderr — we'll use printf to stderr via fputs for simplicity
            // Allocate an endptr on the stack
            llvm::Function* currentFunc = builder.GetInsertBlock()->getParent();
            llvm::AllocaInst* endPtr = m_cg.createEntryBlockAlloca(
                currentFunc, "endptr", builder.getPtrTy());

            llvm::Value* strVal = m_lastValue;
            llvm::Value* result = builder.CreateCall(strtodFunc, {strVal, endPtr}, "str2float");

            // Check: if *endptr == '\0' and endptr != str, conversion succeeded
            llvm::Value* endPtrVal = builder.CreateLoad(builder.getPtrTy(), endPtr, "endptrval");

            // *endptr should be '\0' (null terminator) for full conversion
            llvm::Value* endChar = builder.CreateLoad(builder.getInt8Ty(), endPtrVal, "endchar");
            llvm::Value* isNull  = builder.CreateICmpEQ(
                endChar,
                llvm::ConstantInt::get(builder.getInt8Ty(), 0),
                "isnull");

            // Also check endptr != str (to catch empty string "")
            llvm::Value* notSame = builder.CreateICmpNE(endPtrVal, strVal, "notempty");
            llvm::Value* valid   = builder.CreateAnd(isNull, notSame, "valid");

            // Branch: valid → continue, invalid → print error and exit
            llvm::BasicBlock* okBlock  = llvm::BasicBlock::Create(ctx, "cast_ok",      currentFunc);
            llvm::BasicBlock* errBlock = llvm::BasicBlock::Create(ctx, "cast_err",     currentFunc);
            llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(ctx, "cast_merge", currentFunc);

            builder.CreateCondBr(valid, okBlock, errBlock);

            // Error block: print message and call exit(1)
            builder.SetInsertPoint(errBlock);
            llvm::Function* putsFunc = declareFunc("puts",
                llvm::FunctionType::get(
                    builder.getInt32Ty(), {builder.getPtrTy()}, false));
            llvm::Function* exitFunc = declareFunc("exit",
                llvm::FunctionType::get(
                    builder.getVoidTy(), {builder.getInt32Ty()}, false));

            llvm::Value* errMsg = builder.CreateGlobalString(
                "Error: invalid cast from string to float", "cast_err_msg");
            builder.CreateCall(putsFunc, {errMsg});
            builder.CreateCall(exitFunc, {llvm::ConstantInt::get(builder.getInt32Ty(), 1)});
            builder.CreateUnreachable();

            // Ok block
            builder.SetInsertPoint(okBlock);
            builder.CreateBr(mergeBlock);

            // Merge block: result is valid
            builder.SetInsertPoint(mergeBlock);
            m_lastValue   = result;
            m_lastVarType = VarType::FLOAT;
        }

    } else if (node.targetType == "int") {
        if (m_lastVarType == VarType::FLOAT) {
            m_lastValue   = builder.CreateFPToSI(m_lastValue, builder.getInt32Ty(), "float2int");
            m_lastVarType = VarType::INT;

        } else if (m_lastVarType == VarType::STRING) {
            // long strtol(const char* str, char** endptr, int base)
            llvm::Function* strtolFunc = declareFunc("strtol",
                llvm::FunctionType::get(
                    builder.getInt64Ty(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty()},
                    false));

            llvm::Function* currentFunc = builder.GetInsertBlock()->getParent();
            llvm::AllocaInst* endPtr = m_cg.createEntryBlockAlloca(
                currentFunc, "endptr", builder.getPtrTy());

            llvm::Value* strVal = m_lastValue;
            llvm::Value* result64 = builder.CreateCall(
                strtolFunc,
                {strVal, endPtr, llvm::ConstantInt::get(builder.getInt32Ty(), 10)},
                "str2long");

            // Truncate long → int32
            llvm::Value* result = builder.CreateTrunc(result64, builder.getInt32Ty(), "long2int");

            // Validate: *endptr == '\0' and endptr != str
            llvm::Value* endPtrVal = builder.CreateLoad(builder.getPtrTy(), endPtr, "endptrval");
            llvm::Value* endChar   = builder.CreateLoad(builder.getInt8Ty(), endPtrVal, "endchar");
            llvm::Value* isNull    = builder.CreateICmpEQ(
                endChar,
                llvm::ConstantInt::get(builder.getInt8Ty(), 0),
                "isnull");
            llvm::Value* notSame   = builder.CreateICmpNE(endPtrVal, strVal, "notempty");
            llvm::Value* valid     = builder.CreateAnd(isNull, notSame, "valid");

            llvm::BasicBlock* okBlock    = llvm::BasicBlock::Create(ctx, "cast_ok",    currentFunc);
            llvm::BasicBlock* errBlock   = llvm::BasicBlock::Create(ctx, "cast_err",   currentFunc);
            llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(ctx, "cast_merge", currentFunc);

            builder.CreateCondBr(valid, okBlock, errBlock);

            // Error block
            builder.SetInsertPoint(errBlock);
            llvm::Function* putsFunc = declareFunc("puts",
                llvm::FunctionType::get(
                    builder.getInt32Ty(), {builder.getPtrTy()}, false));
            llvm::Function* exitFunc = declareFunc("exit",
                llvm::FunctionType::get(
                    builder.getVoidTy(), {builder.getInt32Ty()}, false));

            llvm::Value* errMsg = builder.CreateGlobalString(
                "Error: invalid cast from string to int", "cast_err_msg");
            builder.CreateCall(putsFunc, {errMsg});
            builder.CreateCall(exitFunc, {llvm::ConstantInt::get(builder.getInt32Ty(), 1)});
            builder.CreateUnreachable();

            // Ok block
            builder.SetInsertPoint(okBlock);
            builder.CreateBr(mergeBlock);

            // Merge
            builder.SetInsertPoint(mergeBlock);
            m_lastValue   = result;
            m_lastVarType = VarType::INT;
        }
    }
}

void CodeGenVisitor::visit(const BinaryOpNode& node)
{
    auto& builder = m_cg.getBuilder();
    node.left->accept(*this);  llvm::Value* L = m_lastValue; VarType LType = m_lastVarType;
    node.right->accept(*this); llvm::Value* R = m_lastValue; VarType RType = m_lastVarType;

    // Handle mixed int/float operations by promoting ints to floats.
    // This ensures operations like 5 + 3.2 work correctly.
    bool isFloat = (LType == VarType::FLOAT || RType == VarType::FLOAT);
    if (isFloat) {
        if (LType == VarType::INT)
            L = builder.CreateSIToFP(L, builder.getDoubleTy(), "int2float");
        if (RType == VarType::INT)
            R = builder.CreateSIToFP(R, builder.getDoubleTy(), "int2float");
    }

    switch (node.op) {
        case TokenType::PLUS:
            if (isFloat) m_lastValue = builder.CreateFAdd(L, R, "fadd");
            else m_lastValue = builder.CreateAdd(L, R, "add");
            m_lastVarType = isFloat ? VarType::FLOAT : VarType::INT;
            break;
        case TokenType::MINUS:
            if (isFloat) m_lastValue = builder.CreateFSub(L, R, "fsub");
            else m_lastValue = builder.CreateSub(L, R, "sub");
            m_lastVarType = isFloat ? VarType::FLOAT : VarType::INT;
            break;
        case TokenType::STAR:
            if (isFloat) m_lastValue = builder.CreateFMul(L, R, "fmul");
            else m_lastValue = builder.CreateMul(L, R, "mul");
            m_lastVarType = isFloat ? VarType::FLOAT : VarType::INT;
            break;
        case TokenType::SLASH:
            if (isFloat) m_lastValue = builder.CreateFDiv(L, R, "fdiv");
            else m_lastValue = builder.CreateSDiv(L, R, "div");
            m_lastVarType = isFloat ? VarType::FLOAT : VarType::INT;
            break;
        case TokenType::LESS:
            if (isFloat) m_lastValue = builder.CreateFCmpOLT(L, R, "lt");
            else m_lastValue = builder.CreateICmpSLT(L, R, "lt");
            m_lastVarType = VarType::BOOL;
            break;
        case TokenType::GREATER:
            if (isFloat) m_lastValue = builder.CreateFCmpOGT(L, R, "gt");
            else m_lastValue = builder.CreateICmpSGT(L, R, "gt");
            m_lastVarType = VarType::BOOL;
            break;
        case TokenType::LESS_EQUAL:
            if (isFloat) m_lastValue = builder.CreateFCmpOLE(L, R, "le");
            else m_lastValue = builder.CreateICmpSLE(L, R, "le");
            m_lastVarType = VarType::BOOL;
            break;
        case TokenType::GREATER_EQUAL:
            if (isFloat) m_lastValue = builder.CreateFCmpOGE(L, R, "ge");
            else m_lastValue = builder.CreateICmpSGE(L, R, "ge");
            m_lastVarType = VarType::BOOL;
            break;
        case TokenType::DOUBLE_EQUAL:
            if (isFloat) m_lastValue = builder.CreateFCmpUEQ(L, R, "eq");
            else m_lastValue = builder.CreateICmpEQ(L, R, "eq");
            m_lastVarType = VarType::BOOL;
            break;
        case TokenType::NOT_EQUAL:
            if (isFloat) m_lastValue = builder.CreateFCmpUNE(L, R, "ne");
            else m_lastValue = builder.CreateICmpNE(L, R, "ne");
            m_lastVarType = VarType::BOOL;
            break;
        case TokenType::LOGICAL_AND:
            m_lastValue = builder.CreateAnd(toBool(L), toBool(R), "and");
            m_lastVarType = VarType::BOOL;
            break;
        case TokenType::LOGICAL_OR:
            m_lastValue = builder.CreateOr(toBool(L), toBool(R), "or");
            m_lastVarType = VarType::BOOL;
            break;
        default: throw std::runtime_error("Unsupported binary operator in codegen");
    }
}

void CodeGenVisitor::visit(const UnaryOpNode& node)
{
    auto& builder = m_cg.getBuilder();
    node.operand->accept(*this);
    switch (node.op) {
        case TokenType::MINUS:
            if (m_lastVarType == VarType::FLOAT)
                m_lastValue = builder.CreateFNeg(m_lastValue, "fneg");
            else
                m_lastValue = builder.CreateNeg(m_lastValue, "neg");
            break;
        case TokenType::NOT:
            if (m_lastVarType == VarType::FLOAT)
                throw std::runtime_error("NOT operator does not support float");
            m_lastValue = builder.CreateNot(m_lastValue, "not");
            break;
        default: throw std::runtime_error("Unsupported unary operator in codegen");
    }
}


void CodeGenVisitor::visit(const LetNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* func = builder.GetInsertBlock()->getParent();

    if (node.hasTypeAnnotation && node.isArray) {
        VarType elemVarType = VarType::INT;
        if      (node.annotatedType == "int")    elemVarType = VarType::INT;
        else if (node.annotatedType == "float")  elemVarType = VarType::FLOAT;
        else if (node.annotatedType == "bool")   elemVarType = VarType::BOOL;
        else if (node.annotatedType == "string") elemVarType = VarType::STRING;

        llvm::Type* elemLLVM = varTypeToLLVM(elemVarType);

        int size = node.arraySize;
        if (size == -1) {
            auto* lit = dynamic_cast<const ArrayLiteralNode*>(node.value.get());
            size = static_cast<int>(lit->elements.size());
        }

        llvm::AllocaInst* alloc = m_cg.createArrayAlloca(func, node.variableName, elemLLVM, size);
        m_cg.declareVariable(node.variableName, alloc);
        m_cg.setArrayElementType(node.variableName, elemLLVM);

        if (node.value) {
            auto* lit = dynamic_cast<const ArrayLiteralNode*>(node.value.get());
            for (int i = 0; i < size; i++) {
                lit->elements[i]->accept(*this);
                llvm::Value* elem = m_lastValue;
                if (elemLLVM->isDoubleTy() && m_lastVarType == VarType::INT) {
                    elem = builder.CreateSIToFP(elem, builder.getDoubleTy(), "int2float");
                }
                llvm::Value* idx = llvm::ConstantInt::get(builder.getInt32Ty(), i);
                llvm::Value* ptr = m_cg.getArrayElementPtr(alloc, idx, elemLLVM);
                builder.CreateStore(elem, ptr);
            }
        }
        return;
    }

    // Scalar
    node.value->accept(*this);
    llvm::Value*      val    = m_lastValue;
    llvm::Type*       allocTy = val->getType();
    if (node.hasTypeAnnotation) {
        VarType annotatedType = stringToVarType(node.annotatedType);
        allocTy = varTypeToLLVM(annotatedType);
        if (annotatedType == VarType::FLOAT && m_lastVarType == VarType::INT) {
            val = builder.CreateSIToFP(val, builder.getDoubleTy(), "int2float");
            m_lastVarType = VarType::FLOAT;
        }
    }
    llvm::AllocaInst* alloc  = m_cg.createEntryBlockAlloca(func, node.variableName, allocTy);
    builder.CreateStore(val, alloc);
    m_cg.declareVariable(node.variableName, alloc);
}


void CodeGenVisitor::visit(const ArrayLiteralNode& node)
{
    for (const auto& elem : node.elements) elem->accept(*this);
}

void CodeGenVisitor::visit(const ArrayAccessNode& node)
{
    auto& builder = m_cg.getBuilder();

    llvm::AllocaInst* alloc = m_cg.lookupVariable(node.name);
    if (!alloc) throw std::runtime_error("Unknown array: " + node.name);

    llvm::Type* elemTy = m_cg.getArrayElementType(node.name);
    if (!elemTy) throw std::runtime_error("No element-type for array: " + node.name);

    node.index->accept(*this);
    llvm::Value* idx = m_lastValue;

    
    llvm::Type* allocatedTy = alloc->getAllocatedType();
    llvm::Value* ptr;

    if (allocatedTy->isArrayTy()) {
        ptr = m_cg.getArrayElementPtr(alloc, idx, elemTy);
    } else {
        // Array parameter: the alloca holds a pointer; load the pointer first
        llvm::Value* base = builder.CreateLoad(allocatedTy, alloc, node.name + "_ptr");
        ptr = builder.CreateInBoundsGEP(elemTy, base, idx, "elemptr");
    }

    m_lastValue = builder.CreateLoad(elemTy, ptr, node.name + "_elem");
    m_lastVarType = llvmTypeToVarType(elemTy);
}

void CodeGenVisitor::visit(const ArrayAssignNode& node)
{
    auto& builder = m_cg.getBuilder();

    llvm::AllocaInst* alloc = m_cg.lookupVariable(node.name);
    if (!alloc) throw std::runtime_error("Unknown array: " + node.name);

    llvm::Type* elemTy = m_cg.getArrayElementType(node.name);
    if (!elemTy) throw std::runtime_error("No element-type for array: " + node.name);

    node.index->accept(*this);
    llvm::Value* idx = m_lastValue;

    node.value->accept(*this);
    llvm::Value* val = m_lastValue;
    if (elemTy->isDoubleTy() && val->getType()->isIntegerTy(32)) {
        val = builder.CreateSIToFP(val, builder.getDoubleTy(), "int2float");
    } else if (elemTy->isIntegerTy(32) && val->getType()->isDoubleTy()) {
        val = builder.CreateFPToSI(val, builder.getInt32Ty(), "float2int");
    }

    llvm::Type* allocatedTy = alloc->getAllocatedType();
    llvm::Value* ptr;

    if (allocatedTy->isArrayTy()) {
        ptr = m_cg.getArrayElementPtr(alloc, idx, elemTy);
    } else {
        llvm::Value* base = builder.CreateLoad(allocatedTy, alloc, node.name + "_ptr");
        ptr = builder.CreateInBoundsGEP(elemTy, base, idx, "elemptr");
    }

    builder.CreateStore(val, ptr);
}


void CodeGenVisitor::visit(const AssignmentNode& node)
{
    auto& builder = m_cg.getBuilder();
    node.expression->accept(*this);
    llvm::AllocaInst* var = m_cg.lookupVariable(node.variableName);
    if (!var) throw std::runtime_error("Unknown variable: " + node.variableName);

    llvm::Type* targetType = var->getAllocatedType();
    llvm::Value* value = m_lastValue;
    if (targetType->isDoubleTy() && value->getType()->isIntegerTy(32)) {
        value = builder.CreateSIToFP(value, builder.getDoubleTy(), "int2float");
    } else if (targetType->isIntegerTy(32) && value->getType()->isDoubleTy()) {
        value = builder.CreateFPToSI(value, builder.getInt32Ty(), "float2int");
    }
    builder.CreateStore(value, var);
}

void CodeGenVisitor::visit(const ReturnNode& node)
{
    auto& builder = m_cg.getBuilder();
    node.expression->accept(*this);

    llvm::Type* retType = builder.GetInsertBlock()->getParent()->getReturnType();
    llvm::Value* value = m_lastValue;

    if (retType->isDoubleTy() && value->getType()->isIntegerTy(32))
        value = builder.CreateSIToFP(value, builder.getDoubleTy(), "int2float");
    else if (retType->isIntegerTy(32) && value->getType()->isDoubleTy())
        value = builder.CreateFPToSI(value, builder.getInt32Ty(), "float2int");

    builder.CreateRet(value);
}

void CodeGenVisitor::visit(const BlockNode& node)
{
    m_cg.pushScope();
    for (const auto& stmt : node.statements) stmt->accept(*this);
    m_cg.popScope();
}


void CodeGenVisitor::visit(const IfNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* func = builder.GetInsertBlock()->getParent();

    node.condition->accept(*this);
    llvm::Value* cond = toBool(m_lastValue);

    // Create basic blocks for then, else, and merge points.
    // LLVM requires explicit control flow with branches.
    llvm::BasicBlock* thenBB  = llvm::BasicBlock::Create(builder.getContext(), "then",   func);
    llvm::BasicBlock* elseBB  = llvm::BasicBlock::Create(builder.getContext(), "else",   func);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(builder.getContext(), "ifcont", func);

    builder.CreateCondBr(cond, thenBB, elseBB);
    builder.SetInsertPoint(thenBB);
    node.thenBranch->accept(*this);
    if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(mergeBB);
    builder.SetInsertPoint(elseBB);
    if (node.elseBranch) node.elseBranch->accept(*this);
    if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(mergeBB);
    builder.SetInsertPoint(mergeBB);
}

void CodeGenVisitor::visit(const WhileNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* func = builder.GetInsertBlock()->getParent();

    // While loops need a condition block, body block, and exit block.
    // Unconditional branch to condition, then conditional branch to body or exit.
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(builder.getContext(), "whilecond", func);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(builder.getContext(), "whilebody", func);
    llvm::BasicBlock* endBB  = llvm::BasicBlock::Create(builder.getContext(), "whileend",  func);

    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);
    node.condition->accept(*this);
    builder.CreateCondBr(toBool(m_lastValue), bodyBB, endBB);
    builder.SetInsertPoint(bodyBB);
    node.body->accept(*this);
    if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(condBB);
    builder.SetInsertPoint(endBB);
}


void CodeGenVisitor::visit(const ForNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* func = builder.GetInsertBlock()->getParent();

    m_cg.pushScope();

    llvm::AllocaInst* iterAlloc = m_cg.createEntryBlockAlloca(func, node.iteratorName, builder.getInt32Ty());
    m_cg.declareVariable(node.iteratorName, iterAlloc);

    if (node.isRange()) {
        node.iterable->accept(*this);
        builder.CreateStore(m_lastValue, iterAlloc);

        llvm::BasicBlock* condBB = llvm::BasicBlock::Create(builder.getContext(), "forcond", func);
        llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(builder.getContext(), "forbody", func);
        llvm::BasicBlock* endBB  = llvm::BasicBlock::Create(builder.getContext(), "forend",  func);

        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);

        llvm::Value* currentValue = builder.CreateLoad(iterAlloc->getAllocatedType(), iterAlloc, node.iteratorName + "_val");
        node.endExpr->accept(*this);
        llvm::Value* endValue = m_lastValue;
        llvm::Value* cond = builder.CreateICmpSLT(currentValue, endValue, "forcmp");
        builder.CreateCondBr(cond, bodyBB, endBB);

        builder.SetInsertPoint(bodyBB);
        node.body->accept(*this);
        if (!builder.GetInsertBlock()->getTerminator()) {
            llvm::Value* nextValue = builder.CreateAdd(currentValue,
                                                 llvm::ConstantInt::get(builder.getInt32Ty(), 1),
                                                 "nexttmp");
            builder.CreateStore(nextValue, iterAlloc);
            builder.CreateBr(condBB);
        }
        builder.SetInsertPoint(endBB);
    } else {
        // Iterate over array values
        llvm::AllocaInst* arrayAlloc = nullptr;
        llvm::Type* arrayElemTy = nullptr;

        if (auto* varNode = dynamic_cast<const VariableNode*>(node.iterable.get())) {
            arrayAlloc = m_cg.lookupVariable(varNode->name);
            if (arrayAlloc)
                arrayElemTy = m_cg.getArrayElementType(varNode->name);
        }

        if (!arrayAlloc || !arrayElemTy)
            throw std::runtime_error("Unsupported array iterable in for loop");

        llvm::Value* startPtr = nullptr;
        llvm::Value* lengthValue = nullptr;
        llvm::Type* allocatedTy = arrayAlloc->getAllocatedType();

        if (allocatedTy->isArrayTy()) {
            llvm::Value* zero = llvm::ConstantInt::get(builder.getInt32Ty(), 0);
            startPtr = builder.CreateInBoundsGEP(allocatedTy, arrayAlloc, {zero, zero}, "arraystart");
            lengthValue = llvm::ConstantInt::get(builder.getInt32Ty(),
                                                allocatedTy->getArrayNumElements());
        } else if (allocatedTy->isPointerTy()) {
            (void)arrayAlloc;
            throw std::runtime_error("For loops over parameter arrays are not supported");
        } else {
            throw std::runtime_error("Unsupported array iterable in for loop");
        }

        llvm::AllocaInst* idxAlloc = m_cg.createEntryBlockAlloca(func, node.iteratorName + "_idx", builder.getInt32Ty());
        builder.CreateStore(llvm::ConstantInt::get(builder.getInt32Ty(), 0), idxAlloc);

        // Declare element variable using current array element type.
        llvm::AllocaInst* elemAlloc = m_cg.createEntryBlockAlloca(func, node.iteratorName, arrayElemTy);
        m_cg.declareVariable(node.iteratorName, elemAlloc);

        llvm::BasicBlock* condBB = llvm::BasicBlock::Create(builder.getContext(), "forcond", func);
        llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(builder.getContext(), "forbody", func);
        llvm::BasicBlock* endBB  = llvm::BasicBlock::Create(builder.getContext(), "forend",  func);

        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);

        llvm::Value* currentIndex = builder.CreateLoad(idxAlloc->getAllocatedType(), idxAlloc, node.iteratorName + "_idx_val");
        llvm::Value* cmp = builder.CreateICmpSLT(currentIndex, lengthValue, "forcmp");
        builder.CreateCondBr(cmp, bodyBB, endBB);

        builder.SetInsertPoint(bodyBB);
        llvm::Value* elementPtr = builder.CreateInBoundsGEP(arrayElemTy, startPtr, currentIndex, "elementptr");
        llvm::Value* element = builder.CreateLoad(arrayElemTy, elementPtr, node.iteratorName + "_elem");
        builder.CreateStore(element, elemAlloc);

        node.body->accept(*this);

        if (!builder.GetInsertBlock()->getTerminator()) {
            llvm::Value* nextIdx = builder.CreateAdd(currentIndex,
                                                    llvm::ConstantInt::get(builder.getInt32Ty(), 1),
                                                    "nextidx");
            builder.CreateStore(nextIdx, idxAlloc);
            builder.CreateBr(condBB);
        }

        builder.SetInsertPoint(endBB);
    }

    m_cg.popScope();
}


void CodeGenVisitor::visit(const FunctionNode& node)
{
    auto& builder = m_cg.getBuilder();
    auto& module  = m_cg.getModule();
    auto& context = m_cg.getContext();

    // Return type
    llvm::Type* retType = nullptr;
    if      (node.returnType == "void")   retType = builder.getInt32Ty();
    else if (node.returnType == "int")    retType = builder.getInt32Ty();
    else if (node.returnType == "float")  retType = builder.getDoubleTy();
    else if (node.returnType == "bool")   retType = builder.getInt1Ty();
    else if (node.returnType == "string") retType = llvm::PointerType::getUnqual(context);
    else throw std::runtime_error("Unsupported return type: " + node.returnType);

    // Parameter types
    std::vector<llvm::Type*> argTypes;
    for (const auto& p : node.parameters) {
        if (p.isArray) {
            llvm::Type* elemTy = varTypeToLLVM(stringToVarType(p.type));
            argTypes.push_back(llvm::PointerType::getUnqual(context));
            (void)elemTy;
        } else {
            if      (p.type == "int")    argTypes.push_back(builder.getInt32Ty());
            else if (p.type == "float")  argTypes.push_back(builder.getDoubleTy());
            else if (p.type == "bool")   argTypes.push_back(builder.getInt1Ty());
            else if (p.type == "string") argTypes.push_back(llvm::PointerType::getUnqual(context));
            else throw std::runtime_error("Unsupported parameter type: " + p.type);
        }
    }

    llvm::FunctionType* funcType = llvm::FunctionType::get(retType, argTypes, false);
    llvm::Function* function = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, node.name, &module);

    unsigned idx = 0;
    for (auto& arg : function->args()) arg.setName(node.parameters[idx++].name);

    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entryBB);
    m_cg.pushScope();

    idx = 0;
    for (auto& arg : function->args()) {
        const Parameter& p = node.parameters[idx++];

        if (p.isArray) {
            // Alloca for a pointer slot (ptr type)
            llvm::AllocaInst* alloc = builder.CreateAlloca(
                llvm::PointerType::getUnqual(context), nullptr, arg.getName());
            builder.CreateStore(&arg, alloc);
            m_cg.declareVariable(arg.getName().str(), alloc);

            // Register the element type so ArrayAccessNode / ArrayAssignNode
            // can emit the right GEP inside this function
            llvm::Type* elemTy = varTypeToLLVM(stringToVarType(p.type));
            m_cg.setArrayElementType(arg.getName().str(), elemTy);
        } else {
            llvm::AllocaInst* alloc = builder.CreateAlloca(
                arg.getType(), nullptr, arg.getName());
            builder.CreateStore(&arg, alloc);
            m_cg.declareVariable(arg.getName().str(), alloc);
        }
    }

    if (node.body) node.body->accept(*this);

    if (!builder.GetInsertBlock()->getTerminator()) {
        if (retType->isIntegerTy())
            builder.CreateRet(llvm::ConstantInt::get(retType, 0));
        else
            builder.CreateRetVoid();
    }

    m_cg.popScope();
}


void CodeGenVisitor::visit(const CallNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* function = m_cg.getModule().getFunction(node.functionName);
    if (!function)
        throw std::runtime_error("Unknown function: " + node.functionName);

    std::vector<llvm::Value*> args;
    unsigned paramIdx = 0;

    for (auto& argNode : node.arguments) {
        llvm::Type* expectedTy = function->getFunctionType()->getParamType(paramIdx++);

        // If the expected parameter is a pointer type and the argument is a
        // VariableNode whose alloca holds a [N x T] array, we decay it to ptr.
        if (expectedTy->isPointerTy()) {
            if (auto* varNode = dynamic_cast<const VariableNode*>(argNode.get())) {
                llvm::AllocaInst* alloc = m_cg.lookupVariable(varNode->name);
                if (alloc && alloc->getAllocatedType()->isArrayTy()) {
                    // Decay: GEP to first element  →  ptr
                    llvm::Value* zero = llvm::ConstantInt::get(builder.getInt32Ty(), 0);
                    llvm::Value* ptr  = builder.CreateInBoundsGEP(
                        alloc->getAllocatedType(), alloc, {zero, zero}, "arraydecay");
                    args.push_back(ptr);
                    continue;
                }
                // If it's a pointer-slot alloca (already a param ptr), load the ptr
                if (alloc && alloc->getAllocatedType()->isPointerTy()) {
                    llvm::Value* ptr = builder.CreateLoad(
                        alloc->getAllocatedType(), alloc, varNode->name + "_ptr");
                    args.push_back(ptr);
                    continue;
                }
            }
        }

        // Default: evaluate normally
        argNode->accept(*this);
        if (expectedTy->isDoubleTy() && m_lastValue->getType()->isIntegerTy(32)) {
            m_lastValue = builder.CreateSIToFP(m_lastValue, builder.getDoubleTy(), "int2float");
        }
        args.push_back(m_lastValue);
    }

    m_lastValue = builder.CreateCall(function, args, "calltmp");
    m_lastVarType = llvmTypeToVarType(function->getReturnType());
}


void CodeGenVisitor::visit(const PrintNode& node)
{
    auto& builder = m_cg.getBuilder();
    auto& context = m_cg.getContext();
    auto& module  = m_cg.getModule();

    llvm::Function* printfFunc = module.getFunction("printf");
    if (!printfFunc) {
        llvm::FunctionType* pft = llvm::FunctionType::get(
            builder.getInt32Ty(),
            { llvm::PointerType::getUnqual(context) },
            true);
        printfFunc = llvm::Function::Create(
            pft, llvm::Function::ExternalLinkage, "printf", &module);
    }

    if (node.args.empty()) {
        std::string fmt = node.formatString + (node.newline ? "\n" : "");
        builder.CreateCall(printfFunc, { builder.CreateGlobalString(fmt, ".fmt") });
        return;
    }

    std::vector<llvm::Value*> argVals;
    std::vector<VarType>      argTypes;

    for (auto& argNode : node.args) {
        argNode->accept(*this);
        llvm::Value* v = m_lastValue;
        if (v->getType()->isIntegerTy(1))
            v = builder.CreateZExt(v, builder.getInt32Ty());
        argVals.push_back(v);
        if (v->getType()->isPointerTy()) {
            argTypes.push_back(VarType::STRING);
        } else if (v->getType()->isDoubleTy()) {
            argTypes.push_back(VarType::FLOAT);
        } else {
            argTypes.push_back(VarType::INT);
        }
    }

    std::string fmtStr = convertFormatString(node.formatString, argTypes, node.newline);
    std::vector<llvm::Value*> callArgs = { builder.CreateGlobalString(fmtStr, ".fmt") };
    callArgs.insert(callArgs.end(), argVals.begin(), argVals.end());
    builder.CreateCall(printfFunc, callArgs);
}

void CodeGenVisitor::visit(const InputNode& node) {
    auto& ctx     = m_cg.getContext();
    auto& builder = m_cg.getBuilder();
    auto& mod     = m_cg.getModule();

    // Declare fgets if not already present: char* fgets(char*, int, FILE*)
    llvm::Function* fgetsFunc = mod.getFunction("fgets");
    if (!fgetsFunc) {
        llvm::Type* charPtrTy = builder.getPtrTy();
        llvm::FunctionType* fgetsTy = llvm::FunctionType::get(
            charPtrTy,
            {charPtrTy, builder.getInt32Ty(), charPtrTy},
            false);
        fgetsFunc = llvm::Function::Create(
            fgetsTy, llvm::Function::ExternalLinkage, "fgets", mod);
    }

    // Allocate a 256-byte buffer on the stack
    llvm::Function* currentFunc = builder.GetInsertBlock()->getParent();
    llvm::ArrayType* bufTy = llvm::ArrayType::get(builder.getInt8Ty(), 256);
    llvm::AllocaInst* buf = m_cg.createEntryBlockAlloca(currentFunc, "input_buf", bufTy);

    // Get pointer to the start of the buffer (GEP {0,0})
    llvm::Value* zero = llvm::ConstantInt::get(builder.getInt32Ty(), 0);
    llvm::Value* bufPtr = builder.CreateInBoundsGEP(bufTy, buf, {zero, zero}, "bufptr");

    llvm::GlobalVariable* stdinGlobal = mod.getGlobalVariable("stdin");
    if (!stdinGlobal) {
        stdinGlobal = new llvm::GlobalVariable(
            mod,
            builder.getPtrTy(),
            false,
            llvm::GlobalValue::ExternalLinkage,
            nullptr,
            "stdin");
    }
    llvm::Value* stdinVal = builder.CreateLoad(builder.getPtrTy(), stdinGlobal, "stdin");

    // Call fgets(bufPtr, 256, stdin)
    llvm::Value* size256 = llvm::ConstantInt::get(builder.getInt32Ty(), 256);
    builder.CreateCall(fgetsFunc, {bufPtr, size256, stdinVal});

    // Strip the trailing newline: find '\n' and replace with '\0'
    // Simplest: call strcspn then null-terminate
    llvm::Function* strcspnFunc = mod.getFunction("strcspn");
    if (!strcspnFunc) {
        llvm::Type* charPtrTy = builder.getPtrTy();
        llvm::FunctionType* ft = llvm::FunctionType::get(
            builder.getInt64Ty(), {charPtrTy, charPtrTy}, false);
        strcspnFunc = llvm::Function::Create(
            ft, llvm::Function::ExternalLinkage, "strcspn", mod);
    }

    llvm::Value* newlineStr = builder.CreateGlobalString("\n", "nl");
    llvm::Value* nlPos = builder.CreateCall(strcspnFunc, {bufPtr, newlineStr}, "nlpos");
    llvm::Value* nlIdx = builder.CreateIntCast(nlPos, builder.getInt32Ty(), false);
    llvm::Value* nlPtr = builder.CreateInBoundsGEP(
        builder.getInt8Ty(), bufPtr, nlIdx, "nlptr");
    builder.CreateStore(llvm::ConstantInt::get(builder.getInt8Ty(), 0), nlPtr);

    m_lastValue   = bufPtr;
    m_lastVarType = VarType::STRING;
}

llvm::Value* CodeGenVisitor::toBool(llvm::Value* val)
{
    auto& builder = m_cg.getBuilder();
    if (val->getType()->isIntegerTy(1)) return val;
    if (val->getType()->isDoubleTy())
        return builder.CreateFCmpUNE(val, llvm::ConstantFP::get(builder.getDoubleTy(), 0.0), "tobool");
    return builder.CreateICmpNE(val, llvm::ConstantInt::get(val->getType(), 0), "tobool");
}

std::string CodeGenVisitor::convertFormatString(const std::string&          input,
                                                  const std::vector<VarType>& argTypes,
                                                  bool                        newline)
{
    std::string result;
    size_t typeIndex = 0;
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '{' && i + 1 < input.size() && input[i+1] == '}') {
            if (typeIndex >= argTypes.size())
                throw std::runtime_error("Too few arguments for format string");
            switch (argTypes[typeIndex]) {
                case VarType::INT:
                case VarType::BOOL:   result += "%d"; break;
                case VarType::FLOAT:  result += "%f"; break;
                case VarType::STRING: result += "%s"; break;
                default: throw std::runtime_error("Unsupported type in printf");
            }
            typeIndex++; i++;
        } else {
            result += input[i];
        }
    }
    if (newline) result += "\n";
    return result;
}


VarType CodeGenVisitor::stringToVarType(const std::string& s)
{
    if (s == "int")    return VarType::INT;
    if (s == "float")  return VarType::FLOAT;
    if (s == "string") return VarType::STRING;
    if (s == "bool")   return VarType::BOOL;
    if (s == "void")   return VarType::VOID;
    throw std::runtime_error("Unknown type: " + s);
}
