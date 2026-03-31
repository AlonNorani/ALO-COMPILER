#include "CodeGenVisitor.h"

CodeGenVisitor::CodeGenVisitor(CodeGenerator& cg)
    : m_cg(cg), m_lastValue(nullptr)
{
}

void CodeGenVisitor::generate(const std::vector<std::unique_ptr<ASTNode>>& program)
{
    for (const auto& node : program)
        node->accept(*this);
}

void CodeGenVisitor::visit(const NumberNode& node)
{
    m_lastValue = llvm::ConstantInt::get(
        m_cg.getBuilder().getInt32Ty(),
        node.value
    );
    m_lastVarType = VarType::INT;
}

void CodeGenVisitor::visit(const BoolNode& node)
{
    m_lastValue = llvm::ConstantInt::get(
        m_cg.getBuilder().getInt1Ty(),
        node.value
    );
    m_lastVarType = VarType::BOOL;
}

void CodeGenVisitor::visit(const VariableNode& node)
{
    m_lastValue = m_cg.loadVariable(node.name);
}

void CodeGenVisitor::visit(const BinaryOpNode& node)
{
    auto& builder = m_cg.getBuilder();

    node.left->accept(*this);
    llvm::Value* L = m_lastValue;

    node.right->accept(*this);
    llvm::Value* R = m_lastValue;

    switch (node.op)
    {
        case TokenType::PLUS:      m_lastValue = builder.CreateAdd(L, R, "addtmp"); break;
        case TokenType::MINUS:     m_lastValue = builder.CreateSub(L, R, "subtmp"); break;
        case TokenType::STAR:      m_lastValue = builder.CreateMul(L, R, "multmp"); break;
        case TokenType::SLASH:     m_lastValue = builder.CreateSDiv(L, R, "divtmp"); break;

        case TokenType::LESS:      m_lastValue = builder.CreateICmpSLT(L, R, "lttmp"); break;
        case TokenType::GREATER:   m_lastValue = builder.CreateICmpSGT(L, R, "gttmp"); break;
        case TokenType::LESS_EQUAL:m_lastValue = builder.CreateICmpSLE(L, R, "letmp"); break;
        case TokenType::GREATER_EQUAL: m_lastValue = builder.CreateICmpSGE(L, R, "getmp"); break;

        case TokenType::DOUBLE_EQUAL: m_lastValue = builder.CreateICmpEQ(L, R, "eqtmp"); break;
        case TokenType::NOT_EQUAL:    m_lastValue = builder.CreateICmpNE(L, R, "netmp"); break;

        case TokenType::LOGICAL_AND:
            L = toBool(L);
            R = toBool(R);
            m_lastValue = builder.CreateAnd(L, R, "andtmp");
            break;
        case TokenType::LOGICAL_OR:
            L = toBool(L);
            R = toBool(R);
            m_lastValue = builder.CreateOr(L, R, "ortmp");
            break;

        default:
            throw std::runtime_error("Unsupported binary operator in codegen");
    }
}

void CodeGenVisitor::visit(const UnaryOpNode& node)
{
    auto& builder = m_cg.getBuilder();
    node.operand->accept(*this);
    llvm::Value* val = m_lastValue;

    switch (node.op)
    {
        case TokenType::MINUS: m_lastValue = builder.CreateNeg(val, "negtmp"); break;
        case TokenType::NOT:   m_lastValue = builder.CreateNot(val, "nottmp"); break;
        default: throw std::runtime_error("Unsupported unary operator in codegen");
    }
}

void CodeGenVisitor::visit(const LetNode& node)
{
    auto& builder = m_cg.getBuilder();
    node.value->accept(*this);
    llvm::Value* val = m_lastValue;

    llvm::Function* func = builder.GetInsertBlock()->getParent();

    
    llvm::Type* allocTy = val->getType();
    llvm::AllocaInst* alloc = m_cg.createEntryBlockAlloca(func, node.variableName, allocTy);

    builder.CreateStore(val, alloc);
    m_cg.declareVariable(node.variableName, alloc);
}

