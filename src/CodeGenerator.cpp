#include "CodeGenerator.h"
#include <llvm/Support/raw_ostream.h>

CodeGenerator::CodeGenerator()
    : m_builder(m_context),
      m_module("alo_module", m_context)
{
    pushScope();
}

llvm::IRBuilder<>& CodeGenerator::getBuilder() { return m_builder; }
llvm::LLVMContext& CodeGenerator::getContext() { return m_context; }
llvm::Module&      CodeGenerator::getModule()  { return m_module;  }

void CodeGenerator::pushScope() { m_scopes.push_back({}); }
void CodeGenerator::popScope()  { if (!m_scopes.empty()) m_scopes.pop_back(); }

void CodeGenerator::declareVariable(const std::string& name, llvm::AllocaInst* alloc)
{
    if (m_scopes.empty())
        throw std::runtime_error("No active scope");
    m_scopes.back()[name] = alloc;
}

llvm::AllocaInst* CodeGenerator::lookupVariable(const std::string& name)
{
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return nullptr;
}

llvm::Value* CodeGenerator::loadVariable(const std::string& name)
{
    llvm::AllocaInst* var = lookupVariable(name);
    if (!var) throw std::runtime_error("Unknown variable: " + name);
    return m_builder.CreateLoad(var->getAllocatedType(), var, name);
}

llvm::AllocaInst* CodeGenerator::createEntryBlockAlloca(llvm::Function*    function,
                                                          const std::string& name,
                                                          llvm::Type*        type)
{
    llvm::IRBuilder<> tmp(&function->getEntryBlock(),
                          function->getEntryBlock().begin());
    if (!type) type = m_builder.getInt32Ty();
    return tmp.CreateAlloca(type, nullptr, name);
}

// ─────────────────────────────────────────────
//  Array helpers  (NEW)
// ─────────────────────────────────────────────

// Allocates  [size x elementType]  in the function entry block.
llvm::AllocaInst* CodeGenerator::createArrayAlloca(llvm::Function*    function,
                                                    const std::string& name,
                                                    llvm::Type*        elementType,
                                                    int                size)
{
    llvm::IRBuilder<> tmp(&function->getEntryBlock(),
                          function->getEntryBlock().begin());

    llvm::ArrayType* arrTy = llvm::ArrayType::get(elementType, static_cast<uint64_t>(size));
    llvm::AllocaInst* alloc = tmp.CreateAlloca(arrTy, nullptr, name);

    // Remember the element type so loads/stores can use it later
    m_arrayElemTypes[name] = elementType;

    return alloc;
}

// Returns a pointer to arr[index] using GEP.
// alloc->getAllocatedType() is [N x elemTy]; we index into it with {0, index}.
llvm::Value* CodeGenerator::getArrayElementPtr(llvm::AllocaInst* alloc,
                                                llvm::Value*      index,
                                                llvm::Type*       elementType)
{
    llvm::Value* zero = llvm::ConstantInt::get(m_builder.getInt32Ty(), 0);
    return m_builder.CreateInBoundsGEP(
        alloc->getAllocatedType(),
        alloc,
        {zero, index},
        "elemptr"
    );
}

void CodeGenerator::setArrayElementType(const std::string& name, llvm::Type* elemTy)
{
    m_arrayElemTypes[name] = elemTy;
}

llvm::Type* CodeGenerator::getArrayElementType(const std::string& name)
{
    auto it = m_arrayElemTypes.find(name);
    if (it != m_arrayElemTypes.end()) return it->second;
    return nullptr;
}

// ─────────────────────────────────────────────
//  Main boilerplate (unchanged)
// ─────────────────────────────────────────────
llvm::Function* CodeGenerator::createMainFunction()
{
    llvm::FunctionType* mainType =
        llvm::FunctionType::get(m_builder.getInt32Ty(), false);

    llvm::Function* mainFunc =
        llvm::Function::Create(mainType, llvm::Function::ExternalLinkage,
                               "main", m_module);

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(m_context, "entry", mainFunc);
    m_builder.SetInsertPoint(entry);

    pushScope();
    return mainFunc;
}

void CodeGenerator::finishMain()
{
    m_builder.CreateRet(llvm::ConstantInt::get(m_builder.getInt32Ty(), 0));
    popScope();
}

void CodeGenerator::printIR()
{
    m_module.print(llvm::outs(), nullptr);
}

void CodeGenerator::writeIRToFile(const std::string& filename)
{
    std::error_code EC;
    llvm::raw_fd_ostream file(filename, EC);
    if (EC) throw std::runtime_error("Failed to open IR output file");
    m_module.print(file, nullptr);
}
