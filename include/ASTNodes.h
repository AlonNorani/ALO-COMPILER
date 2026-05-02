#ifndef ASTNODES_H
#define ASTNODES_H

#include "ASTVisitor.h"
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>
#include "Token.h"

/**
 * @file ASTNodes.h
 * @brief Defines the Abstract Syntax Tree (AST) node structures for the ALO compiler.
 *
 * This file contains all the AST node classes that represent the syntactic structure
 * of ALO programs. Each node type corresponds to a language construct like expressions,
 * statements, declarations, etc. The AST is built by the parser and used by the
 * semantic analyzer and code generator.
 */

// Forward declaration for visitor pattern
struct ASTVisitor;

/**
 * @struct Parameter
 * @brief Represents a function parameter with type and ownership information.
 *
 * Parameters can be owned (moved), immutable borrows (&T), or mutable borrows (&mut T).
 */
struct Parameter
{
    std::string type;           ///< The base type (int, float, string, etc.)
    std::string name;           ///< The parameter name
    bool        isArray = false; ///< Whether this is an array type

    // Borrow type for ownership system:
    // "" or "owned" = move (T)
    // "imm" or "&" = immutable borrow (&T)
    // "mut" or "&mut" = mutable borrow (&mut T)
    std::string borrowType = "";  // "" means owned/moved, "imm" means &T, "mut" means &mut T
};

inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::PLUS:          return "+";
        case TokenType::MINUS:         return "-";
        case TokenType::STAR:          return "*";
        case TokenType::SLASH:         return "/";
        case TokenType::GREATER:       return ">";
        case TokenType::LESS:          return "<";
        case TokenType::GREATER_EQUAL: return ">=";
        case TokenType::LESS_EQUAL:    return "<=";
        case TokenType::DOUBLE_EQUAL:  return "==";
        case TokenType::NOT_EQUAL:     return "!=";
        case TokenType::LOGICAL_AND:   return "&&";
        case TokenType::LOGICAL_OR:    return "||";
        case TokenType::NOT:           return "!";
        case TokenType::ARROW:         return "->";
        default:                       return "UnknownOp";
    }
}

/**
 * @struct ASTNode
 * @brief Base class for all AST nodes in the syntax tree.
 *
 * Every node in the AST inherits from this class and implements the visitor pattern
 * for traversal during semantic analysis and code generation.
 */
struct ASTNode {
    int line, column;  ///< Position in source code for error reporting
    ASTNode(int l = 0, int c = 0) : line(l), column(c) {}
    virtual ~ASTNode() = default;

    /// Accept a visitor for traversal (Visitor pattern)
    virtual void accept(ASTVisitor& visitor) const = 0;

    /// Evaluate the node as an integer (for constant folding)
    virtual int  evaluate() const {
        throw std::runtime_error("Evaluate not implemented for this node type");
    }

    /// Print the AST node for debugging
    virtual void print(int indent = 0) const = 0;
};

#define ACCEPT_VISITOR \
    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }

// ================================
// Literal Value Nodes
// ================================

/**
 * @struct NumberNode
 * @brief Represents an integer literal in the source code.
 */
struct NumberNode : ASTNode {
    int value;  ///< The integer value
    NumberNode(int val, int l, int c) : ASTNode(l, c), value(val) {}
    ACCEPT_VISITOR
    int  evaluate() const override { return value; }
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Number: " << value << "\n";
    }
};

/**
 * @struct BoolNode
 * @brief Represents a boolean literal (true/false) in the source code.
 */
struct BoolNode : ASTNode {
    bool value;  ///< The boolean value
    BoolNode(bool val, int l, int c) : ASTNode(l, c), value(val) {}
    ACCEPT_VISITOR
    int  evaluate() const override { return value ? 1 : 0; }
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Bool: " << (value ? "true" : "false") << "\n";
    }
};

/**
 * @struct FloatNode
 * @brief Represents a floating-point literal in the source code.
 */
struct FloatNode : ASTNode {
    double value;  ///< The floating-point value
    FloatNode(double val, int l, int c) : ASTNode(l, c), value(val) {}
    ACCEPT_VISITOR
    int  evaluate() const override { return static_cast<int>(value); }
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Float: " << value << "\n";
    }
};