void CodeGenVisitor::visit(const AssignmentNode& node)
{
    node.expression->accept(*this);
    llvm::Value* val = m_lastValue;

    llvm::Value* var = m_cg.lookupVariable(node.variableName);
    if (!var)
        throw std::runtime_error("Unknown variable");

    m_cg.getBuilder().CreateStore(val, var);
}

void CodeGenVisitor::visit(const ReturnNode& node)
{
    node.expression->accept(*this);
    m_cg.getBuilder().CreateRet(m_lastValue);
}

void CodeGenVisitor::visit(const BlockNode& node)
{
    m_cg.pushScope();
    for (const auto& stmt : node.statements)
        stmt->accept(*this);
    m_cg.popScope();
}

void CodeGenVisitor::visit(const IfNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* func = builder.GetInsertBlock()->getParent();

    node.condition->accept(*this);
    llvm::Value* cond = toBool(m_lastValue);

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(builder.getContext(), "then", func);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(builder.getContext(), "else", func);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(builder.getContext(), "ifcont", func);

    builder.CreateCondBr(cond, thenBB, elseBB);

    builder.SetInsertPoint(thenBB);
    node.thenBranch->accept(*this);
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(mergeBB);

    builder.SetInsertPoint(elseBB);
    if (node.elseBranch)
        node.elseBranch->accept(*this);
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(mergeBB);

    builder.SetInsertPoint(mergeBB);
}

void CodeGenVisitor::visit(const WhileNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* func = builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(builder.getContext(), "whilecond", func);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(builder.getContext(), "whilebody", func);
    llvm::BasicBlock* endBB  = llvm::BasicBlock::Create(builder.getContext(), "whileend", func);

    builder.CreateBr(condBB);

    builder.SetInsertPoint(condBB);
    node.condition->accept(*this);
    builder.CreateCondBr(toBool(m_lastValue), bodyBB, endBB);

    builder.SetInsertPoint(bodyBB);
    node.body->accept(*this);
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(condBB);

    builder.SetInsertPoint(endBB);
}

void CodeGenVisitor::visit(const FunctionNode& node)
{
    auto& builder = m_cg.getBuilder();
    auto& module = m_cg.getModule();
    llvm::LLVMContext& context = builder.getContext();

    llvm::Type* retType = nullptr;
    if (node.returnType == "void") retType = builder.getInt32Ty();
    else if (node.returnType == "int") retType = builder.getInt32Ty();
    else if (node.returnType == "bool") retType = builder.getInt1Ty();
    else if (node.returnType == "string") retType = llvm::PointerType::getUnqual(context);
    else throw std::runtime_error("Unsupported return type: " + node.returnType);

    std::vector<llvm::Type*> argTypes;
    for (auto& param : node.parameters)
    {
        if (param.type == "int") argTypes.push_back(builder.getInt32Ty());
        else if (param.type == "bool") argTypes.push_back(builder.getInt1Ty());
        else if (param.type == "string") argTypes.push_back(llvm::PointerType::getUnqual(context));
        else throw std::runtime_error("Unsupported parameter type: " + param.type);
    }

    llvm::FunctionType* funcType = llvm::FunctionType::get(retType, argTypes, false);
    llvm::Function* function = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, node.name, &module
    );

    unsigned idx = 0;
    for (auto& arg : function->args())
        arg.setName(node.parameters[idx++].name);

    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entryBB);

    m_cg.pushScope();

    for (auto& arg : function->args())
    {
        llvm::Type* argType = arg.getType();
        llvm::AllocaInst* alloc = builder.CreateAlloca(argType, nullptr, arg.getName());
        builder.CreateStore(&arg, alloc);
        m_cg.declareVariable(arg.getName().str(), alloc);
    }

    if (node.body)
        node.body->accept(*this);

    if (!builder.GetInsertBlock()->getTerminator())
    {
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
    auto& module = m_cg.getModule();

    llvm::Function* function = module.getFunction(node.functionName);
    if (!function)
        throw std::runtime_error("Unknown function: " + node.functionName);

    std::vector<llvm::Value*> args;
    for (auto& arg : node.arguments)
    {
        arg->accept(*this);
        args.push_back(m_lastValue);
    }

    m_lastValue = builder.CreateCall(function, args, "calltmp");
}

