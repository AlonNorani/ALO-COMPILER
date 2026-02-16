#include "include/SemanticAnalyzer.h"

SemanticAnalyzer::SemanticAnalyzer()
{

}

SemanticAnalyzer::~SemanticAnalyzer()
{

}


void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    enterScope(); // start global scope
    for (const auto& node : ast) {
        analyzeNode(node.get());
    }
    exitScope();
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
    // search from innermost to outermost scope
    for (auto it = m_symbolTable.rbegin(); it != m_symbolTable.rend(); ++it) {
        auto found = it->find(name); // find the variable in the current scope
        if (found != it->end()) {
            return std::ref(found->second); // return reference wrapped in optional
        }
    }
    return std::nullopt; // not found
}

void SemanticAnalyzer::analyzeNode(const ASTNode* node) {
    if (auto assign = dynamic_cast<const AssignmentNode*>(node)) {
        analyzeAssignment(assign);
    }
    else if (auto binOp = dynamic_cast<const BinaryOpNode*>(node)) {
        analyzeBinaryOp(binOp);
    }
    else if (auto var = dynamic_cast<const VariableNode*>(node)) {
        analyzeVariable(var);
    }
    else if (auto block = dynamic_cast<const BlockNode*>(node)) {
        analyzeBlock(block);
    }
    else if (auto ifNode = dynamic_cast<const IfNode*>(node)) {
        analyzeIf(ifNode);
    }
    else if (auto whileNode = dynamic_cast<const WhileNode*>(node)) {
        analyzeWhile(whileNode);
    }
}
