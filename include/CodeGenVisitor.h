#ifndef CODEGENVISITOR_H
#define CODEGENVISITOR_H

#include "ASTVisitor.h"
#include "ASTNodes.h"
#include "CodeGenerator.h"
#include <llvm/IR/Value.h>
#include <stack>

class CodeGenVisitor : public ASTVisitor
{
public:
    CodeGenVisitor(CodeGenerator& cg);
    void generate(const std::vector<std::unique_ptr<ASTNode>>& program);
    llvm::Value* getValue();

    void visit(const NumberNode& node) override;
    void visit(const BoolNode& node) override;
    void visit(const VariableNode& node) override;
    void visit(const BinaryOpNode& node) override;
    void visit(const UnaryOpNode& node) override;
    void visit(const LetNode& node) override;
    void visit(const AssignmentNode& node) override;
    void visit(const BlockNode& node) override;
    void visit(const IfNode& node) override;
    void visit(const WhileNode& node) override;
    void visit(const FunctionNode& node) override;
    void visit(const ReturnNode& node) override;
    void visit(const CallNode& node) override;
    void visit(const PrintNode& node) override;

private:
    CodeGenerator& m_cg;
    llvm::Value* m_lastValue;
    llvm::Value* toBool(llvm::Value* val);
};

#endif