void CodeGenVisitor::visit(const PrintNode& node)
{
    auto& builder = m_cg.getBuilder();
    auto& context = m_cg.getContext();
    auto& module = m_cg.getModule();

    // Declare printf once
    llvm::Function* printfFunc = module.getFunction("printf");
    if (!printfFunc)
    {
        
        llvm::FunctionType* printfType = llvm::FunctionType::get(
            builder.getInt32Ty(),
            { llvm::PointerType::getUnqual(context) },
            true
        );
        printfFunc = llvm::Function::Create(
            printfType,
            llvm::Function::ExternalLinkage,
            "printf",
            &module
        );
    }

  
    if (node.args.empty())
    {
        std::string fmt = node.formatString + (node.newline ? "\n" : "");
        llvm::Value* fmtPtr = builder.CreateGlobalString(fmt, ".fmt");
        builder.CreateCall(printfFunc, { fmtPtr });
        return;
    }

    std::vector<llvm::Value*> argVals;
    std::vector<VarType> argTypes;
    for (auto& argNode : node.args)
    {
       
        argNode->accept(*this);
        llvm::Value* v = m_lastValue;

        // Widen i1 booleans to i32 so %d works correctly.
        if (v->getType()->isIntegerTy(1))
            v = builder.CreateZExt(v, builder.getInt32Ty());

        argVals.push_back(v);

        // Determine format specifier type for convertFormatString
        if (v->getType()->isIntegerTy())
            argTypes.push_back(VarType::INT);
        else if (v->getType()->isPointerTy())
            argTypes.push_back(VarType::STRING);
        else
            throw std::runtime_error("Unsupported type in PrintNode");
    }

    std::string fmtStr = convertFormatString(node.formatString, argTypes, node.newline);
    llvm::Value* fmtPtr = builder.CreateGlobalString(fmtStr, ".fmt");

    std::vector<llvm::Value*> callArgs = { fmtPtr };
    callArgs.insert(callArgs.end(), argVals.begin(), argVals.end());
    builder.CreateCall(printfFunc, callArgs);
}

void CodeGenVisitor::visit(const StringNode& node)
{
    // FIX: CreateGlobalString instead of deprecated CreateGlobalStringPtr.
    m_lastValue = m_cg.getBuilder().CreateGlobalString(node.value, ".str");
    m_lastVarType = VarType::STRING;
}

llvm::Value* CodeGenVisitor::toBool(llvm::Value* val)
{
    auto& builder = m_cg.getBuilder();
    if (val->getType()->isIntegerTy(1))
        return val;

    return builder.CreateICmpNE(val, llvm::ConstantInt::get(val->getType(), 0), "tobool");
}

std::string CodeGenVisitor::convertFormatString(const std::string& input, const std::vector<VarType>& argTypes, bool newline)
{
    std::string result;
    size_t typeIndex = 0;

    for (size_t i = 0; i < input.size(); i++)
    {
        if (input[i] == '{' && i + 1 < input.size() && input[i + 1] == '}')
        {
            if (typeIndex >= argTypes.size())
                throw std::runtime_error("Too few types for placeholders");

            switch(argTypes[typeIndex])
            {
                case VarType::INT:
                case VarType::BOOL: result += "%d"; break;
                case VarType::FLOAT: result += "%f"; break;
                case VarType::STRING: result += "%s"; break;
                default: throw std::runtime_error("Unsupported type for printf");
            }

            typeIndex++;
            i++; // skip '}'
        }
        else result += input[i];
    }

    if (newline) result += "\n";
    return result;
}
