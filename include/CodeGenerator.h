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

// stack of symbol tables
using SymbolTable = std::vector<std::unordered_map<std::string, llvm::Value*>>;

class CodeGenerator {
public:
    CodeGenerator();

    // LLVM access 
    llvm::IRBuilder<>& getBuilder();
    llvm::LLVMContext& getContext();
    llvm::Module& getModule();

    // Scope management
    void pushScope();
    void popScope();

    // Variables
    void declareVariable(const std::string& name, llvm::Value* value);
    llvm::Value* lookupVariable(const std::string& name);

    // Functions
    llvm::Function* createMainFunction();

    // Debug
    void printIR();

private:
    llvm::LLVMContext m_context;
    llvm::IRBuilder<> m_builder;
    llvm::Module m_module;

    SymbolTable m_scopes;
};

#endif