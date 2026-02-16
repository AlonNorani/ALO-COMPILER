#ifndef SEMANTICANALYZER_H
#define SEMANTICANALYZER_H
#include <string>
#include <unordered_map>
#include "Token.h"
#include <vector>
#include "ASTNodes.h"
#include <memory>
#include <optional>
#include <functional>
#include <stdexcept>

enum class VarType {
    INT,
    FLOAT,
    STRING,
    BOOL,
    VOID
};

struct VariableInfo {
    std::string name;
    VarType type; 
    bool mutableFlag; 
};

using SymbolTable = std::vector<std::unordered_map<std::string, VariableInfo>>;

class SemanticAnalyzer
{
public:
    SemanticAnalyzer();
    ~SemanticAnalyzer();
    void analyze(const std::vector<std::unique_ptr<ASTNode>>& ast);

private:
    SymbolTable m_symbolTable;
    // manage scopes
    void enterScope();
    void exitScope();
    // variable declaration and lookup
    bool declareVariable(const std::string& name, VarType type, bool mutableFlag = false);
    std::optional<std::reference_wrapper<VariableInfo>> lookupVariable(const std::string& name);
    // Semantic checks for AST nodes
    void analyzeNode(const ASTNode* node);
    void analyzeAssignment(const AssignmentNode* node);
    void analyzeBinaryOp(const BinaryOpNode* node);
    void analyzeVariable(const VariableNode* node);
    void analyzeBlock(const BlockNode* node);
    void analyzeIf(const IfNode* node);
    void analyzeWhile(const WhileNode* node);



};

#endif