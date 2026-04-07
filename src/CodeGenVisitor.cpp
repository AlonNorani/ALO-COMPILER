#include "CodeGenVisitor.h"

CodeGenVisitor::CodeGenVisitor(CodeGenerator& cg)
    : m_cg(cg), m_lastValue(nullptr), m_lastVarType(VarType::INT)
{}

void CodeGenVisitor::generate(const std::vector<std::unique_ptr<ASTNode>>& program)
{
    for (const auto& node : program) node->accept(*this);
}

llvm::Value* CodeGenVisitor::getValue() { return m_lastValue; }

// ─────────────────────────────────────────────
//  VarType → LLVM scalar Type*
// ─────────────────────────────────────────────
llvm::Type* CodeGenVisitor::varTypeToLLVM(VarType vt)
{
    auto& builder = m_cg.getBuilder();
    auto& context = m_cg.getContext();
    switch (vt) {
        case VarType::INT:    return builder.getInt32Ty();
        case VarType::BOOL:   return builder.getInt1Ty();
        case VarType::STRING: return llvm::PointerType::getUnqual(context);
        default:
            throw std::runtime_error("Unsupported element type");
    }
}

// ─────────────────────────────────────────────
//  Literals
// ─────────────────────────────────────────────
void CodeGenVisitor::visit(const NumberNode& node)
{
    m_lastValue   = llvm::ConstantInt::get(m_cg.getBuilder().getInt32Ty(), node.value);
    m_lastVarType = VarType::INT;
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
}

// ─────────────────────────────────────────────
//  Expressions
// ─────────────────────────────────────────────
void CodeGenVisitor::visit(const BinaryOpNode& node)
{
    auto& builder = m_cg.getBuilder();
    node.left->accept(*this);  llvm::Value* L = m_lastValue;
    node.right->accept(*this); llvm::Value* R = m_lastValue;

    switch (node.op) {
        case TokenType::PLUS:          m_lastValue = builder.CreateAdd (L, R, "add"); break;
        case TokenType::MINUS:         m_lastValue = builder.CreateSub (L, R, "sub"); break;
        case TokenType::STAR:          m_lastValue = builder.CreateMul (L, R, "mul"); break;
        case TokenType::SLASH:         m_lastValue = builder.CreateSDiv(L, R, "div"); break;
        case TokenType::LESS:          m_lastValue = builder.CreateICmpSLT(L, R, "lt");  break;
        case TokenType::GREATER:       m_lastValue = builder.CreateICmpSGT(L, R, "gt");  break;
        case TokenType::LESS_EQUAL:    m_lastValue = builder.CreateICmpSLE(L, R, "le");  break;
        case TokenType::GREATER_EQUAL: m_lastValue = builder.CreateICmpSGE(L, R, "ge");  break;
        case TokenType::DOUBLE_EQUAL:  m_lastValue = builder.CreateICmpEQ (L, R, "eq");  break;
        case TokenType::NOT_EQUAL:     m_lastValue = builder.CreateICmpNE (L, R, "ne");  break;
        case TokenType::LOGICAL_AND:
            m_lastValue = builder.CreateAnd(toBool(L), toBool(R), "and"); break;
        case TokenType::LOGICAL_OR:
            m_lastValue = builder.CreateOr (toBool(L), toBool(R), "or");  break;
        default: throw std::runtime_error("Unsupported binary operator in codegen");
    }
}

void CodeGenVisitor::visit(const UnaryOpNode& node)
{
    auto& builder = m_cg.getBuilder();
    node.operand->accept(*this);
    switch (node.op) {
        case TokenType::MINUS: m_lastValue = builder.CreateNeg(m_lastValue, "neg"); break;
        case TokenType::NOT:   m_lastValue = builder.CreateNot(m_lastValue, "not"); break;
        default: throw std::runtime_error("Unsupported unary operator in codegen");
    }
}

// ─────────────────────────────────────────────
//  LetNode
// ─────────────────────────────────────────────
void CodeGenVisitor::visit(const LetNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* func = builder.GetInsertBlock()->getParent();

    if (node.hasTypeAnnotation && node.isArray) {
        VarType elemVarType = VarType::INT;
        if      (node.annotatedType == "int")    elemVarType = VarType::INT;
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
                llvm::Value* idx = llvm::ConstantInt::get(builder.getInt32Ty(), i);
                llvm::Value* ptr = m_cg.getArrayElementPtr(alloc, idx, elemLLVM);
                builder.CreateStore(m_lastValue, ptr);
            }
        }
        return;
    }

    // Scalar
    node.value->accept(*this);
    llvm::Value*      val    = m_lastValue;
    llvm::AllocaInst* alloc  = m_cg.createEntryBlockAlloca(func, node.variableName, val->getType());
    builder.CreateStore(val, alloc);
    m_cg.declareVariable(node.variableName, alloc);
}

