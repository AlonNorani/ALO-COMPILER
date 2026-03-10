#include "CodeGenerator.h"
#include <llvm/Support/raw_ostream.h>

CodeGenerator::CodeGenerator()
    : m_builder(m_context),
      m_module("alo_compiler_module", m_context)
{
    pushScope();
}
void CodeGenerator::pushScope() {
    m_scopes.push_back({});
}

void CodeGenerator::popScope() {
    if (!m_scopes.empty())
        m_scopes.pop_back();
}

void CodeGenerator::declareVariable(const std::string& name, llvm::Value* value) {
    if (m_scopes.empty())
        throw std::runtime_error("No active scope to declare variable");
    m_scopes.back()[name] = value;
}

llvm::Value* CodeGenerator::lookupVariable(const std::string& name) {
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end())
            return found->second;
    }
    return nullptr; // Not found
}

llvm::Function* CodeGenerator::createMainFunction() {
    llvm::FunctionType* mainType =
        llvm::FunctionType::get(m_builder.getInt32Ty(), false);

    llvm::Function* mainFunc =
        llvm::Function::Create(
            mainType,
            llvm::Function::ExternalLinkage,
            "main",
            m_module
        );

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(m_context, "entry", mainFunc);

    m_builder.SetInsertPoint(entry);

    // Push scope for function
    pushScope();

    return mainFunc;
}

// -------- Print IR --------
void CodeGenerator::printIR() {
    m_module.print(llvm::outs(), nullptr);
}

llvm::IRBuilder<>& CodeGenerator::getBuilder() {
    return m_builder;
}

llvm::LLVMContext& CodeGenerator::getContext() {
    return m_context;
}

llvm::Module& CodeGenerator::getModule() {
    return m_module;
}
