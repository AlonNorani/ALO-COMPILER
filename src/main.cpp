#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "../include/Lexer.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }

    // Read entire file into a string
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Lexer lexer(source);

    std::cout << "=== Tokens ===" << std::endl;
    Token token;
    do {
        token = lexer.getNextToken();
        std::cout << "Token: " << static_cast<int>(token.type)
                  << " Lexeme: '" << token.lexeme
                  << "' Line: " << token.line
                  << " Column: " << token.column << std::endl;
    } while (token.type != TokenType::EOF_TOKEN);

    return 0;
}
