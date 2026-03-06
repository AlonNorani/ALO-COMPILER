#include "include/SemanticAnalyzer.h"

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

void SemanticAnalyzer::visit(const AssignmentNode& node)
{
    auto var = lookupVariable(node.variableName); // look for the variable being assigned to
    if (!var) 
        throw std::runtime_error("Undefined variable: " + node.variableName);
    node.expression->accept(*this); // analyze the expression being assigned
    
}

void SemanticAnalyzer::visit(const BinaryOpNode& node)
{
    node.left->accept(*this); // analyze left operand
    node.right->accept(*this); // analyze right operand
}

void SemanticAnalyzer::visit(const VariableNode& node)
{
    auto var = lookupVariable(node.name); // look for the variable
    if (!var) {
        throw std::runtime_error("Undefined variable: " + node.name);
    }
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
    node.condition->accept(*this); // analyze the condition expression
    node.thenBranch->accept(*this); // analyze the then branch
    if(node.elseBranch) { // check if there is an else branch
        node.elseBranch->accept(*this); // analyze the else branch if it exists
    }
}

void SemanticAnalyzer::visit(const WhileNode& node)
{
    node.condition->accept(*this); // analyze the loop condition
    node.body->accept(*this); // analyze the loop body
    
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

