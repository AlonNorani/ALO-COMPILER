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
    int result = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after statement");

    std::cout << "Result: " << result << std::endl;
}


int Parser::parseExpression()
{
    int value = parseTerm();

    while(currentToken.type == TokenType::PLUS ||
          currentToken.type == TokenType::MINUS) {

        TokenType op = currentToken.type;
        advance();

        int rhs = parseTerm();

        if(op == TokenType::PLUS)
            value += rhs;
        else
            value -= rhs;
    }

    return value;
}


/*
this function parses a term in an expression. For now, a term is just a number, but in the future it can be extended to include variables, function calls, and parenthesized expressions.
*/
int Parser::parseTerm()
{
    int value = parseFactor();

    while(currentToken.type == TokenType::STAR ||
          currentToken.type == TokenType::SLASH) {

        TokenType op = currentToken.type;
        advance();

        int rhs = parseFactor();

        if(op == TokenType::STAR)
            value *= rhs;
        else
            value /= rhs;
    }

    return value;
}


int Parser::parseFactor()
{
    if(currentToken.type == TokenType::NUMBER) {
        int value = std::stoi(currentToken.lexeme);
        advance();
        return value;
    }

    if(currentToken.type == TokenType::LPAREN) {
        advance();
        int value = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        return value;
    }

    throw std::runtime_error(
        "Syntax Error at line " + std::to_string(currentToken.line) +
        ", column " + std::to_string(currentToken.column) +
        ": Expected a factor"
    );
}


