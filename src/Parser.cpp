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
    expect(TokenType::SEMICOLON, "Expected ';' after statement");

}

void Parser::parseExpression() // for now, expression is just a number
{
    parseTerm(); // get the left hand side term (number)
    // In the future, we can extend this to handle operators and more complex expressions
    while(currentToken.type == TokenType::PLUS || currentToken.type == TokenType::MINUS) {
        advance(); // consume the operator
        parseTerm(); // get the right hand side term (number for now)
    }
}

/*
this function parses a term in an expression. For now, a term is just a number, but in the future it can be extended to include variables, function calls, and parenthesized expressions.
*/
void Parser::parseTerm() 
{
    if(currentToken.type == TokenType::NUMBER) {
        advance();
        return;
    }
    throw std::runtime_error(
        "Syntax Error at line " + std::to_string(currentToken.line) +
        ", column " + std::to_string(currentToken.column) +
        ": Expected a term (number)"
    );  
}