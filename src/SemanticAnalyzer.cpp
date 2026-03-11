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
            
            if (node.op == TokenType::SLASH) {
                if(auto literal = dynamic_cast<const NumberNode*>(node.right.get())) {
                    if(literal->value == 0) {
                        throw std::runtime_error("Division by zero");
                    }
                }
            }
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
    if (m_currentType != VarType::BOOL && m_currentType != VarType::INT) {
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

void SemanticAnalyzer::visit(const FunctionNode& node)
{
    if(m_functionsTable.find(node.name) != m_functionsTable.end()) {
        throw std::runtime_error("Function already declared: " + node.name);
    }

    FunctionInfo info;

    for(auto& param : node.parameters)
        info.paramTypes.push_back(stringToVarType(param.type));

    info.returnType = stringToVarType(node.returnType);

    m_functionsTable[node.name] = info;

    VarType returnType = info.returnType;

    m_currentFunctionReturnType = returnType;
    m_foundReturn = false;

    m_insideFunction = true;
    enterScope();

    for(auto& param : node.parameters)
    {
        declareVariable(param.name, stringToVarType(param.type), false);
    }

    node.body->accept(*this);

    exitScope();
    m_insideFunction = false;

    if(returnType != VarType::VOID && !m_foundReturn)
    {
        throw std::runtime_error("Function '" + node.name + "' must return a value");
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

VarType SemanticAnalyzer::stringToVarType(const std::string& typeStr)
{

    if (typeStr == "int") return VarType::INT;
    if (typeStr == "float") return VarType::FLOAT;
    if (typeStr == "string") return VarType::STRING;
    if (typeStr == "bool") return VarType::BOOL;
    if (typeStr == "void") return VarType::VOID;
    throw std::runtime_error("Unknown type: " + typeStr);

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

void SemanticAnalyzer::visit(const ReturnNode& node)
{
    if(!m_insideFunction)
    {
        throw std::runtime_error("Return statement outside function");
    }
    node.expression->accept(*this);

    if(m_currentType != m_currentFunctionReturnType)
    {
        throw std::runtime_error("Return type mismatch");
    }

    m_foundReturn = true;
}

void SemanticAnalyzer::visit(const CallNode& node)
{
    // Check that the function exists
    auto it = m_functionsTable.find(node.functionName);
    if(it == m_functionsTable.end()) {
        throw std::runtime_error("Call to undefined function: " + node.functionName);
    }

    const FunctionInfo& info = it->second;

    // Check argument count
    if(node.arguments.size() != info.paramTypes.size()) {
        throw std::runtime_error("Function '" + node.functionName +
                                 "' expects " + std::to_string(info.paramTypes.size()) +
                                 " arguments, got " + std::to_string(node.arguments.size()));
    }

    // Analyze arguments and check types
    for(size_t i = 0; i < node.arguments.size(); i++) {
        node.arguments[i]->accept(*this);
        if(m_currentType != info.paramTypes[i]) {
            throw std::runtime_error("Type mismatch for argument " + std::to_string(i+1) +
                                     " in call to function '" + node.functionName + "'");
        }
    }

    // The type of a function call expression is the function's return type
    m_currentType = info.returnType;
}

void SemanticAnalyzer::visit(const PrintNode& node)
{
    if (!node.expression) {
        throw std::runtime_error("Print/Println requires an expression");
    }

    // Analyze the expression
    node.expression->accept(*this);

    // Only allow int or bool for now
    if (m_currentType != VarType::INT && m_currentType != VarType::BOOL) {
        throw std::runtime_error("Print/Println only supports int or bool expressions");
    }

    // Print statements themselves are void
    m_currentType = VarType::VOID;
}
