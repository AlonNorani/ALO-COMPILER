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
#include <llvm/Support/raw_ostream.h>
#include <system_error>

using ScopesTable = std::vector<std::unordered_map<std::string, llvm::AllocaInst*>>;

class CodeGenerator
{
public:
    CodeGenerator();

    llvm::IRBuilder<>& getBuilder();
    llvm::LLVMContext& getContext();
    llvm::Module&      getModule();

    // Scope management
    void pushScope();
    void popScope();

    void declareVariable(const std::string& name, llvm::AllocaInst* alloc);
    llvm::AllocaInst* lookupVariable (const std::string& name);
    llvm::Value* loadVariable   (const std::string& name);

    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function*    function,
                                              const std::string& name,
                                              llvm::Type*        type = nullptr);

    llvm::AllocaInst* createArrayAlloca(llvm::Function*    function,
                                         const std::string& name,
                                         llvm::Type*        elementType,
                                         int                size);

    llvm::Value* getArrayElementPtr(llvm::AllocaInst* alloc,
                                     llvm::Value*      index,
                                     llvm::Type*       elementType);

    void              setArrayElementType(const std::string& name, llvm::Type* elemTy);
    llvm::Type*       getArrayElementType(const std::string& name);

    llvm::Function* createMainFunction();
    void            finishMain();

    void printIR();
    void writeIRToFile(const std::string& filename);

private:
    llvm::LLVMContext m_context;
    llvm::IRBuilder<> m_builder;
    llvm::Module      m_module;

    ScopesTable m_scopes;

    std::unordered_map<std::string, llvm::Type*> m_arrayElemTypes;
};

#endif
