#include "CodeGenerator.h"
#include <llvm/Support/raw_ostream.h>

CodeGenerator::CodeGenerator()
    : m_builder(m_context),
      m_module("alo_module", m_context)
{
    pushScope();
}

llvm::IRBuilder<>& CodeGenerator::getBuilder()
{
    return m_builder;
}

llvm::LLVMContext& CodeGenerator::getContext()
{
    return m_context;
}

llvm::Module& CodeGenerator::getModule()
{
    return m_module;
}

void CodeGenerator::pushScope()
{
    m_scopes.push_back({});
}

void CodeGenerator::popScope()
{
    if (!m_scopes.empty())
        m_scopes.pop_back();
}

void CodeGenerator::declareVariable(const std::string& name, llvm::AllocaInst* alloc)
{
    if (m_scopes.empty())
        throw std::runtime_error("No active scope");

    m_scopes.back()[name] = alloc;
}

llvm::AllocaInst* CodeGenerator::lookupVariable(const std::string& name)
{
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
    {
        auto found = it->find(name);
        if (found != it->end())
            return found->second;
    }

    return nullptr;
}

llvm::Value* CodeGenerator::loadVariable(const std::string& name)
{
    llvm::AllocaInst* var = lookupVariable(name);

    if (!var)
        throw std::runtime_error("Unknown variable: " + name);

    return m_builder.CreateLoad(
        m_builder.getInt32Ty(),
        var,
        name
    );
}

llvm::AllocaInst* CodeGenerator::createEntryBlockAlloca(llvm::Function* function, const std::string& name)
{
    llvm::IRBuilder<> tmpBuilder(
        &function->getEntryBlock(),
        function->getEntryBlock().begin()
    );

    return tmpBuilder.CreateAlloca(
        m_builder.getInt32Ty(),
        nullptr,
        name
    );
}

llvm::Function* CodeGenerator::createMainFunction()
{
    llvm::FunctionType* mainType =
        llvm::FunctionType::get(
            m_builder.getInt32Ty(),
            false
        );

    llvm::Function* mainFunc =
        llvm::Function::Create(
            mainType,
            llvm::Function::ExternalLinkage,
            "main",
            m_module
        );

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(
            m_context,
            "entry",
            mainFunc
        );

    m_builder.SetInsertPoint(entry);

    pushScope();

    return mainFunc;
}

void CodeGenerator::finishMain()
{
    m_builder.CreateRet(
        llvm::ConstantInt::get(
            m_builder.getInt32Ty(),
            0
        )
    );

    popScope();
}

void CodeGenerator::printIR()
{
    m_module.print(llvm::outs(), nullptr);
}