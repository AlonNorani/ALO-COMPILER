#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instructions.h>

// Symbol table: stack of scopes
// Each variable maps to its stack allocation (AllocaInst)
using SymbolTable = std::vector<std::unordered_map<std::string, llvm::AllocaInst*>>;

class CodeGenerator
{
public:

    CodeGenerator();

    // Access LLVM components
    llvm::IRBuilder<>& getBuilder();
    llvm::LLVMContext& getContext();
    llvm::Module& getModule();

    // Scope management
    void pushScope();
    void popScope();

    // Variable management
    void declareVariable(const std::string& name, llvm::AllocaInst* alloc);
    llvm::AllocaInst* lookupVariable(const std::string& name);

    // Load variable value
    llvm::Value* loadVariable(const std::string& name);

    // Allocate variable in function entry block
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* function, const std::string& name);

    // Function helpers
    llvm::Function* createMainFunction();
    void finishMain();

    // Debug
    void printIR();

private:

    llvm::LLVMContext m_context;
    llvm::IRBuilder<> m_builder;
    llvm::Module m_module;

    SymbolTable m_scopes;
};

#endif