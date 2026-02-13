#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <stdexcept>
#include "Lexer.h"
#include <memory>
#include "ASTNodes.h"
#include <vector>

class Parser
{
public:
    explicit Parser(Lexer& lexer);
    ~Parser() = default;
    std::vector<std::unique_ptr<ASTNode>> parseProgram();

private:
    Lexer& lexer; // reference to a Lexer instance
    Token currentToken;
    bool match(TokenType expectedType);
    void expect(TokenType expectedType, const std::string& errorMessage);
    void advance();

    // parse functions for different grammar rules
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseTerm();
    std::unique_ptr<ASTNode> parseFactor();



};

#endif