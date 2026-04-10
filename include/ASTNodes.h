#ifndef ASTNODES_H
#define ASTNODES_H

#include "ASTVisitor.h"
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>
#include "Token.h"

// Defines the Abstract Syntax Tree (AST) node structures.
// Each node represents a construct in the source code, used for semantic analysis and code generation.
struct ASTVisitor;

struct Parameter
{
    std::string type;           
    std::string name;
    bool        isArray = false; 
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

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) const = 0;
    virtual int  evaluate() const {
        throw std::runtime_error("Evaluate not implemented for this node type");
    }
    virtual void print(int indent = 0) const = 0;
};

#define ACCEPT_VISITOR \
    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }

//  Literals

struct NumberNode : ASTNode {
    int value;
    NumberNode(int val) : value(val) {}
    ACCEPT_VISITOR
    int  evaluate() const override { return value; }
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Number: " << value << "\n";
    }
};

struct BoolNode : ASTNode {
    bool value;
    BoolNode(bool val) : value(val) {}
    ACCEPT_VISITOR
    int  evaluate() const override { return value ? 1 : 0; }
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Bool: " << (value ? "true" : "false") << "\n";
    }
};

struct FloatNode : ASTNode {
    double value;
    FloatNode(double val) : value(val) {}
    ACCEPT_VISITOR
    int  evaluate() const override { return static_cast<int>(value); }
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Float: " << value << "\n";
    }
};

struct StringNode : ASTNode {
    std::string value;
    StringNode(std::string val) : value(std::move(val)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "String: \"" << value << "\"\n";
    }
};

struct CastNode : ASTNode {
    std::string targetType;
    std::unique_ptr<ASTNode> expression;
    CastNode(std::string type, std::unique_ptr<ASTNode> expr)
        : targetType(std::move(type)), expression(std::move(expr)) {}
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
                 TokenType o)
        : left(std::move(l)), right(std::move(r)), op(o) {}
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
    UnaryOpNode(TokenType op, std::unique_ptr<ASTNode> operand)
        : op(op), operand(std::move(operand)) {}
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

//  Variables

struct VariableNode : ASTNode {
    std::string name;
    VariableNode(const std::string& n) : name(n) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Variable: " << name << "\n";
    }
};

struct LetNode : ASTNode {
    std::string              variableName;
    std::unique_ptr<ASTNode> value;
    bool                     isMutable;

    bool        hasTypeAnnotation = false;
    std::string annotatedType;
    bool        isArray           = false;
    int         arraySize         = -1;

    LetNode(const std::string& var,
            std::unique_ptr<ASTNode> val,
            bool mutableFlag = false)
        : variableName(var), value(std::move(val)), isMutable(mutableFlag) {}

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
    AssignmentNode(const std::string& var, std::unique_ptr<ASTNode> expr)
        : variableName(var), expression(std::move(expr)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Assignment: " << variableName << "\n";
        expression->print(indent + 2);
    }
};

struct BlockNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
    BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts)
        : statements(std::move(stmts)) {}
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
           std::unique_ptr<BlockNode> elseB = nullptr)
        : condition(std::move(cond)),
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
    WhileNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<BlockNode> b)
        : condition(std::move(cond)), body(std::move(b)) {}
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
            std::unique_ptr<BlockNode> b)
        : iteratorName(name),
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
                 std::unique_ptr<BlockNode> b)
        : name(n), parameters(std::move(params)), returnType(retType), body(std::move(b)) {}
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
    ReturnNode(std::unique_ptr<ASTNode> expr) : expression(std::move(expr)) {}
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
    CallNode(const std::string& name, std::vector<std::unique_ptr<ASTNode>> args)
        : functionName(name), arguments(std::move(args)) {}
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
              bool nl)
        : formatString(std::move(fmt)), args(std::move(arguments)), newline(nl) {}
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
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Input!\n";
    }
};

//  Array nodes

struct ArrayLiteralNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> elements;
    ArrayLiteralNode(std::vector<std::unique_ptr<ASTNode>> elems)
        : elements(std::move(elems)) {}
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
    ArrayAccessNode(const std::string& name, std::unique_ptr<ASTNode> idx)
        : name(name), index(std::move(idx)) {}
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
                    std::unique_ptr<ASTNode> val)
        : name(name), index(std::move(idx)), value(std::move(val)) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "ArrayAssign: " << name << "[\n";
        index->print(indent + 2);
        std::cout << std::string(indent, ' ') << "] =\n";
        value->print(indent + 2);
    }
};

#endif
