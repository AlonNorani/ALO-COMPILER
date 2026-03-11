#include "CodeGenVisitor.h"

CodeGenVisitor::CodeGenVisitor(CodeGenerator& cg)
    : m_cg(cg), m_lastValue(nullptr)
{

}



void CodeGenVisitor::visit(const NumberNode& node)
{
    m_lastValue = llvm::ConstantInt::get(
        m_cg.getBuilder().getInt32Ty(),
        node.value
    );
}

void CodeGenVisitor::visit(const BoolNode& node)
{
    m_lastValue = llvm::ConstantInt::get(
        m_cg.getBuilder().getInt1Ty(), // bool is represented as i1 in LLVM
        node.value
    );
}

void CodeGenVisitor::visit(const VariableNode& node)
{
    m_lastValue = m_cg.loadVariable(node.name);
}

void CodeGenVisitor::visit(const BinaryOpNode& node)
{
    auto& builder = m_cg.getBuilder();

    // Generate code for left and right expressions
    node.left->accept(*this);
    llvm::Value* L = m_lastValue;

    node.right->accept(*this);
    llvm::Value* R = m_lastValue;

    switch (node.op)
    {
        // Arithmetic
        case TokenType::PLUS:
            m_lastValue = builder.CreateAdd(L, R, "addtmp");
            break;
        case TokenType::MINUS:
            m_lastValue = builder.CreateSub(L, R, "subtmp");
            break;
        case TokenType::STAR:
            m_lastValue = builder.CreateMul(L, R, "multmp");
            break;
        case TokenType::SLASH:
            m_lastValue = builder.CreateSDiv(L, R, "divtmp");
            break;

        // Comparison
        case TokenType::LESS:
            m_lastValue = builder.CreateICmpSLT(L, R, "lttmp");
            break;
        case TokenType::GREATER:
            m_lastValue = builder.CreateICmpSGT(L, R, "gttmp");
            break;
        case TokenType::LESS_EQUAL:
            m_lastValue = builder.CreateICmpSLE(L, R, "letmp");
            break;
        case TokenType::GREATER_EQUAL:
            m_lastValue = builder.CreateICmpSGE(L, R, "getmp");
            break;

        // Equality
        case TokenType::DOUBLE_EQUAL:
            m_lastValue = builder.CreateICmpEQ(L, R, "eqtmp");
            break;
        case TokenType::NOT_EQUAL:
            m_lastValue = builder.CreateICmpNE(L, R, "netmp");
            break;

        // Logical
        case TokenType::LOGICAL_AND:
        {
            L = toBool(L);
            R = toBool(R);
            m_lastValue = builder.CreateAnd(L, R, "andtmp");
            break;
        }
        case TokenType::LOGICAL_OR:
        {
            L = toBool(L);
            R = toBool(R);
            m_lastValue = builder.CreateOr(L, R, "ortmp");
            break;
        }

        default:
            throw std::runtime_error("Unsupported binary operator in codegen");
    }
}

void CodeGenVisitor::visit(const UnaryOpNode& node)
{
    auto& builder = m_cg.getBuilder();

    node.operand->accept(*this);
    llvm::Value* value = m_lastValue;

    switch (node.op)
    {
        case TokenType::MINUS:   // Arithmetic negation
            m_lastValue = builder.CreateNeg(value, "negtmp");
            break;

        case TokenType::NOT:     // Bitwise NOT for i1 (bool) or i32
            m_lastValue = builder.CreateNot(value, "nottmp");
            break;

        default:
            throw std::runtime_error("Unsupported unary operator in codegen");
    }
}

void CodeGenVisitor::visit(const LetNode& node)
{
    auto& builder = m_cg.getBuilder();

    node.value->accept(*this);
    llvm::Value* value = m_lastValue;

    llvm::Function* function =
        builder.GetInsertBlock()->getParent(); // get the current function we're generating code for

    auto alloc =
        m_cg.createEntryBlockAlloca(function, node.variableName); // just allocate space for the variable in the entry block

    builder.CreateStore(value, alloc);

    m_cg.declareVariable(node.variableName, alloc); // add the variable to the current scope
}

void CodeGenVisitor::visit(const AssignmentNode& node)
{
    node.expression->accept(*this);
    llvm::Value* value = m_lastValue;

    auto var = m_cg.lookupVariable(node.variableName);

    if(!var)
        throw std::runtime_error("Unknown variable");

    m_cg.getBuilder().CreateStore(value, var);
}

void CodeGenVisitor::visit(const ReturnNode& node)
{
    node.expression->accept(*this);

    m_cg.getBuilder().CreateRet(m_lastValue);
}

void CodeGenVisitor::visit(const BlockNode& node)
{
    m_cg.pushScope();

    for(const auto& stmt : node.statements)
        stmt->accept(*this);

    m_cg.popScope();
}


