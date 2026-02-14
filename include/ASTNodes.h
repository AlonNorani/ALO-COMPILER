#ifndef ASTNODES_H
#define ASTNODES_H

#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include "Token.h"
// Converts TokenType to readable string
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
        default: return "UnknownOp";
    }
}

// Base struct for all AST nodes
struct ASTNode {
    virtual ~ASTNode() = default;

    virtual int evaluate() const { return 0; }

    virtual void print(int indent = 0) const = 0;
};

struct NumberNode : ASTNode {
    int value;

    NumberNode(int val) : value(val) {}

    int evaluate() const override { return value; }

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Number: " << value << "\n";
    }
};

// Binary operation
struct BinaryOpNode : ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    TokenType op; // operator type

    BinaryOpNode(std::unique_ptr<ASTNode> l,
                 std::unique_ptr<ASTNode> r,
                 TokenType o)
        : left(std::move(l)), right(std::move(r)), op(o) {}

    int evaluate() const override {
        int lval = left->evaluate();
        int rval = right->evaluate();
        switch(op) {
            case TokenType::PLUS: return lval + rval;
            case TokenType::MINUS: return lval - rval;
            case TokenType::STAR: return lval * rval;
            case TokenType::SLASH: return lval / rval; // assume no divide by zero
            case TokenType::DOUBLE_EQUAL: return lval == rval;
            case TokenType::NOT_EQUAL: return lval != rval;
            case TokenType::LESS: return lval < rval;
            case TokenType::LESS_EQUAL: return lval <= rval;
            case TokenType::GREATER: return lval > rval;
            case TokenType::GREATER_EQUAL: return lval >= rval;
            default: return 0;
        }
    }

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "BinaryOp: " << tokenTypeToString(op) << "\n";
        left->print(indent + 2);
        right->print(indent + 2);
    }
};
// Unary operation
class UnaryOpNode : public ASTNode {
public:
    TokenType op;
    std::unique_ptr<ASTNode> operand;

    UnaryOpNode(TokenType op,
                std::unique_ptr<ASTNode> operand)
        : op(op), operand(std::move(operand)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "UnaryOp: " << tokenTypeToString(op) << "\n";
        operand->print(indent + 2);
    }
};


// Variable node
struct VariableNode : ASTNode {
    std::string name;

    VariableNode(const std::string& n) : name(n) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Variable: " << name << "\n";
    }
};

// Assignment node
struct AssignmentNode : ASTNode {
    std::string variableName;
    std::unique_ptr<ASTNode> expression;

    AssignmentNode(const std::string& var, std::unique_ptr<ASTNode> expr)
        : variableName(var), expression(std::move(expr)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Assignment: " << variableName << "\n";
        expression->print(indent + 2);
    }
};

// block of statements node
struct BlockNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;

    BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts)
        : statements(std::move(stmts)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Block:\n";
        for (const auto& stmt : statements)
            stmt->print(indent + 2);
    }
};

// if statement node
struct IfNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<BlockNode> thenBranch;
    std::unique_ptr<BlockNode> elseBranch; // can be nullptr

    IfNode(std::unique_ptr<ASTNode> cond,
           std::unique_ptr<BlockNode> thenB,
           std::unique_ptr<BlockNode> elseB = nullptr)
        : condition(std::move(cond)),
          thenBranch(std::move(thenB)),
          elseBranch(std::move(elseB)) {}

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

// While loop
struct WhileNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<BlockNode> body;

    WhileNode(std::unique_ptr<ASTNode> cond,
              std::unique_ptr<BlockNode> b)
        : condition(std::move(cond)), body(std::move(b)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "While:\n";
        std::cout << std::string(indent + 2, ' ') << "Condition:\n";
        condition->print(indent + 4);
        std::cout << std::string(indent + 2, ' ') << "Body:\n";
        body->print(indent + 4);
    }
};

struct BoolNode : ASTNode {
    bool value;

    BoolNode(bool val) : value(val) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ')
                  << "Bool: " << (value ? "true" : "false") << "\n";
    }
};


#endif // ASTNODES_H
