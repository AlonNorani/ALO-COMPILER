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
    VOID,
    ARRAY,
};

struct VariableInfo {
    std::string name;
    VarType     type;
    bool        mutableFlag;
    // Array metadata (only meaningful when type == ARRAY)
    VarType     elementType = VarType::INT;
    int         arraySize   = -1;
};

// ─────────────────────────────────────────────
//  ParamInfo — richer than a bare VarType so we
//  can carry element-type info for array params
// ─────────────────────────────────────────────
struct ParamInfo {
    VarType varType    = VarType::INT;   // ARRAY or scalar type
    VarType elemType   = VarType::INT;   // only meaningful when varType==ARRAY
    bool    isArray    = false;
};

struct FunctionInfo
{
    std::vector<ParamInfo> params;       // replaces old vector<VarType>
    VarType                returnType;
};

using SymbolTable    = std::vector<std::unordered_map<std::string, VariableInfo>>;
using FunctionsTable = std::unordered_map<std::string, FunctionInfo>;

class SemanticAnalyzer : public ASTVisitor
{
public:
    SemanticAnalyzer();
    ~SemanticAnalyzer();

    void analyze(const std::vector<std::unique_ptr<ASTNode>>& ast);

    virtual void visit(const NumberNode&       node) override;
    virtual void visit(const BoolNode&         node) override;
    virtual void visit(const StringNode&       node) override;
    virtual void visit(const BinaryOpNode&     node) override;
    virtual void visit(const UnaryOpNode&      node) override;
    virtual void visit(const VariableNode&     node) override;
    virtual void visit(const LetNode&          node) override;
    virtual void visit(const AssignmentNode&   node) override;
    virtual void visit(const BlockNode&        node) override;
    virtual void visit(const IfNode&           node) override;
    virtual void visit(const WhileNode&        node) override;
    virtual void visit(const FunctionNode&     node) override;
    virtual void visit(const ReturnNode&       node) override;
    virtual void visit(const CallNode&         node) override;
    virtual void visit(const PrintNode&        node) override;
    virtual void visit(const ArrayLiteralNode& node) override;
    virtual void visit(const ArrayAccessNode&  node) override;
    virtual void visit(const ArrayAssignNode&  node) override;

    VarType getCurrentElementType() const { return m_currentElementType; }

private:
    VarType m_currentType               = VarType::INT;
    VarType m_currentElementType        = VarType::INT;
    VarType m_currentFunctionReturnType = VarType::VOID;
    bool    m_foundReturn               = false;
    bool    m_insideFunction            = false;

    SymbolTable    m_symbolTable;
    FunctionsTable m_functionsTable;

    void    enterScope();
    void    exitScope();
    bool    declareVariable(const std::string& name,
                            VarType            type,
                            bool               mutableFlag  = false,
                            VarType            elementType  = VarType::INT,
                            int                arraySize    = -1);
    VarType stringToVarType(const std::string& typeStr);
    std::optional<std::reference_wrapper<VariableInfo>>
            lookupVariable(const std::string& name);
};

#endif
