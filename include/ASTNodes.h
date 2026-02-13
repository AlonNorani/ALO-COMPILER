#ifndef ASTNODES_H
#define ASTNODES_H

#include <memory>
#include <string>
#include <iostream>

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
    char op; // '+', '-', '*', '/'

    BinaryOpNode(std::unique_ptr<ASTNode> l,
                 std::unique_ptr<ASTNode> r,
                 char o)
        : left(std::move(l)), right(std::move(r)), op(o) {}

    int evaluate() const override {
        int lval = left->evaluate();
        int rval = right->evaluate();
        switch(op) {
            case '+': return lval + rval;
            case '-': return lval - rval;
            case '*': return lval * rval;
            case '/': return lval / rval; // assume no divide by zero
            default: return 0;
        }
    }

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "BinaryOp: " << op << "\n";
        left->print(indent + 2);
        right->print(indent + 2);
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

#endif // ASTNODES_H
