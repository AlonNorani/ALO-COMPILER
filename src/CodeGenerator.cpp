#include "CodeGenerator.h"
#include <llvm/Support/raw_ostream.h>

// CodeGenerator: Manages LLVM IR generation, including contexts, modules, builders,
// and symbol tables for variables and arrays. Handles scope management for local variables.

CodeGenerator::CodeGenerator()
    : m_builder(m_context),
      m_module("alo_module", m_context)
{
    pushScope();
}

llvm::IRBuilder<>& CodeGenerator::getBuilder() { return m_builder; }
llvm::LLVMContext& CodeGenerator::getContext() { return m_context; }
llvm::Module&      CodeGenerator::getModule()  { return m_module;  }

// Scope management: Uses a stack of maps for nested scopes (e.g., function bodies, blocks).
// Variables are looked up from innermost to outermost scope.
void CodeGenerator::pushScope() { m_scopes.push_back({}); }
void CodeGenerator::popScope()  { if (!m_scopes.empty()) m_scopes.pop_back(); }

// Declares a variable in the current scope, associating name with its LLVM alloca instruction.
void CodeGenerator::declareVariable(const std::string& name, llvm::AllocaInst* alloc)
{
    if (m_scopes.empty())
        throw std::runtime_error("No active scope");
    m_scopes.back()[name] = alloc;
}

// Looks up a variable by name across all scopes, returning nullptr if not found.
llvm::AllocaInst* CodeGenerator::lookupVariable(const std::string& name)
{
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return nullptr;
}

// Loads the value of a variable from its alloca, creating an LLVM load instruction.
llvm::Value* CodeGenerator::loadVariable(const std::string& name)
{
    llvm::AllocaInst* var = lookupVariable(name);
    if (!var) throw std::runtime_error("Unknown variable: " + name);
    return m_builder.CreateLoad(var->getAllocatedType(), var, name);
}

// Creates an alloca instruction at the function's entry block for local variables.
// This ensures variables are allocated at the start of the function in LLVM.
llvm::AllocaInst* CodeGenerator::createEntryBlockAlloca(llvm::Function*    function,
                                                          const std::string& name,
                                                          llvm::Type*        type)
{
    llvm::IRBuilder<> tmp(&function->getEntryBlock(),
                          function->getEntryBlock().begin());
    if (!type) type = m_builder.getInt32Ty();
    return tmp.CreateAlloca(type, nullptr, name);
}

// Creates an alloca for an array with fixed size, storing the element type for later access.
// LLVM arrays are [size x elementType], allocated on the stack.
llvm::AllocaInst* CodeGenerator::createArrayAlloca(llvm::Function*    function,
                                                    const std::string& name,
                                                    llvm::Type*        elementType,
                                                    int                size)
{
    llvm::IRBuilder<> tmp(&function->getEntryBlock(),
                          function->getEntryBlock().begin());

    llvm::ArrayType* arrTy = llvm::ArrayType::get(elementType, static_cast<uint64_t>(size));
    llvm::AllocaInst* alloc = tmp.CreateAlloca(arrTy, nullptr, name);

    m_arrayElemTypes[name] = elementType;

    return alloc;
}

// Generates a GetElementPtr (GEP) instruction to compute the address of an array element.
// For arrays like [N x T], GEP uses {0, index} to get &array[index].
// This is LLVM's way to handle pointer arithmetic safely.
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

// Stores the element type of an array for type checking during access.
void CodeGenerator::setArrayElementType(const std::string& name, llvm::Type* elemTy)
{
    m_arrayElemTypes[name] = elemTy;
}

// Retrieves the stored element type of an array.
llvm::Type* CodeGenerator::getArrayElementType(const std::string& name)
{
    auto it = m_arrayElemTypes.find(name);
    if (it != m_arrayElemTypes.end()) return it->second;
    return nullptr;
}

// Creates the main function with LLVM's standard signature: int main().
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

// Finishes the main function by adding a return 0 and popping the scope.
void CodeGenerator::finishMain()
{
    m_builder.CreateRet(llvm::ConstantInt::get(m_builder.getInt32Ty(), 0));
    popScope();
}

// Prints the generated LLVM IR to stdout for debugging.
void CodeGenerator::printIR()
{
    m_module.print(llvm::outs(), nullptr);
}

// Writes the LLVM IR to a file for further processing (e.g., by llc).
void CodeGenerator::writeIRToFile(const std::string& filename)
{
    std::error_code EC;
    llvm::raw_fd_ostream file(filename, EC);
    if (EC) throw std::runtime_error("Failed to open IR output file");
    m_module.print(file, nullptr);
}
