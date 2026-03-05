#ifndef ASTVISITOR_H
#define ASTVISITOR_H

struct NumberNode;
struct BinaryOpNode;
struct UnaryOpNode;
struct VariableNode;
struct AssignmentNode;
struct BlockNode;
struct IfNode;
struct WhileNode;
struct BoolNode;

struct ASTVisitor {
    virtual ~ASTVisitor() = default;

    virtual void visit(const NumberNode& node) = 0;
    virtual void visit(const BinaryOpNode& node) = 0;
    virtual void visit(const UnaryOpNode& node) = 0;
    virtual void visit(const VariableNode& node) = 0;
    virtual void visit(const AssignmentNode& node) = 0;
    virtual void visit(const BlockNode& node) = 0;
    virtual void visit(const IfNode& node) = 0;
    virtual void visit(const WhileNode& node) = 0;
    virtual void visit(const BoolNode& node) = 0;
};

#endif