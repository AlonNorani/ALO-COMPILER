#include "Parser.h"

Parser::Parser(Lexer& lexer) : lexer(lexer)
{
    advance(); // Initialize currentToken with the first token
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseProgram() {
    std::vector<std::unique_ptr<ASTNode>> nodes;

    while(currentToken.type != TokenType::EOF_TOKEN) {
        nodes.push_back(parseStatement());
    }

    return nodes;
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

std::unique_ptr<ASTNode> Parser::parseStatement() {
    // check if it's an assignment
    if (currentToken.type == TokenType::IDENTIFIER && lexer.peekNextToken().type == TokenType::EQUAL) {
        std::string varName = currentToken.lexeme;
        advance(); // consume variable
        advance(); // consume '='
        auto expr = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after assignment");
        return std::make_unique<AssignmentNode>(varName, std::move(expr));
    }

    // else it's just an expression statement
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after statement");
    return expr;
}


// parseExpression: parses terms connected by + or -
std::unique_ptr<ASTNode> Parser::parseExpression() {
    auto left = parseTerm();

    while (currentToken.type == TokenType::PLUS ||
           currentToken.type == TokenType::MINUS) {
        char op = (currentToken.type == TokenType::PLUS) ? '+' : '-';
        advance();
        auto right = parseTerm();
        left = std::make_unique<BinaryOpNode>(std::move(left), std::move(right), op);
    }

    return left;
}


// parseTerm: parses factors connected by * or /
std::unique_ptr<ASTNode> Parser::parseTerm() {
    auto left = parseFactor();

    while (currentToken.type == TokenType::STAR || 
           currentToken.type == TokenType::SLASH) {
        char op = (currentToken.type == TokenType::STAR) ? '*' : '/';
        advance();
        auto right = parseFactor();
        left = std::make_unique<BinaryOpNode>(std::move(left), std::move(right), op);
    }

    return left;
}
std::unique_ptr<ASTNode> Parser::parseFactor() {
    if(currentToken.type == TokenType::NUMBER) {
        int val = std::stoi(currentToken.lexeme);
        advance();
        return std::make_unique<NumberNode>(val);
    }
    if(currentToken.type == TokenType::IDENTIFIER) {
        std::string name = currentToken.lexeme;
        advance();
        return std::make_unique<VariableNode>(name);
    }
    if(currentToken.type == TokenType::LPAREN) {
        advance();
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }
    throw std::runtime_error("Syntax Error: Expected a factor");
}


