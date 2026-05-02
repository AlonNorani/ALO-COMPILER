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
struct LetNode;
struct FunctionNode;
struct ReturnNode;
struct CallNode;
struct PrintNode;
struct StringNode;
struct FloatNode;
struct CastNode;
struct ArrayLiteralNode;
struct ArrayAccessNode;
struct ArrayAssignNode;
struct ForNode;
struct InputNode;
struct BorrowNode;
struct CopyNode;

struct ASTVisitor {
    virtual ~ASTVisitor() = default;

    virtual void visit(const NumberNode&     node) = 0;
    virtual void visit(const FloatNode&      node) = 0;
    virtual void visit(const BinaryOpNode&   node) = 0;
    virtual void visit(const UnaryOpNode&    node) = 0;
    virtual void visit(const VariableNode&   node) = 0;
    virtual void visit(const AssignmentNode& node) = 0;
    virtual void visit(const BlockNode&      node) = 0;
    virtual void visit(const IfNode&         node) = 0;
    virtual void visit(const WhileNode&      node) = 0;
    virtual void visit(const BoolNode&       node) = 0;
    virtual void visit(const LetNode&        node) = 0;
    virtual void visit(const FunctionNode&   node) = 0;
    virtual void visit(const ReturnNode&     node) = 0;
    virtual void visit(const CallNode&       node) = 0;
    virtual void visit(const PrintNode&      node) = 0;
    virtual void visit(const StringNode&     node) = 0;
    virtual void visit(const CastNode&       node) = 0;
    virtual void visit(const ArrayLiteralNode& node) = 0;
    virtual void visit(const ArrayAccessNode&  node) = 0;
    virtual void visit(const ArrayAssignNode&  node) = 0;
    virtual void visit(const ForNode&         node) = 0;
    virtual void visit(const InputNode& node) = 0;
    virtual void visit(const BorrowNode& node) = 0;
    virtual void visit(const CopyNode&   node) = 0;
};

#endif
