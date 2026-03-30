#ifndef CODEGENVISITOR_H
#define CODEGENVISITOR_H

#include "ASTVisitor.h"
#include "ASTNodes.h"
#include "CodeGenerator.h"
#include <llvm/IR/Value.h>
#include <stack>
#include "SemanticAnalyzer.h"

class CodeGenVisitor : public ASTVisitor
{
public:
    CodeGenVisitor(CodeGenerator& cg);
    void generate(const std::vector<std::unique_ptr<ASTNode>>& program);
    llvm::Value* getValue();

    void visit(const NumberNode& node) override;
    void visit(const BoolNode& node) override;
    void visit(const StringNode& node) override;
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
    VarType m_lastVarType; // track the type of the last evaluated expression for print formatting
    llvm::Value* toBool(llvm::Value* val);

    // Helper to convert our format string with {} to printf format
    std::string convertFormatString(const std::string& input, const std::vector<VarType>& argTypes, bool newline);

};

#endif