/**
 * @struct StringNode
 * @brief Represents a string literal in the source code.
 */
struct StringNode : ASTNode {
    std::string value;  ///< The string value
    StringNode(std::string val, int l, int c) : ASTNode(l, c), value(std::move(val)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "String: \"" << value << "\"\n";
    }
};

struct CastNode : ASTNode {
    std::string targetType;
    std::unique_ptr<ASTNode> expression;
    CastNode(std::string type, std::unique_ptr<ASTNode> expr, int l, int c)
        : ASTNode(l, c), targetType(std::move(type)), expression(std::move(expr)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Cast: (" << targetType << ")\n";
        expression->print(indent + 2);
    }
};

//  Expressions

struct BinaryOpNode : ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    TokenType op;
    BinaryOpNode(std::unique_ptr<ASTNode> l,
                 std::unique_ptr<ASTNode> r,
                 TokenType o, int lline, int c)
        : ASTNode(lline, c), left(std::move(l)), right(std::move(r)), op(o) {}
    ACCEPT_VISITOR
    int evaluate() const override {
        int l = left->evaluate(), r = right->evaluate();
        switch (op) {
            case TokenType::PLUS:          return l + r;
            case TokenType::MINUS:         return l - r;
            case TokenType::STAR:          return l * r;
            case TokenType::SLASH:         return l / r;
            case TokenType::DOUBLE_EQUAL:  return l == r;
            case TokenType::NOT_EQUAL:     return l != r;
            case TokenType::LESS:          return l < r;
            case TokenType::LESS_EQUAL:    return l <= r;
            case TokenType::GREATER:       return l > r;
            case TokenType::GREATER_EQUAL: return l >= r;
            case TokenType::LOGICAL_AND:   return l && r;
            case TokenType::LOGICAL_OR:    return l || r;
            default: throw std::runtime_error("Unsupported binary operator");
        }
    }
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "BinaryOp: " << tokenTypeToString(op) << "\n";
        left->print(indent + 2);
        right->print(indent + 2);
    }
};

struct UnaryOpNode : ASTNode {
    TokenType op;
    std::unique_ptr<ASTNode> operand;
    UnaryOpNode(TokenType o, std::unique_ptr<ASTNode> operand, int l, int c)
        : ASTNode(l, c), op(o), operand(std::move(operand)) {}
    ACCEPT_VISITOR
    int evaluate() const override {
        int val = operand->evaluate();
        switch (op) {
            case TokenType::MINUS: return -val;
            case TokenType::NOT:   return !val;
            default: throw std::runtime_error("Unsupported unary operator");
        }
    }
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "UnaryOp: " << tokenTypeToString(op) << "\n";
        operand->print(indent + 2);
    }
};

// ================================
// Variable and Declaration Nodes
// ================================

/**
 * @struct VariableNode
 * @brief Represents a variable reference in expressions.
 */
struct VariableNode : ASTNode {
    std::string name;  ///< The variable name
    VariableNode(const std::string& n, int l, int c) : ASTNode(l, c), name(n) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Variable: " << name << "\n";
    }
};

/**
 * @struct LetNode
 * @brief Represents a variable declaration (let statement).
 *
 * Handles both simple declarations and array declarations with optional type annotations.
 */
struct LetNode : ASTNode {
    std::string              variableName;    ///< Name of the variable being declared
    std::unique_ptr<ASTNode> value;           ///< Initial value expression
    bool                     isMutable;       ///< Whether the variable can be reassigned

    // Type annotation information
    bool        hasTypeAnnotation = false;   ///< Whether a type was explicitly specified
    std::string annotatedType;               ///< The annotated type name
    bool        isArray           = false;    ///< Whether this is an array type
    int         arraySize         = -1;       ///< Array size (-1 for dynamic size)

    LetNode(const std::string& var,
            std::unique_ptr<ASTNode> val,
            bool mutableFlag = false,
            int l = 0,
            int c = 0)
        : ASTNode(l, c), variableName(var), value(std::move(val)), isMutable(mutableFlag) {}

