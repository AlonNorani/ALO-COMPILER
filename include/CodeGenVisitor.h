#ifndef CODEGENVISITOR_H
#define CODEGENVISITOR_H

#include "ASTVisitor.h"
#include "ASTNodes.h"
#include "CodeGenerator.h"
#include <llvm/IR/Value.h>
#include "SemanticAnalyzer.h"

/**
 * @file CodeGenVisitor.h
 * @brief Defines the CodeGenVisitor class that generates LLVM IR from the AST.
 *
 * The CodeGenVisitor implements the visitor pattern to traverse the AST and
 * generate corresponding LLVM Intermediate Representation (IR) instructions.
 * It handles all language constructs including expressions, statements, control
 * flow, and ownership-aware operations.
 */

/**
 * @class CodeGenVisitor
 * @brief AST visitor that generates LLVM IR instructions.
 *
 * This class traverses the Abstract Syntax Tree and emits LLVM IR code for
 * each node. It manages the translation of high-level ALO constructs into
 * low-level LLVM instructions, including memory management, control flow,
 * and heap allocation for strings and arrays.
 */
class CodeGenVisitor : public ASTVisitor
{
public:
    /**
     * @brief Construct a code generation visitor.
     * @param cg Reference to the code generator
     */
    explicit CodeGenVisitor(CodeGenerator& cg);

    /**
     * @brief Generate LLVM IR for the entire program.
     * @param program The AST nodes to compile
     */
    void generate(const std::vector<std::unique_ptr<ASTNode>>& program);

    /**
     * @brief Get the last computed LLVM value.
     * @return The most recently generated LLVM value
     */
    llvm::Value* getValue();

    // AST visitor methods - each generates LLVM IR for the corresponding node
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
    CodeGenerator& m_cg;         ///< Reference to the code generator
    llvm::Value*   m_lastValue;  ///< Last computed LLVM value
    VarType        m_lastVarType; ///< Type of the last computed value

    // Helper methods
    llvm::Value* toBool(llvm::Value* val);  ///< Convert value to boolean
    std::string  convertFormatString(const std::string&         input,
                                      const std::vector<VarType>& argTypes,
                                      bool                        newline);  ///< Convert format strings
    llvm::Type*  varTypeToLLVM(VarType vt);           ///< Convert ALO type to LLVM type
    VarType      llvmTypeToVarType(llvm::Type* type); ///< Convert LLVM type to ALO type
    VarType      stringToVarType(const std::string& s);  ///< Convert string to VarType
};

#endif
