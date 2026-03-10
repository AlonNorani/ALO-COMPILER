#ifndef ASTNODES_H
#define ASTNODES_H

#include "ASTVisitor.h"
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>
#include "Token.h"

struct ASTVisitor;
struct Parameter
{
    std::string type;
    std::string name;
};
// Convert TokenType to readable operator string
inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::PLUS: return "+";
        case TokenType::MINUS: return "-";
        case TokenType::STAR: return "*";
        case TokenType::SLASH: return "/";
        case TokenType::GREATER: return ">";
        case TokenType::LESS: return "<";
        case TokenType::GREATER_EQUAL: return ">=";
        case TokenType::LESS_EQUAL: return "<=";
        case TokenType::DOUBLE_EQUAL: return "==";
        case TokenType::NOT_EQUAL: return "!=";
        case TokenType::LOGICAL_AND: return "&&";
        case TokenType::LOGICAL_OR: return "||";
        case TokenType::NOT: return "!";
        case TokenType::ARROW: return "->";
        default: return "UnknownOp";
    }
}

struct ASTNode {
    virtual ~ASTNode() = default;

    virtual void accept(ASTVisitor& visitor) const = 0;

    // Used only for testing expressions
    virtual int evaluate() const {
        throw std::runtime_error("Evaluate not implemented for this node type");
    }

    virtual void print(int indent = 0) const = 0;
};

// Macro to avoid repeating accept() everywhere
#define ACCEPT_VISITOR \
void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }

// literals
struct NumberNode : ASTNode {
    int value;

    NumberNode(int val) : value(val) {}

    ACCEPT_VISITOR

    int evaluate() const override {
        return value;
    }

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Number: " << value << "\n";
    }
};

struct BoolNode : ASTNode {
    bool value;

    BoolNode(bool val) : value(val) {}

    ACCEPT_VISITOR

    int evaluate() const override {
        return value ? 1 : 0;
    }

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Bool: " << (value ? "true" : "false") << "\n";
    }
};

// expressions

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
        int l = left->evaluate();
        int r = right->evaluate();

        switch(op) {
            case TokenType::PLUS: return l + r;
            case TokenType::MINUS: return l - r;
            case TokenType::STAR: return l * r;
            case TokenType::SLASH: return l / r;

            case TokenType::DOUBLE_EQUAL: return l == r;
            case TokenType::NOT_EQUAL: return l != r;

            case TokenType::LESS: return l < r;
            case TokenType::LESS_EQUAL: return l <= r;

            case TokenType::GREATER: return l > r;
            case TokenType::GREATER_EQUAL: return l >= r;

            case TokenType::LOGICAL_AND: return l && r;
            case TokenType::LOGICAL_OR: return l || r;

            default:
                throw std::runtime_error("Unsupported binary operator");
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

    UnaryOpNode(TokenType op,
                std::unique_ptr<ASTNode> operand)
        : op(op), operand(std::move(operand)) {}

    ACCEPT_VISITOR

    int evaluate() const override {
        int val = operand->evaluate();

        switch(op) {
            case TokenType::MINUS: return -val;
            case TokenType::NOT: return !val;
            default:
                throw std::runtime_error("Unsupported unary operator");
        }
    }

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "UnaryOp: " << tokenTypeToString(op) << "\n";
        operand->print(indent + 2);
    }
};

// variables

struct VariableNode : ASTNode {
    std::string name;

    VariableNode(const std::string& n) : name(n) {}

    ACCEPT_VISITOR

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Variable: " << name << "\n";
    }
};

// assigmnments
struct LetNode : ASTNode {
    std::string variableName;
    std::unique_ptr<ASTNode> value;
    bool isMutable;
    LetNode(const std::string& var, std::unique_ptr<ASTNode> val, bool mutableFlag = false)
        : variableName(var), value(std::move(val)), isMutable(mutableFlag) {}
    ACCEPT_VISITOR
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Let: " << variableName 
                  << (isMutable ? " (mutable)" : " (immutable)") << "\n";
        if(value) {
            std::cout << std::string(indent + 2, ' ')
                      << "Value:\n";
            value->print(indent + 4);
        }
    }
};


struct AssignmentNode : ASTNode {
    std::string variableName;
    std::unique_ptr<ASTNode> expression;

    AssignmentNode(const std::string& var,
                   std::unique_ptr<ASTNode> expr)
        : variableName(var), expression(std::move(expr)) {}

    ACCEPT_VISITOR

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Assignment: " << variableName << "\n";
        expression->print(indent + 2);
    }
};

// blocks

struct BlockNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;

    BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts)
        : statements(std::move(stmts)) {}

    ACCEPT_VISITOR

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Block:\n";

        for (const auto& stmt : statements)
            stmt->print(indent + 2);
    }
};

// if

struct IfNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<BlockNode> thenBranch;
    std::unique_ptr<BlockNode> elseBranch;

    IfNode(std::unique_ptr<ASTNode> cond,
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

// while

struct WhileNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<BlockNode> body;

    WhileNode(std::unique_ptr<ASTNode> cond,
              std::unique_ptr<BlockNode> b)
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


// function definitions and calls 
struct FunctionNode : ASTNode {
    std::string name;
    std::vector<Parameter> parameters;
    std::string returnType;
    std::unique_ptr<BlockNode> body;

    FunctionNode(const std::string& n,
                 std::vector<Parameter> params,
                 const std::string& retType,
                 std::unique_ptr<BlockNode> b)
        : name(n), parameters(std::move(params)), returnType(retType), body(std::move(b)) {}

    ACCEPT_VISITOR

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Function: " << name << "\n";

        if (!parameters.empty()) {
            std::cout << std::string(indent + 2, ' ')
                      << "Parameters:\n";
            for (const auto& param : parameters) {
                std::cout << std::string(indent + 4, ' ')
                          << param.name << ": " << param.type << "\n";
            }
        }

        if (!returnType.empty()) {
            std::cout << std::string(indent + 2, ' ')
                      << "Return Type: " << returnType << "\n";
        }

        if (body) {
            std::cout << std::string(indent + 2, ' ')
                      << "Body:\n";
            body->print(indent + 4);
        }
    }
};

struct ReturnNode : ASTNode {
    std::unique_ptr<ASTNode> expression;

    ReturnNode(std::unique_ptr<ASTNode> expr)
        : expression(std::move(expr)) {}

    ACCEPT_VISITOR

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Return:\n";
        if (expression) {
            expression->print(indent + 2);
        }
    }
};
#endif