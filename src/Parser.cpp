#include "Parser.h"

Parser::Parser(Lexer& lexer) : lexer(lexer)
{
    advance(); // Initialize currentToken with the first token
}

void Parser::parseProgram()
{
    while(currentToken.type != TokenType::EOF_TOKEN) {
        parseStatement();
    }
}

bool Parser::match(TokenType expectedType)
{
    if(currentToken.type == expectedType) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType expectedType, const std::string& errorMessage)
{
    if(!match(expectedType)) {
        throw std::runtime_error(
            "Syntax Error at line " + std::to_string(currentToken.line) +
            ", column " + std::to_string(currentToken.column) +
            ": " + errorMessage
        );
    }
}


void Parser::advance()
{
    currentToken = lexer.getNextToken();
}

void Parser::parseStatement()
{
    parseExpression(); // for now, a statement is just an expression (number)
}

void Parser::parseExpression() // for now, expression is just a number
{
    if(currentToken.type == TokenType::NUMBER) {
        advance();
        return;
    }
    throw std::runtime_error(
        "Syntax Error at line " + std::to_string(currentToken.line) +
        ", column " + std::to_string(currentToken.column) +
        ": Expected an expression (number)"
    );
    
}