    ACCEPT_VISITOR

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Let: " << variableName
                  << (isMutable ? " (mutable)" : " (immutable)");
        if (hasTypeAnnotation) {
            std::cout << " : " << annotatedType;
            if (isArray)
                std::cout << "[" << (arraySize >= 0 ? std::to_string(arraySize) : "") << "]";
        }
        std::cout << "\n";
        if (value) {
            std::cout << std::string(indent + 2, ' ') << "Value:\n";
            value->print(indent + 4);
        }
    }

    int evaluate() const override { return value ? value->evaluate() : 0; }
};

struct AssignmentNode : ASTNode {
    std::string              variableName;
    std::unique_ptr<ASTNode> expression;
    AssignmentNode(const std::string& var, std::unique_ptr<ASTNode> expr, int l, int c)
        : ASTNode(l, c), variableName(var), expression(std::move(expr)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Assignment: " << variableName << "\n";
        expression->print(indent + 2);
    }
};

struct BlockNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
    BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts, int l, int c)
        : ASTNode(l, c), statements(std::move(stmts)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Block:\n";
        for (const auto& stmt : statements) stmt->print(indent + 2);
    }
};

struct IfNode : ASTNode {
    std::unique_ptr<ASTNode>   condition;
    std::unique_ptr<BlockNode> thenBranch;
    std::unique_ptr<BlockNode> elseBranch;
    IfNode(std::unique_ptr<ASTNode>   cond,
           std::unique_ptr<BlockNode> thenB,
           std::unique_ptr<BlockNode> elseB = nullptr,
           int l = 0,
           int c = 0)
        : ASTNode(l, c), condition(std::move(cond)),
          thenBranch(std::move(thenB)),
          elseBranch(std::move(elseB)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "If:\n";
        std::cout << std::string(indent + 2, ' ') << "Condition:\n";
        condition->print(indent + 4);
        std::cout << std::string(indent + 2, ' ') << "Then:\n";
        thenBranch->print(indent + 4);
        if (elseBranch) {
            std::cout << std::string(indent + 2, ' ') << "Else:\n";
            elseBranch->print(indent + 4);
        }
    }
};

struct WhileNode : ASTNode {
    std::unique_ptr<ASTNode>   condition;
    std::unique_ptr<BlockNode> body;
    WhileNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<BlockNode> b, int l, int c)
        : ASTNode(l, c), condition(std::move(cond)), body(std::move(b)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "While:\n";
        std::cout << std::string(indent + 2, ' ') << "Condition:\n";
        condition->print(indent + 4);
        std::cout << std::string(indent + 2, ' ') << "Body:\n";
        body->print(indent + 4);
    }
};

struct ForNode : ASTNode {
    std::string               iteratorName;
    std::unique_ptr<ASTNode>  iterable;
    std::unique_ptr<ASTNode>  endExpr;
    std::unique_ptr<BlockNode> body;

    ForNode(const std::string& name,
            std::unique_ptr<ASTNode> iterableExpr,
            std::unique_ptr<ASTNode> endExpression,
            std::unique_ptr<BlockNode> b,
            int l,
            int c)
        : ASTNode(l, c), iteratorName(name),
          iterable(std::move(iterableExpr)),
          endExpr(std::move(endExpression)),
          body(std::move(b)) {}

    bool isRange() const { return endExpr != nullptr; }

    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "For:\n";
        std::cout << std::string(indent + 2, ' ') << "Iterator: " << iteratorName << "\n";
        std::cout << std::string(indent + 2, ' ') << "Iterable:\n";
        iterable->print(indent + 4);
        if (isRange()) {
            std::cout << std::string(indent + 2, ' ') << "End:\n";
            endExpr->print(indent + 4);
        }
        std::cout << std::string(indent + 2, ' ') << "Body:\n";
        body->print(indent + 4);
    }
};

struct FunctionNode : ASTNode {
    std::string            name;
    std::vector<Parameter> parameters;
    std::string            returnType;
    std::unique_ptr<BlockNode> body;
    FunctionNode(const std::string& n,
                 std::vector<Parameter> params,
                 const std::string& retType,
                 std::unique_ptr<BlockNode> b,
                 int l,
                 int c)
        : ASTNode(l, c), name(n), parameters(std::move(params)), returnType(retType), body(std::move(b)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Function: " << name << "\n";
        if (!parameters.empty()) {
            std::cout << std::string(indent + 2, ' ') << "Parameters:\n";
            for (const auto& p : parameters)
                std::cout << std::string(indent + 4, ' ')
                          << p.name << ": " << p.type
                          << (p.isArray ? "[]" : "") << "\n";
        }
        if (!returnType.empty())
            std::cout << std::string(indent + 2, ' ')
                      << "Return Type: " << returnType << "\n";
        if (body) {
            std::cout << std::string(indent + 2, ' ') << "Body:\n";
            body->print(indent + 4);
        }
    }
};

