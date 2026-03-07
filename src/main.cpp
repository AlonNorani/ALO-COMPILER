#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"

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

void checkLexer(const std::string& source)
{
    Lexer lexer(source);

    std::cout << "=== Lexer check ===" << std::endl;

    Token token;
    do {
        token = lexer.getNextToken();
        std::cout << "Token: " << static_cast<int>(token.type)
                  << " Lexeme: '" << token.lexeme
                  << "' Line: " << token.line
                  << " Column: " << token.column << std::endl;
    } while (token.type != TokenType::EOF_TOKEN);

    std::cout << std::endl;
}

void checkParser(const std::string& source)
{
    Lexer lexer(source);
    Parser parser(lexer);

    auto ast = parser.parseProgram();

    std::cout << "=== AST check ===" << std::endl;
    for (auto& node : ast) {
        node->print();           // print tree structure
        std::cout << "Value: " << node->evaluate() << "\n\n"; // optional evaluation
    }
}

void checkSemanticAnalyzer(const std::string& source)
{
    Lexer lexer(source);
    Parser parser(lexer);
    auto ast = parser.parseProgram();

    SemanticAnalyzer analyzer;
    analyzer.analyze(ast); // will throw if something is wrong

    std::cout << "Semantic analysis completed successfully." << std::endl;
}
int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    try {
        std::string source = readFile(argv[1]);

        // Uncomment when you want lexer debugging
        // checkLexer(source);

        // Uncomment when you want parser debugging
        // checkParser(source);

        // Uncomment when you want semantic analysis debugging
        checkSemanticAnalyzer(source);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
