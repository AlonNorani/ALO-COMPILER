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
    if(currentToken.type == TokenType::LET) {
        return parseAssignment(); 
    }
    else if(currentToken.type == TokenType::IF) {
        return parseIfStatement();
    }
    else if(currentToken.type == TokenType::WHILE) {
        return parseWhileStatement();
    }
    else if(currentToken.type == TokenType::LBRACE) { // '{'
        return parseBlock();
    }
    else {
        auto expr = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after statement");
        return expr;
    }
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

std::unique_ptr<ASTNode> Parser::parseAssignment()
{
    advance(); // consume the let keyword
    if(currentToken.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected variable name after 'let'");
    }
    std::string varName = currentToken.lexeme;
    advance(); // consume variable name
    expect(TokenType::EQUAL, "Expected '=' after variable name");
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after declaration");
    return std::make_unique<AssignmentNode>(varName, std::move(expr));
}

std::unique_ptr<BlockNode> Parser::parseBlock() {
    expect(TokenType::LBRACE, "Expected '{'");
    std::vector<std::unique_ptr<ASTNode>> stmts;

    while(currentToken.type != TokenType::RBRACE &&
          currentToken.type != TokenType::EOF_TOKEN) {
        stmts.push_back(parseStatement());
    }

    expect(TokenType::RBRACE, "Expected '}'");
    return std::make_unique<BlockNode>(std::move(stmts));
}


std::unique_ptr<IfNode> Parser::parseIfStatement() {
    expect(TokenType::IF, "Expected 'if'");

    expect(TokenType::LPAREN, "Expected '(' after 'if'");
    auto condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after condition");

    auto thenBranch = parseBlock();

    std::unique_ptr<BlockNode> elseBranch = nullptr;

    if (currentToken.type == TokenType::ELSE) {
        advance(); // consume 'else'

        // check for else if (nested if) to support else if chains
        if (currentToken.type == TokenType::IF) {
            // recursively parse nested if
            auto nestedIf = parseIfStatement();

            // wrap nested if inside a block
            std::vector<std::unique_ptr<ASTNode>> stmts;
            stmts.push_back(std::move(nestedIf));
            elseBranch = std::make_unique<BlockNode>(std::move(stmts));
        }
        else {
            elseBranch = parseBlock();
        }
    }

    return std::make_unique<IfNode>(
        std::move(condition),
        std::move(thenBranch),
        std::move(elseBranch)
    );
}


std::unique_ptr<WhileNode> Parser::parseWhileStatement() {
    expect(TokenType::WHILE, "Expected 'while'");

    expect(TokenType::LPAREN, "Expected '(' after 'while'");
    auto condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after condition");

    auto body = parseBlock();

    return std::make_unique<WhileNode>(
        std::move(condition),
        std::move(body)
    );
}
