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

/**
 * @file CodeGenerator.h
 * @brief Defines the CodeGenerator class that manages LLVM IR generation.
 *
 * The CodeGenerator provides a high-level interface for generating LLVM IR
 * from the AST. It handles variable allocation, scoping, array operations,
 * and maintains metadata about variables and types.
 */

/** Type alias for nested scopes: stack of maps from variable name to LLVM allocation */
using ScopesTable = std::vector<std::unordered_map<std::string, llvm::AllocaInst*>>;

/**
 * @class CodeGenerator
 * @brief Manages LLVM IR generation and provides utilities for code generation.
 *
 * This class encapsulates the LLVM context, module, and IR builder. It provides
 * methods for variable management, scoping, array operations, and metadata tracking
 * necessary for generating correct LLVM IR from ALO programs.
 */
class CodeGenerator
{
public:
    /**
     * @brief Construct a new code generator with LLVM context and module.
     */
    CodeGenerator();

    // LLVM component access
    llvm::IRBuilder<>& getBuilder();  ///< Get the LLVM IR builder
    llvm::LLVMContext& getContext();  ///< Get the LLVM context
    llvm::Module&      getModule();   ///< Get the LLVM module

    // Scope management
    void pushScope();  ///< Enter a new variable scope
    void popScope();   ///< Exit the current variable scope

    // Variable management
    void declareVariable(const std::string& name, llvm::AllocaInst* alloc);  ///< Declare a variable in current scope
    llvm::AllocaInst* lookupVariable (const std::string& name);              ///< Find variable allocation
    llvm::Value* loadVariable   (const std::string& name);                   ///< Load variable value

    // Memory allocation
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function*    function,
                                              const std::string& name,
                                              llvm::Type*        type = nullptr);  ///< Allocate in function entry

    llvm::AllocaInst* createArrayAlloca(llvm::Function*    function,
                                         const std::string& name,
                                         llvm::Type*        elementType,
                                         int                size);  ///< Allocate array on stack

    // Array operations
    llvm::Value* getArrayElementPtr(llvm::AllocaInst* alloc,
                                     llvm::Value*      index,
                                     llvm::Type*       elementType);  ///< Get pointer to array element

    // Array metadata (for semantic information)
    void              setArrayElementType(const std::string& name, llvm::Type* elemTy);  ///< Set array element type
    llvm::Type*       getArrayElementType(const std::string& name);                      ///< Get array element type

    void              setArraySize(const std::string& name, int size);  ///< Set array size
    int               getArraySize(const std::string& name);            ///< Get array size

    // Function management
    llvm::Function* createMainFunction();  ///< Create the main function
    void            finishMain();           ///< Finalize the main function

    // Output
    void printIR();  ///< Print LLVM IR to console
    void writeIRToFile(const std::string& filename);  ///< Write LLVM IR to file

private:
    llvm::LLVMContext m_context;   ///< LLVM context for this compilation
    llvm::IRBuilder<> m_builder;   ///< IR builder for generating instructions
    llvm::Module      m_module;    ///< LLVM module containing all functions and globals

    ScopesTable m_scopes;  ///< Stack of variable scopes

    // Metadata for arrays (element type and size)
    std::unordered_map<std::string, llvm::Type*> m_arrayElemTypes;  ///< Maps variable name to element type
    std::unordered_map<std::string, int> m_arraySizes;             ///< Maps variable name to array size
};

#endif