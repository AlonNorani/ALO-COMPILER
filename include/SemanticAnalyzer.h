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
#include "ASTVisitor.h"

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

struct FunctionInfo
{
    std::vector<VarType> paramTypes;
    VarType returnType;
};

using SymbolTable = std::vector<std::unordered_map<std::string, VariableInfo>>;
using FunctionsTable = std::unordered_map<std::string, FunctionInfo>;
class SemanticAnalyzer : public ASTVisitor
{
public:
    SemanticAnalyzer();
    ~SemanticAnalyzer();

    void analyze(const std::vector<std::unique_ptr<ASTNode>>& ast);

    // Visitor functions

    // literals
    virtual void visit(const NumberNode& node) override;
    virtual void visit(const BoolNode& node) override;

    // operations
    virtual void visit(const BinaryOpNode& node) override;
    virtual void visit(const UnaryOpNode& node) override;

    // variables and assignments
    virtual void visit(const VariableNode& node) override;
    virtual void visit(const LetNode& node) override;
    virtual void visit(const AssignmentNode& node) override;

    // control flow
    virtual void visit(const BlockNode& node) override;
    virtual void visit(const IfNode& node) override;
    virtual void visit(const WhileNode& node) override;

    // functions
    virtual void visit(const FunctionNode& node) override;
    virtual void visit(const ReturnNode& node) override;
    virtual void visit(const CallNode& node) override;

    // print
    virtual void visit(const PrintNode& node) override;
   

private:
    VarType m_currentType; // to keep track of the current expression type during analysis
    VarType m_currentFunctionReturnType = VarType::VOID; // to track the return type of the current function being analyzed
    bool m_foundReturn = false; // to track if a return statement was found in a non-void function
    bool m_insideFunction = false;
    SymbolTable m_symbolTable;
    FunctionsTable m_functionsTable;
    // scope management
    void enterScope();
    void exitScope();

    // symbol table operations
    bool declareVariable(const std::string& name, VarType type, bool mutableFlag = false);

    VarType stringToVarType(const std::string& typeStr);
    std::optional<std::reference_wrapper<VariableInfo>> 
    lookupVariable(const std::string& name);
};

#endif