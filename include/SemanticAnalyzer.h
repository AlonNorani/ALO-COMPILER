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

/**
 * @file SemanticAnalyzer.h
 * @brief Defines the SemanticAnalyzer class that performs semantic analysis on the AST.
 *
 * The semantic analyzer checks for type correctness, variable scoping, function calls,
 * and ownership/borrowing rules. It ensures the program is semantically valid before
 * code generation.
 */

/**
 * @enum VarType
 * @brief Represents the different types that variables can have in ALO.
 */
enum class VarType {
    INT,     ///< 32-bit integer
    FLOAT,   ///< 64-bit floating point
    STRING,  ///< Heap-allocated string
    BOOL,    ///< Boolean value
    VOID,    ///< Void type (for functions)
    ARRAY,   ///< Heap-allocated array
};

/**
 * @enum OwnershipState
 * @brief Tracks the ownership state of heap-allocated variables.
 *
 * Only applies to heap types (STRING, ARRAY). Stack types are always copied.
 */
enum class OwnershipState {
    OWNED,           ///< Variable owns the value and can be used normally
    BORROWED_IMM,    ///< Variable is immutably borrowed (read-only reference)
    BORROWED_MUT,    ///< Variable is mutably borrowed (exclusive write reference)
    MOVED,           ///< Variable has been moved and cannot be used
};

/**
 * @struct BorrowInfo
 * @brief Information about an active borrow of a variable.
 */
struct BorrowInfo {
    OwnershipState state;       ///< Type of borrow (immutable or mutable)
    std::string borrowedBy;     ///< Name of the variable/reference doing the borrowing
};

/**
 * @struct VariableInfo
 * @brief Complete information about a variable in the symbol table.
 */
struct VariableInfo {
    std::string name;           ///< Variable name
    VarType     type;           ///< Variable type
    bool        mutableFlag;    ///< Whether the variable can be reassigned
    VarType     elementType = VarType::INT;  ///< For arrays: type of elements
    int         arraySize   = -1;             ///< For arrays: size (-1 for dynamic)

    // Ownership system: only used for heap types (STRING, ARRAY)
    OwnershipState ownershipState = OwnershipState::OWNED;  ///< Current ownership state
    std::vector<BorrowInfo> activeBorrows;  ///< List of active borrows

    /**
     * @brief Check if this variable has a heap-allocated type.
     * @return true if the variable is a string or array
     */
    bool isHeapType() const {
        return type == VarType::STRING || type == VarType::ARRAY;
    }
};

/**
 * @struct ParamInfo
 * @brief Information about a function parameter.
 */
struct ParamInfo {
    VarType varType    = VarType::INT;   ///< Parameter type
    VarType elemType   = VarType::INT;   ///< Element type (for arrays)
    bool    isArray    = false;          ///< Whether this is an array parameter
    std::string borrowType = "";         ///< Borrow type: ""=owned, "imm"=&T, "mut"=&mut T
};

/**
 * @struct FunctionInfo
 * @brief Information about a function definition.
 */
struct FunctionInfo
{
    std::vector<ParamInfo> params;       ///< Function parameters
    VarType                returnType;   ///< Return type
};

/** Symbol table type: stack of scopes, each scope is a map of name -> variable info */
using SymbolTable    = std::vector<std::unordered_map<std::string, VariableInfo>>;
/** Functions table: map of function name -> function info */
using FunctionsTable = std::unordered_map<std::string, FunctionInfo>;

/**
 * @class SemanticAnalyzer
 * @brief Performs semantic analysis on the AST including type checking and ownership validation.
 *
 * The semantic analyzer traverses the AST and:
 * - Checks type correctness of expressions and assignments
 * - Validates variable scoping and declarations
 * - Ensures proper ownership and borrowing rules for heap types
 * - Verifies function calls match their declarations
 * - Prevents use-after-move and invalid borrowing patterns
 */
class SemanticAnalyzer : public ASTVisitor
{
public:
    SemanticAnalyzer();
    ~SemanticAnalyzer();

    /**
     * @brief Analyze the entire AST for semantic correctness.
     * @param ast The abstract syntax tree to analyze
     */
    void analyze(const std::vector<std::unique_ptr<ASTNode>>& ast);

    // AST visitor methods (implementations in .cpp file)
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
    virtual void visit(const BorrowNode&       node) override;
    virtual void visit(const CopyNode&         node) override;

    /**
     * @brief Get the current element type being processed.
     * @return The current element type
     */
    VarType getCurrentElementType() const { return m_currentElementType; }

private:
    VarType m_currentType               = VarType::INT;   ///< Current expression type
    VarType m_currentElementType        = VarType::INT;   ///< Current array element type
    VarType m_currentFunctionReturnType = VarType::VOID;  ///< Current function's return type
    bool    m_foundReturn               = false;           ///< Whether a return was found in current function
    bool    m_insideFunction            = false;           ///< Whether we're inside a function

    SymbolTable    m_symbolTable;     ///< Stack of symbol tables (one per scope)
    FunctionsTable m_functionsTable;  ///< Global function definitions

    // Scope management
    void enterScope();  ///< Enter a new scope
    void exitScope();   ///< Exit current scope

    // Variable management
    bool declareVariable(const std::string& name,
                        VarType            type,
                        bool               mutableFlag  = false,
                        VarType            elementType  = VarType::INT,
                        int                arraySize    = -1);

    // Type conversion utilities
    VarType stringToVarType(const std::string& typeStr);
    std::string varTypeToString(VarType vt);

    // Variable lookup
    std::optional<std::reference_wrapper<VariableInfo>>
        lookupVariable(const std::string& name);

    // Ownership system validation
    void validateNotMoved(const std::string& varName, int line, int col);  ///< Ensure variable wasn't moved
    void validateCanBorrow(const std::string& varName, OwnershipState requestedBorrow, int line, int col);  ///< Check borrowing rules
    void validateCanMutate(const std::string& varName, int line, int col);  ///< Check if variable can be mutated
    void markAsMoved(const std::string& varName);        ///< Mark variable as moved
    void markAsBorrowed(const std::string& varName, OwnershipState borrowType);  ///< Record a borrow
    void clearBorrows(const std::string& varName);       ///< Clear all borrows of a variable
    bool isHeapType(VarType type) const;                 ///< Check if type is heap-allocated
};

#endif
