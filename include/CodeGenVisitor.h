#ifndef CODEGENVISITOR_H
#define CODEGENVISITOR_H

#include "ASTVisitor.h"
#include "ASTNodes.h"
#include "CodeGenerator.h"
#include <llvm/IR/Value.h>
#include "SemanticAnalyzer.h"

class CodeGenVisitor : public ASTVisitor
{
public:
    explicit CodeGenVisitor(CodeGenerator& cg);
    void generate(const std::vector<std::unique_ptr<ASTNode>>& program);
    llvm::Value* getValue();

    void visit(const NumberNode&       node) override;
    void visit(const BoolNode&         node) override;
    void visit(const StringNode&       node) override;
    void visit(const VariableNode&     node) override;
    void visit(const BinaryOpNode&     node) override;
    void visit(const UnaryOpNode&      node) override;
    void visit(const LetNode&          node) override;
    void visit(const AssignmentNode&   node) override;
    void visit(const BlockNode&        node) override;
    void visit(const IfNode&           node) override;
    void visit(const WhileNode&        node) override;
    void visit(const FunctionNode&     node) override;
    void visit(const ReturnNode&       node) override;
    void visit(const CallNode&         node) override;
    void visit(const PrintNode&        node) override;
    void visit(const ArrayLiteralNode& node) override;
    void visit(const ArrayAccessNode&  node) override;
    void visit(const ArrayAssignNode&  node) override;

private:
    CodeGenerator& m_cg;
    llvm::Value*   m_lastValue;
    VarType        m_lastVarType;

    llvm::Value* toBool(llvm::Value* val);
    std::string  convertFormatString(const std::string&         input,
                                      const std::vector<VarType>& argTypes,
                                      bool                        newline);
    llvm::Type*  varTypeToLLVM(VarType vt);
    VarType      stringToVarType(const std::string& s);  // NEW
};

#endif