void CodeGenVisitor::visit(const IfNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* function = builder.GetInsertBlock()->getParent();

    node.condition->accept(*this);
    llvm::Value* cond = m_lastValue;

    llvm::BasicBlock* thenBB =
        llvm::BasicBlock::Create(builder.getContext(), "then", function);

    llvm::BasicBlock* elseBB =
        llvm::BasicBlock::Create(builder.getContext(), "else", function);

    llvm::BasicBlock* mergeBB =
        llvm::BasicBlock::Create(builder.getContext(), "ifcont", function);

    builder.CreateCondBr(cond, thenBB, elseBB);

    builder.SetInsertPoint(thenBB);
    node.thenBranch->accept(*this);
    builder.CreateBr(mergeBB);

    builder.SetInsertPoint(elseBB);
    if(node.elseBranch)
        node.elseBranch->accept(*this);
    builder.CreateBr(mergeBB);

    builder.SetInsertPoint(mergeBB);
}

void CodeGenVisitor::visit(const WhileNode& node)
{
    auto& builder = m_cg.getBuilder();
    llvm::Function* function = builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* condBB =
        llvm::BasicBlock::Create(builder.getContext(), "whilecond", function);

    llvm::BasicBlock* bodyBB =
        llvm::BasicBlock::Create(builder.getContext(), "whilebody", function);

    llvm::BasicBlock* endBB =
        llvm::BasicBlock::Create(builder.getContext(), "whileend", function);

    builder.CreateBr(condBB);

    builder.SetInsertPoint(condBB);
    node.condition->accept(*this);

    builder.CreateCondBr(m_lastValue, bodyBB, endBB);

    builder.SetInsertPoint(bodyBB);
    node.body->accept(*this);

    builder.CreateBr(condBB);

    builder.SetInsertPoint(endBB);
}

void CodeGenVisitor::visit(const FunctionNode& node)
{
    auto& builder = m_cg.getBuilder();
    auto& module = m_cg.getModule();
    llvm::LLVMContext& context = builder.getContext();

    // --- Determine return type ---
    llvm::Type* retType = nullptr;
    if (node.returnType == "void") {
        retType = builder.getVoidTy();
    } else if (node.returnType == "int") {
        retType = builder.getInt32Ty();
    } else if (node.returnType == "bool") {
        retType = builder.getInt1Ty();
    } else {
        throw std::runtime_error("Unsupported return type: " + node.returnType);
    }

    // --- Determine argument types ---
    std::vector<llvm::Type*> argTypes;
    for (auto& param : node.parameters) {
        if (param.type == "int") {
            argTypes.push_back(builder.getInt32Ty());
        } else if (param.type == "bool") {
            argTypes.push_back(builder.getInt1Ty());
        } else {
            throw std::runtime_error("Unsupported parameter type: " + param.type);
        }
    }

    // --- Create the function type ---
    llvm::FunctionType* funcType = llvm::FunctionType::get(retType, argTypes, false);

    // --- Create the function in the module ---
    llvm::Function* function = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        node.name,
        module
    );

    // --- Name the arguments ---
    unsigned idx = 0;
    for (auto& arg : function->args())
        arg.setName(node.parameters[idx++].name);

    // --- Create entry block ---
    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entryBB);

    m_cg.pushScope();

    // --- Allocate stack space for arguments and store their values ---
    for (auto& arg : function->args()) {
        llvm::AllocaInst* alloc = m_cg.createEntryBlockAlloca(function, arg.getName().str());
        builder.CreateStore(&arg, alloc);
        m_cg.declareVariable(arg.getName().str(), alloc);
    }

    // --- Generate code for the function body ---
    if (node.body)
        node.body->accept(*this);

    // --- Ensure the function has a return instruction ---
    if (!builder.GetInsertBlock()->getTerminator()) {
        if (retType->isVoidTy()) {
            builder.CreateRetVoid();
        } else if (retType->isIntegerTy()) {
            builder.CreateRet(m_lastValue ? m_lastValue : llvm::ConstantInt::get(retType, 0));
        } else if (retType->isPointerTy()) {
            builder.CreateRet(m_lastValue ? m_lastValue :
                llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(retType)));
        }
    }

    m_cg.popScope();
}

void CodeGenVisitor::visit(const CallNode& node)
{
    auto& builder = m_cg.getBuilder();
    auto& module = m_cg.getModule();

    llvm::Function* function =
        module.getFunction(node.functionName);

    if(!function)
        throw std::runtime_error("Unknown function");

    std::vector<llvm::Value*> args;

    for(const auto& arg : node.arguments)
    {
        arg->accept(*this);
        args.push_back(m_lastValue);
    }

    m_lastValue =
        builder.CreateCall(function, args, "calltmp");
}

// Convert int/bool to LLVM i1
llvm::Value* CodeGenVisitor::toBool(llvm::Value* val)
{
    // this function is used to convert int or bool values to i1 (LLVM's boolean type)
    // it helps to support both int and bool types in logical operations and conditions
    auto& builder = m_cg.getBuilder();

    if (val->getType()->isIntegerTy(1))
        return val;

    return builder.CreateICmpNE(
        val,
        llvm::ConstantInt::get(val->getType(), 0),
        "tobool"
    );
}

void CodeGenVisitor::generate(const std::vector<std::unique_ptr<ASTNode>>& program)
{
    for (const auto& node : program)
    {
        node->accept(*this);
    }
}