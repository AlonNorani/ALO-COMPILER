#include "SemanticAnalyzer.h"

SemanticAnalyzer::SemanticAnalyzer()
{
    enterScope(); // start with global scope
}

SemanticAnalyzer::~SemanticAnalyzer()
{

}


void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    for (const auto& node : ast) {
        node->accept(*this); // use visitor pattern to analyze each node
    }
}

void SemanticAnalyzer::visit(const NumberNode& node)
{
    m_currentType = VarType::INT; 
}

void SemanticAnalyzer::visit(const AssignmentNode& node)
{
    auto var = lookupVariable(node.variableName); // look for the variable being assigned to
    if (!var) 
        throw std::runtime_error("Undefined variable: " + node.variableName);
    if(var->get().mutableFlag == false) {
        throw std::runtime_error("Cannot assign to immutable variable: " + node.variableName);
    }
    node.expression->accept(*this); // analyze the expression being assigned
    
    if(var->get().type != m_currentType) {
        throw std::runtime_error("Type mismatch in assignment to variable: " + node.variableName);
    }
    
}

void SemanticAnalyzer::visit(const BinaryOpNode& node)
{
    node.left->accept(*this);
    VarType leftType = m_currentType;

    node.right->accept(*this);
    VarType rightType = m_currentType;

    switch (node.op)
    {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::STAR:
        case TokenType::SLASH:
        {
            if (leftType != VarType::INT || rightType != VarType::INT)
                throw std::runtime_error("Arithmetic operations require integers");

            m_currentType = VarType::INT;
            break;
        }

        case TokenType::GREATER:
        case TokenType::LESS:
        case TokenType::GREATER_EQUAL:
        case TokenType::LESS_EQUAL:
        {
            if (leftType != VarType::INT || rightType != VarType::INT)
                throw std::runtime_error("Comparison operators require integers");

            m_currentType = VarType::BOOL;
            break;
        }

        case TokenType::DOUBLE_EQUAL:
        case TokenType::NOT_EQUAL:
        {
            if (leftType != rightType)
                throw std::runtime_error("Equality comparison requires operands of the same type");

            m_currentType = VarType::BOOL;
            break;
        }

        case TokenType::LOGICAL_AND:
        case TokenType::LOGICAL_OR:
        {
            if ((leftType != VarType::BOOL && leftType != VarType::INT) ||
                (rightType != VarType::BOOL && rightType != VarType::INT))
            {
                throw std::runtime_error("Logical operations require bool or int operands");
            }

            m_currentType = VarType::BOOL;
            break;
        }

        default:
            throw std::runtime_error("Unsupported binary operator");
    }
}

void SemanticAnalyzer::visit(const VariableNode& node)
{
    auto var = lookupVariable(node.name); // look for the variable
    if (!var) {
        throw std::runtime_error("Undefined variable: " + node.name);
    }
    m_currentType = var->get().type; 
}

void SemanticAnalyzer::visit(const BlockNode& node)
{
    enterScope(); // enter a new scope for the block
    for(const auto& stmt : node.statements) {
        stmt->accept(*this); // analyze each statement in the block
    }
    exitScope(); // exit the block scope
}

void SemanticAnalyzer::visit(const IfNode& node)
{
    node.condition->accept(*this); 
    if (m_currentType != VarType::BOOL) {
        throw std::runtime_error("Condition in 'if' statement must be of type bool");
    }

    node.thenBranch->accept(*this); 
    if(node.elseBranch) { 
        node.elseBranch->accept(*this); // analyze the else branch if it exists
    }
}

void SemanticAnalyzer::visit(const WhileNode& node)
{
    node.condition->accept(*this); // analyze the loop condition
    if(m_currentType != VarType::BOOL) {
        throw std::runtime_error("Condition in 'while' statement must be of type bool");
    }
    node.body->accept(*this); // analyze the loop body
}

void SemanticAnalyzer::visit(const BoolNode& node)
{
    m_currentType = VarType::BOOL;
}

void SemanticAnalyzer::visit(const LetNode& node)
{
    node.value->accept(*this); // analyze the initializer expression
    VarType varType = m_currentType; // get the type of the initializer

    if (!declareVariable(node.variableName, varType, node.isMutable)) {
        throw std::runtime_error("Variable already declared in this scope: " + node.variableName);
    }
}

void SemanticAnalyzer::visit(const UnaryOpNode& node)
{
    node.operand->accept(*this);
    VarType operandType = m_currentType;

    switch (node.op)
    {
        case TokenType::NOT:
        {
            if (operandType != VarType::BOOL && operandType != VarType::INT)
                throw std::runtime_error("Logical NOT requires bool or int operand");

            m_currentType = VarType::BOOL;
            break;
        }

        case TokenType::MINUS:
        {
            if (operandType != VarType::INT)
                throw std::runtime_error("Unary minus requires integer operand");

            m_currentType = VarType::INT;
            break;
        }

        default:
            throw std::runtime_error("Unsupported unary operator");
    }
}

void SemanticAnalyzer::enterScope()
{
    m_symbolTable.emplace_back(); // Add a new scope, just an empty map
}

void SemanticAnalyzer::exitScope()
{
    if (!m_symbolTable.empty())
        m_symbolTable.pop_back(); // remove the current scope
}

bool SemanticAnalyzer::declareVariable(const std::string& name, VarType type, bool mutableFlag)
{
    // variables are immutable by default, mutableFlag can be set to true to allow mutation
    auto& currentScope = m_symbolTable.back();

    if (currentScope.find(name) != currentScope.end()) {
        return false; // variable already declared in this scope
    }

    currentScope[name] = VariableInfo{name, type, mutableFlag};
    return true;
}

std::optional<std::reference_wrapper<VariableInfo>> SemanticAnalyzer::lookupVariable(const std::string& name) {
    // search from inner scope to outer scope
    for (auto it = m_symbolTable.rbegin(); it != m_symbolTable.rend(); ++it) {
        auto found = it->find(name); // find the variable in the current scope
        if (found != it->end()) {
            return std::ref(found->second); // return reference wrapped in optional
        }
    }
    return std::nullopt; // not found
}