// ─────────────────────────────────────────────
//  Array nodes
// ─────────────────────────────────────────────
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

    // ── Distinguish [N x elemTy] alloca (local) from ptr alloca (param) ──
    llvm::Type* allocatedTy = alloc->getAllocatedType();
    llvm::Value* ptr;

    if (allocatedTy->isArrayTy()) {
        // Local array: use two-index GEP {0, i}
        ptr = m_cg.getArrayElementPtr(alloc, idx, elemTy);
    } else {
        // Array parameter: the alloca holds a pointer; load the pointer first
        llvm::Value* base = builder.CreateLoad(allocatedTy, alloc, node.name + "_ptr");
        ptr = builder.CreateInBoundsGEP(elemTy, base, idx, "elemptr");
    }

    m_lastValue = builder.CreateLoad(elemTy, ptr, node.name + "_elem");
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

// ─────────────────────────────────────────────
//  AssignmentNode / ReturnNode / BlockNode
// ─────────────────────────────────────────────
void CodeGenVisitor::visit(const AssignmentNode& node)
{
    node.expression->accept(*this);
    llvm::AllocaInst* var = m_cg.lookupVariable(node.variableName);
    if (!var) throw std::runtime_error("Unknown variable: " + node.variableName);
    m_cg.getBuilder().CreateStore(m_lastValue, var);
}

void CodeGenVisitor::visit(const ReturnNode& node)
{
    node.expression->accept(*this);
    m_cg.getBuilder().CreateRet(m_lastValue);
}

void CodeGenVisitor::visit(const BlockNode& node)
{
    m_cg.pushScope();
    for (const auto& stmt : node.statements) stmt->accept(*this);
    m_cg.popScope();
}

// ─────────────────────────────────────────────
//  IfNode / WhileNode
// ─────────────────────────────────────────────
void CodeGenVisitor::visit(const IfNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* func = builder.GetInsertBlock()->getParent();

    node.condition->accept(*this);
    llvm::Value* cond = toBool(m_lastValue);

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

// ─────────────────────────────────────────────
//  FunctionNode  — KEY CHANGE
//
//  Array parameters are passed as pointers (ptr to element type),
//  which is the standard C ABI for array decay.
//
//  For each array param:
//    - LLVM argument type:  ptr (opaque pointer, LLVM 15+)
//    - Alloca type:         ptr  (we store the incoming ptr into a slot)
//    - Element type table:  registered so arr[i] GEPs work inside the body
// ─────────────────────────────────────────────
void CodeGenVisitor::visit(const FunctionNode& node)
{
    auto& builder = m_cg.getBuilder();
    auto& module  = m_cg.getModule();
    auto& context = m_cg.getContext();

    // Return type
    llvm::Type* retType = nullptr;
    if      (node.returnType == "void")   retType = builder.getInt32Ty();
    else if (node.returnType == "int")    retType = builder.getInt32Ty();
    else if (node.returnType == "bool")   retType = builder.getInt1Ty();
    else if (node.returnType == "string") retType = llvm::PointerType::getUnqual(context);
    else throw std::runtime_error("Unsupported return type: " + node.returnType);

    // Parameter types
    std::vector<llvm::Type*> argTypes;
    for (const auto& p : node.parameters) {
        if (p.isArray) {
            // Arrays decay to a pointer to the element type
            llvm::Type* elemTy = varTypeToLLVM(stringToVarType(p.type));
            argTypes.push_back(llvm::PointerType::getUnqual(context));
            (void)elemTy; // elemTy used below when registering; opaque ptr for the signature
        } else {
            if      (p.type == "int")    argTypes.push_back(builder.getInt32Ty());
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

// ─────────────────────────────────────────────
//  CallNode  — KEY CHANGE
//
//  When an argument is a local array variable,
//  we must pass a pointer to its first element
//  (GEP {0,0}) rather than trying to load the
//  whole [N x T] value.
// ─────────────────────────────────────────────
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
        args.push_back(m_lastValue);
    }

    m_lastValue = builder.CreateCall(function, args, "calltmp");
}

// ─────────────────────────────────────────────
//  PrintNode
// ─────────────────────────────────────────────
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
        argTypes.push_back(v->getType()->isPointerTy() ? VarType::STRING : VarType::INT);
    }

    std::string fmtStr = convertFormatString(node.formatString, argTypes, node.newline);
    std::vector<llvm::Value*> callArgs = { builder.CreateGlobalString(fmtStr, ".fmt") };
    callArgs.insert(callArgs.end(), argVals.begin(), argVals.end());
    builder.CreateCall(printfFunc, callArgs);
}

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
llvm::Value* CodeGenVisitor::toBool(llvm::Value* val)
{
    auto& builder = m_cg.getBuilder();
    if (val->getType()->isIntegerTy(1)) return val;
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

// ─────────────────────────────────────────────
//  Private helper: string type name → VarType
//  (mirrors SemanticAnalyzer::stringToVarType but
//   available here without a SemanticAnalyzer instance)
// ─────────────────────────────────────────────
VarType CodeGenVisitor::stringToVarType(const std::string& s)
{
    if (s == "int")    return VarType::INT;
    if (s == "float")  return VarType::FLOAT;
    if (s == "string") return VarType::STRING;
    if (s == "bool")   return VarType::BOOL;
    if (s == "void")   return VarType::VOID;
    throw std::runtime_error("Unknown type: " + s);
}
