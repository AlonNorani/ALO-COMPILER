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

// Semantic Analyzer: Performs type checking, scoping, and semantic validation on the AST.
// Ensures the program is semantically correct before code generation.
enum class VarType {
    INT,
    FLOAT,
    STRING,
    BOOL,
    VOID,
    ARRAY,
};

// Ownership state: tracks whether a variable is owned, borrowed immutably, or borrowed mutably
enum class OwnershipState {
    OWNED,           // Variable owns the value (normal state)
    BORROWED_IMM,    // Variable is immutably borrowed (read-only reference)
    BORROWED_MUT,    // Variable is mutably borrowed (exclusive write reference)
    MOVED,           // Variable has been moved (cannot be used)
};

// Tracks who borrowed a variable
struct BorrowInfo {
    OwnershipState state;
    std::string borrowedBy;  // Name of the variable/reference that borrowed this
};

struct VariableInfo {
    std::string name;
    VarType     type;
    bool        mutableFlag;
    VarType     elementType = VarType::INT;
    int         arraySize   = -1;
    
    // Ownership system: only heap types (STRING, ARRAY) use this
    // Stack types (INT, FLOAT, BOOL) are always copied
    OwnershipState ownershipState = OwnershipState::OWNED;
    std::vector<BorrowInfo> activeBorrows;  // Active references to this variable
    
    bool isHeapType() const {
        return type == VarType::STRING || type == VarType::ARRAY;
    }
};


struct ParamInfo {
    VarType varType    = VarType::INT;   
    VarType elemType   = VarType::INT;   
    bool    isArray    = false;
};

struct FunctionInfo
{
    std::vector<ParamInfo> params;       
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
    virtual void visit(const FloatNode&        node) override;
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
    virtual void visit(const CastNode&         node) override;
    virtual void visit(const ArrayLiteralNode& node) override;
    virtual void visit(const ArrayAccessNode&  node) override;
    virtual void visit(const ArrayAssignNode&  node) override;
    virtual void visit(const ForNode&          node) override;
    virtual void visit(const InputNode&        node) override;


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
    std::string varTypeToString(VarType vt);
    std::optional<std::reference_wrapper<VariableInfo>>
            lookupVariable(const std::string& name);
    
    // Ownership system helpers
    void    validateNotMoved(const std::string& varName, int line, int col);
    void    validateCanBorrow(const std::string& varName, OwnershipState requestedBorrow, int line, int col);
    void    validateCanMutate(const std::string& varName, int line, int col);
    void    markAsMoved(const std::string& varName);
    void    markAsBorrowed(const std::string& varName, OwnershipState borrowType);
    void    clearBorrows(const std::string& varName);
    bool    isHeapType(VarType type) const;
};

#endif
