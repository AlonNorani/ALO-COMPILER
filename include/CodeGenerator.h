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

// Each scope maps a variable name to its AllocaInst
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

    // Scalar variable management
    void               declareVariable(const std::string& name, llvm::AllocaInst* alloc);
    llvm::AllocaInst*  lookupVariable (const std::string& name);
    llvm::Value*       loadVariable   (const std::string& name);

    // Allocate scalar in function entry block (type defaults to i32)
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function*    function,
                                              const std::string& name,
                                              llvm::Type*        type = nullptr);

    // ── Array helpers (NEW) ───────────────────────────────────────────────
    // Allocate a fixed-size array in the function entry block.
    // elementType: LLVM type of one element  (e.g. i32)
    // size:        number of elements
    llvm::AllocaInst* createArrayAlloca(llvm::Function*    function,
                                         const std::string& name,
                                         llvm::Type*        elementType,
                                         int                size);

    // GEP pointer to arr[index].  alloc must be the AllocaInst for the array.
    llvm::Value* getArrayElementPtr(llvm::AllocaInst* alloc,
                                     llvm::Value*      index,
                                     llvm::Type*       elementType);

    // Store an element type mapping (needed by load/store helpers)
    void              setArrayElementType(const std::string& name, llvm::Type* elemTy);
    llvm::Type*       getArrayElementType(const std::string& name);

    // Main function boilerplate
    llvm::Function* createMainFunction();
    void            finishMain();

    void printIR();
    void writeIRToFile(const std::string& filename);

private:
    llvm::LLVMContext m_context;
    llvm::IRBuilder<> m_builder;
    llvm::Module      m_module;

    ScopesTable m_scopes;

    // Maps variable name → LLVM element type (for arrays)
    std::unordered_map<std::string, llvm::Type*> m_arrayElemTypes;
};

#endif