struct ReturnNode : ASTNode {
    std::unique_ptr<ASTNode> expression;
    ReturnNode(std::unique_ptr<ASTNode> expr, int l, int c) : ASTNode(l, c), expression(std::move(expr)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Return:\n";
        if (expression) expression->print(indent + 2);
    }
    int evaluate() const override { return expression ? expression->evaluate() : 0; }
};

struct CallNode : ASTNode {
    std::string functionName;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    CallNode(const std::string& name, std::vector<std::unique_ptr<ASTNode>> args, int l, int c)
        : ASTNode(l, c), functionName(name), arguments(std::move(args)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Call: " << functionName << "\n";
        if (!arguments.empty()) {
            std::cout << std::string(indent + 2, ' ') << "Arguments:\n";
            for (const auto& a : arguments) a->print(indent + 4);
        }
    }
    int evaluate() const override { return 0; }
};


// I/O
struct PrintNode : ASTNode {
    std::string formatString;
    std::vector<std::unique_ptr<ASTNode>> args;
    bool newline;
    PrintNode(std::string fmt,
              std::vector<std::unique_ptr<ASTNode>> arguments,
              bool nl,
              int l,
              int c)
        : ASTNode(l, c), formatString(std::move(fmt)), args(std::move(arguments)), newline(nl) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << (newline ? "Println!" : "Print!") << "\n";
        if (!args.empty()) {
            std::cout << std::string(indent + 2, ' ') << "Arguments:\n";
            for (const auto& a : args) a->print(indent + 4);
        }
    }
};

struct InputNode : ASTNode {
    InputNode(int l, int c) : ASTNode(l, c) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Input!\n";
    }
};

//  Array nodes

struct ArrayLiteralNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> elements;
    ArrayLiteralNode(std::vector<std::unique_ptr<ASTNode>> elems, int l, int c)
        : ASTNode(l, c), elements(std::move(elems)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "ArrayLiteral[\n";
        for (const auto& e : elements) e->print(indent + 2);
        std::cout << std::string(indent, ' ') << "]\n";
    }
};

struct ArrayAccessNode : ASTNode {
    std::string              name;
    std::unique_ptr<ASTNode> index;
    ArrayAccessNode(const std::string& name, std::unique_ptr<ASTNode> idx, int l, int c)
        : ASTNode(l, c), name(name), index(std::move(idx)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "ArrayAccess: " << name << "[\n";
        index->print(indent + 2);
        std::cout << std::string(indent, ' ') << "]\n";
    }
};

struct ArrayAssignNode : ASTNode {
    std::string              name;
    std::unique_ptr<ASTNode> index;
    std::unique_ptr<ASTNode> value;
    ArrayAssignNode(const std::string& name,
                    std::unique_ptr<ASTNode> idx,
                    std::unique_ptr<ASTNode> val,
                    int l,
                    int c)
        : ASTNode(l, c), name(name), index(std::move(idx)), value(std::move(val)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "ArrayAssign: " << name << "[\n";
        index->print(indent + 2);
        std::cout << std::string(indent, ' ') << "] =\n";
        value->print(indent + 2);
    }
};

// Borrow expression: &a (immutable) or &mut a (mutable)
struct BorrowNode : ASTNode {
    std::string name;
    bool isMutable; // false = &a, true = &mut a
    BorrowNode(const std::string& n, bool mut, int l, int c)
        : ASTNode(l, c), name(n), isMutable(mut) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << (isMutable ? "&mut " : "&") << name << "\n";
    }
};

// Deep copy: copy(a)
struct CopyNode : ASTNode {
    std::string name;
    CopyNode(const std::string& n, int l, int c) : ASTNode(l, c), name(n) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "copy(" << name << ")\n";
    }
};

#endif
