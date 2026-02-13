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

void Parser::parseExpression()
{
    parseTerm();

    while(currentToken.type == TokenType::PLUS ||
          currentToken.type == TokenType::MINUS) {
        advance();        // consume + or -
        parseTerm();      // parse next term
    }
}


/*
this function parses a term in an expression. For now, a term is just a number, but in the future it can be extended to include variables, function calls, and parenthesized expressions.
*/
void Parser::parseTerm()
{
    parseFactor();

    while(currentToken.type == TokenType::STAR ||
          currentToken.type == TokenType::SLASH) {
        advance();        // consume * or /
        parseFactor();    // parse next factor
    }
}

void Parser::parseFactor()
{
    if(currentToken.type == TokenType::NUMBER) {
        advance();
        return;
    }

    if(currentToken.type == TokenType::LPAREN) {
        advance();
        parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        return;
    }

    throw std::runtime_error(
        "Syntax Error at line " + std::to_string(currentToken.line) +
        ", column " + std::to_string(currentToken.column) +
        ": Expected a factor"
    );
}

