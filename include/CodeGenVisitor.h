#ifndef CODEGENVISITOR_H
#define CODEGENVISITOR_H

#include "ASTVisitor.h"
#include "ASTNodes.h"
#include "CodeGenerator.h"
#include <llvm/IR/Value.h>
#include "SemanticAnalyzer.h"

// CodeGenVisitor: Traverses the AST and generates LLVM Intermediate Representation (IR).
// This is the core of code generation, translating high-level constructs into low-level IR instructions.
// Uses LLVM's IRBuilder to create instructions, manage memory, and handle control flow.
class CodeGenVisitor : public ASTVisitor
{
public:
    explicit CodeGenVisitor(CodeGenerator& cg);
    void generate(const std::vector<std::unique_ptr<ASTNode>>& program);
    llvm::Value* getValue();

    void visit(const NumberNode&       node) override;
    void visit(const FloatNode&        node) override;
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
    void visit(const CastNode&         node) override;
    void visit(const ArrayLiteralNode& node) override;
    void visit(const ArrayAccessNode&  node) override;
    void visit(const ArrayAssignNode&  node) override;
    void visit(const ForNode&          node) override;
    void visit(const InputNode&        node) override;
    void visit(const BorrowNode&       node) override;
    void visit(const CopyNode&         node) override;


private:
    CodeGenerator& m_cg;
    llvm::Value*   m_lastValue;
    VarType        m_lastVarType;

    llvm::Value* toBool(llvm::Value* val);
    std::string  convertFormatString(const std::string&         input,
                                      const std::vector<VarType>& argTypes,
                                      bool                        newline);
    llvm::Type*  varTypeToLLVM(VarType vt);
    VarType      llvmTypeToVarType(llvm::Type* type);
    VarType      stringToVarType(const std::string& s);  
};

#endif
