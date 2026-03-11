#include "CodeGenVisitor.h"

CodeGenVisitor::CodeGenVisitor(CodeGenerator& cg)
    : m_cg(cg), m_lastValue(nullptr)
{

}

CodeGenVisitor::~CodeGenVisitor()
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

    node.left->accept(*this);
    llvm::Value* L = m_lastValue;

    node.right->accept(*this);
    llvm::Value* R = m_lastValue;

    switch(node.op)
    {
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

        default:
            throw std::runtime_error("Unsupported binary operator");
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

void CodeGenVisitor::visit(const UnaryOpNode& node)
{
    auto& builder = m_cg.getBuilder();

    node.operand->accept(*this);
    llvm::Value* value = m_lastValue;

    switch(node.op)
    {
        case TokenType::MINUS:
            m_lastValue = builder.CreateNeg(value, "negtmp");
            break;

        case TokenType::NOT:
            m_lastValue = builder.CreateNot(value, "nottmp");
            break;

        default:
            throw std::runtime_error("Unsupported unary operator");
    }
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

    std::vector<llvm::Type*> argTypes(
        node.parameters.size(),
        builder.getInt32Ty()
    );

    llvm::FunctionType* funcType =
        llvm::FunctionType::get(builder.getInt32Ty(), argTypes, false);

    llvm::Function* function =
        llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            node.name,
            module
        );

    unsigned idx = 0;
    for(auto& arg : function->args())
        arg.setName(node.parameters[idx++].name);

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(builder.getContext(), "entry", function);

    builder.SetInsertPoint(entry);

    m_cg.pushScope();

    for(auto& arg : function->args())
    {
        auto alloc =
            m_cg.createEntryBlockAlloca(function, arg.getName().str());

        builder.CreateStore(&arg, alloc);

        m_cg.declareVariable(arg.getName().str(), alloc);
    }

    node.body->accept(*this);

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

