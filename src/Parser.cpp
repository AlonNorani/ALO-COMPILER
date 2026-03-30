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

Token Parser::expect(TokenType expectedType, const std::string& errorMessage)
{
    Token previousToken = currentToken; // store the token that was just matched (or not matched)
    if(!match(expectedType)) {
        throw std::runtime_error(
            "Syntax Error at line " + std::to_string(currentToken.line) +
            ", column " + std::to_string(currentToken.column) +
            ": " + errorMessage
        );
    }
    return previousToken; // return the token that was just matched
}


void Parser::advance()
{
    currentToken = lexer.getNextToken();
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    if(currentToken.type == TokenType::PRINT_MACRO) {
        return parsePrint(false);
    }
    else if(currentToken.type == TokenType::PRINTLN_MACRO) {
        return parsePrint(true);
    }
    else if(currentToken.type == TokenType::FN) {
        return parseFunction();
    }
    else if(currentToken.type == TokenType::RETURN) 
    {
        return parseReturnStatement();
    }
    else if(currentToken.type == TokenType::LET) {
        return parseLetStatement();
    }
    else if(currentToken.type == TokenType::IDENTIFIER)
    {
        // Lookahead to see if this is an assignment
        Token nextToken = lexer.peekNextToken();
        if (nextToken.type == TokenType::EQUAL) {
            return parseAssignment();
        }
        // If it's not an assignment, treat it as an expression statement
        auto expr = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after expression");
        return expr;
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

std::unique_ptr<ASTNode> Parser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<ASTNode> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();

    while (currentToken.type == TokenType::LOGICAL_OR) {
        TokenType op = currentToken.type;
        advance();

        auto right = parseLogicalAnd();

        left = std::make_unique<BinaryOpNode>(
            std::move(left),
            std::move(right),
            op
        );
    }

    return left;
}


std::unique_ptr<ASTNode> Parser::parseLogicalAnd() {
    auto left = parseComparison();

    while (currentToken.type == TokenType::LOGICAL_AND) {
        TokenType op = currentToken.type;
        advance();

        auto right = parseComparison();

        left = std::make_unique<BinaryOpNode>(
            std::move(left),
            std::move(right),
            op
        );
    }

    return left;
}




// parses terms connected by + or -
std::unique_ptr<ASTNode> Parser::parseAdditive() {
    auto left = parseTerm();

    while (currentToken.type == TokenType::PLUS ||
           currentToken.type == TokenType::MINUS) {
        TokenType op = (currentToken.type == TokenType::PLUS) ? TokenType::PLUS : TokenType::MINUS;
        advance();
        auto right = parseTerm();
        left = std::make_unique<BinaryOpNode>(std::move(left), std::move(right), op);
    }

    return left;
}


// parseTerm: parses factors connected by * or /
std::unique_ptr<ASTNode> Parser::parseTerm() {
    auto left = parseUnary();

    while (currentToken.type == TokenType::STAR || 
           currentToken.type == TokenType::SLASH) {
        TokenType op = (currentToken.type == TokenType::STAR) ? TokenType::STAR : TokenType::SLASH;
        advance();
        auto right = parseUnary();
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

    if(currentToken.type == TokenType::STRING) {
        std::string val = currentToken.lexeme;
        advance();
        return std::make_unique<StringNode>(val);
    }
    if(currentToken.type == TokenType::IDENTIFIER) {
        std::string name = currentToken.lexeme;
        advance();

        // check for function call
        if(currentToken.type == TokenType::LPAREN) {
            advance(); // consume '('
            std::vector<std::unique_ptr<ASTNode>> args;

            if(currentToken.type != TokenType::RPAREN) { // not empty argument list
                do {
                    args.push_back(parseExpression()); // parse any expression as argument
                } while(match(TokenType::COMMA));
            }

            expect(TokenType::RPAREN, "Expected ')' after function call arguments");
            return std::make_unique<CallNode>(name, std::move(args));
        }

        // normal variable
        return std::make_unique<VariableNode>(name);
    }

    if(currentToken.type == TokenType::LPAREN) {
        advance();
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }

    if(currentToken.type == TokenType::TRUE) {
        advance();
        return std::make_unique<BoolNode>(true);
    }

    if(currentToken.type == TokenType::FALSE) {
        advance();
        return std::make_unique<BoolNode>(false);
    }

    throw std::runtime_error("Syntax Error: Expected a factor");
}

std::unique_ptr<ASTNode> Parser::parseAssignment()
{
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

std::unique_ptr<ASTNode> Parser::parseLetStatement()
{
    advance(); // consume 'let'

    bool isMutable = false;

    if (currentToken.type == TokenType::MUT) {
        isMutable = true;
        advance(); // consume 'mut'
    }

    if (currentToken.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected variable name after 'let'");
    }

    std::string varName = currentToken.lexeme;
    advance(); // consume identifier

    expect(TokenType::EQUAL, "Expected '=' after variable name");

    auto expr = parseExpression();

    expect(TokenType::SEMICOLON, "Expected ';' after declaration");

    return std::make_unique<LetNode>(
        varName,
        std::move(expr),
        isMutable
    );
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

std::unique_ptr<FunctionNode> Parser::parseFunction()
{
    expect(TokenType::FN, "Expected 'fn'");

    if(currentToken.type != TokenType::IDENTIFIER)
        throw std::runtime_error("Expected function name");

    std::string name = currentToken.lexeme;
    advance();

    expect(TokenType::LPAREN, "Expected '(' after function name");

    auto params = parseParameterList();

    expect(TokenType::RPAREN, "Expected ')' after parameters");

    expect(TokenType::ARROW, "Expected '->' after parameters");

    std::string returnType = parseType();

    auto body = parseBlock();

    return std::make_unique<FunctionNode>(
        name,
        std::move(params),
        returnType,
        std::move(body)
    );
}

std::vector<Parameter> Parser::parseParameterList()
{
    std::vector<Parameter> params;
    if(currentToken.type == TokenType::RPAREN)
        return params;

    while(true)
    {
        if(currentToken.type != TokenType::IDENTIFIER)
            throw std::runtime_error("Expected parameter name");
        std::string name = currentToken.lexeme;
        advance();
        expect(TokenType::COLON, "Expected ':' after parameter name");
        std::string type = parseType();

        params.push_back({type, name});

        if(!match(TokenType::COMMA))
            break;
    }

    return params;
}

std::string Parser::parseType()
{
    if(currentToken.type != TokenType::INT &&
       currentToken.type != TokenType::BOOL &&
       currentToken.type != TokenType::FLOAT &&
       currentToken.type != TokenType::STRING_TYPE &&
       currentToken.type != TokenType::VOID)
    {
        throw std::runtime_error("Expected type");
    }

    std::string type = currentToken.lexeme;
    advance();

    return type;
}

std::unique_ptr<ASTNode> Parser::parseComparison() {
    auto left = parseAdditive();

    while (currentToken.type == TokenType::GREATER ||
           currentToken.type == TokenType::LESS ||
           currentToken.type == TokenType::DOUBLE_EQUAL ||
           currentToken.type == TokenType::NOT_EQUAL ||
           currentToken.type == TokenType::LESS_EQUAL ||
           currentToken.type == TokenType::GREATER_EQUAL) {

        TokenType opToken = currentToken.type;
        advance();

        auto right = parseAdditive();

        left = std::make_unique<BinaryOpNode>(
            std::move(left),
            std::move(right),
            opToken
        );
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parseUnary() {
    if (currentToken.type == TokenType::NOT || currentToken.type == TokenType::MINUS) {

        TokenType op = currentToken.type;
        advance();

        auto operand = parseUnary();

        return std::make_unique<UnaryOpNode>(
            op,
            std::move(operand)
        );
    }

    return parseFactor();
}

std::unique_ptr<ASTNode> Parser::parseReturnStatement()
{
    expect(TokenType::RETURN, "Expected 'return'");

    auto expr = parseExpression();

    expect(TokenType::SEMICOLON, "Expected ';' after return");

    return std::make_unique<ReturnNode>(std::move(expr));
}


std::unique_ptr<ASTNode> Parser::parsePrint(bool newline)
{
    expect(newline ? TokenType::PRINTLN_MACRO : TokenType::PRINT_MACRO,
           "Expected print macro");

    expect(TokenType::LPAREN, "Expected '('");

    std::string format;
    if (currentToken.type == TokenType::STRING) {
        format = expect(TokenType::STRING, "Expected format string").lexeme;
    }

    // Count {} placeholders
    int placeholderCount = 0;
    size_t len = format.size();
    for (size_t i = 0; i < len; i++) {
        if (format[i] == '{' && i + 1 < len && format[i + 1] == '}') {
            placeholderCount++;
            i++;
        }
    }

    std::vector<std::unique_ptr<ASTNode>> args;
    if (placeholderCount > 0) {
        expect(TokenType::COMMA, "Expected ',' after format string");

        for (int i = 0; i < placeholderCount; i++) {
            args.push_back(parseExpression());
            if (i < placeholderCount - 1)
                expect(TokenType::COMMA, "Expected ',' between arguments");
        }
    }

    expect(TokenType::RPAREN, "Expected ')'");
    expect(TokenType::SEMICOLON, "Expected ';'");

    // Ensure exact argument count
    if (args.size() != placeholderCount) {
        throw std::runtime_error("Print argument count does not match placeholders");
    }

    return std::make_unique<PrintNode>(format, std::move(args), newline);
}