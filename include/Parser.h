#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <stdexcept>
#include "Lexer.h"

class Parser
{
public:
    explicit Parser(Lexer& lexer);
    ~Parser() = default;
    void parseProgram();

private:
    Lexer& lexer; // reference to a Lexer instance
    Token currentToken;
    bool match(TokenType expectedType);
    void expect(TokenType expectedType, const std::string& errorMessage);
    void advance();

    // parse functions for different grammar rules
    void parseStatement();
    void parseExpression();



};

#endif