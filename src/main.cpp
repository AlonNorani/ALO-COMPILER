#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "CodeGenerator.h"
#include "CodeGenVisitor.h"

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






void compile(const std::string& source)
{
    Lexer lexer(source);
    Parser parser(lexer);
    auto ast = parser.parseProgram();

    SemanticAnalyzer analyzer;
    analyzer.analyze(ast); // ensure semantic correctness before codegen

    CodeGenerator codeGen;
    CodeGenVisitor visitor(codeGen);
    visitor.generate(ast); // generate LLVM IR from AST
    codeGen.printIR(); // print the generated LLVM IR to stdout


    codeGen.writeIRToFile("output.ll"); // write the generated IR to a file 

    system("llc -filetype=obj output.ll -o output.o");
    system("gcc -no-pie output.o -o output");
    std::cout << "Code generation completed successfully." << std::endl;
}
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
