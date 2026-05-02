#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "CodeGenerator.h"
#include "CodeGenVisitor.h"

/**
 * @file main.cpp
 * @brief Main entry point for the ALO compiler.
 *
 * This program implements a complete compiler pipeline for the ALO programming language:
 * 1. Lexical analysis (tokenization)
 * 2. Syntax analysis (parsing)
 * 3. Semantic analysis (type checking, ownership validation)
 * 4. Code generation (LLVM IR emission)
 * 5. Compilation to executable
 *
 * The compiler enforces ownership and borrowing rules similar to Rust,
 * ensuring memory safety for heap-allocated strings and arrays.
 */

/**
 * @brief Read the contents of a source file.
 * @param filename Path to the source file
 * @return The file contents as a string
 * @throws std::runtime_error if file cannot be opened
 */
std::string readFile(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to open file: ") + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * @brief Compile ALO source code to an executable.
 *
 * This function orchestrates the entire compilation pipeline:
 * - Tokenizes the source code
 * - Parses tokens into an Abstract Syntax Tree
 * - Performs semantic analysis (type checking, ownership validation)
 * - Generates LLVM Intermediate Representation
 * - Compiles IR to native executable using LLVM tools
 *
 * @param source The ALO source code to compile
 */
void compile(const std::string& source)
{
    // Phase 1: Lexical Analysis
    // Convert source text into tokens
    Lexer lexer(source);
    Parser parser(lexer);
    auto ast = parser.parseProgram();

    // Phase 2: Semantic Analysis
    // Check types, scoping, and ownership rules
    SemanticAnalyzer analyzer;
    analyzer.analyze(ast); // ensure semantic correctness before codegen

    // Phase 3: Code Generation
    // Generate LLVM IR from the validated AST
    CodeGenerator codeGen;
    CodeGenVisitor visitor(codeGen);
    visitor.generate(ast); // generate LLVM IR from AST

    // Phase 4: Compilation to Executable
    // Use LLVM tools to create native executable
    codeGen.writeIRToFile("output.ll"); // write the generated IR to a file

    system("llc -filetype=obj output.ll -o output.o");
    system("gcc -no-pie output.o -o output");
    std::cout << "Compilation completed successfully." << std::endl;
}

/**
 * @brief Main entry point.
 *
 * Expects a single command-line argument: the path to an ALO source file.
 * Compiles the source file to an executable named "output".
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return 0 on success, 1 on error
 */
int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    try {
        std::string source = readFile(argv[1]);

        compile(source);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}