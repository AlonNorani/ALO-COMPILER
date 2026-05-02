#include "Parser.h"

// Parser class: Builds an Abstract Syntax Tree (AST) from tokens using recursive descent.
// Handles syntax analysis and error reporting.
Parser::Parser(Lexer& lexer) : lexer(lexer)
{
    advance();
}

// Parses the entire program into a list of top-level statements.
std::vector<std::unique_ptr<ASTNode>> Parser::parseProgram()
{
    std::vector<std::unique_ptr<ASTNode>> nodes;
    while (currentToken.type != TokenType::EOF_TOKEN)
        nodes.push_back(parseStatement());
    return nodes;
}

// Checks if current token matches expected type and advances if so.
bool Parser::match(TokenType expectedType)
{
    if (currentToken.type == expectedType) { advance(); return true; }
    return false;
}

// Expects a specific token type, throws error if not found.
Token Parser::expect(TokenType expectedType, const std::string& errorMessage)
{
    Token prev = currentToken;
    if (!match(expectedType))
        throw std::runtime_error(
            "Syntax Error at line " + std::to_string(currentToken.line) +
            ", column " + std::to_string(currentToken.column) +
            ": " + errorMessage);
    return prev;
}

void Parser::advance() { currentToken = lexer.getNextToken(); }

// Dispatches to the appropriate statement parser based on current token.
std::unique_ptr<ASTNode> Parser::parseStatement()
{
    if (currentToken.type == TokenType::PRINT_MACRO)   return parsePrint(false);
    if (currentToken.type == TokenType::PRINTLN_MACRO) return parsePrint(true);
    if (currentToken.type == TokenType::INPUT_MACRO)   return parseInput();
    if (currentToken.type == TokenType::FN)            return parseFunction();
    if (currentToken.type == TokenType::RETURN)        return parseReturnStatement();
    if (currentToken.type == TokenType::LET)           return parseLetStatement();
    if (currentToken.type == TokenType::IF)            return parseIfStatement();
    if (currentToken.type == TokenType::WHILE)         return parseWhileStatement();
    if (currentToken.type == TokenType::FOR)           return parseForStatement();
    if (currentToken.type == TokenType::LBRACE)        return parseBlock();

    if (currentToken.type == TokenType::IDENTIFIER) {
        Token next = lexer.peekNextToken();
        if (next.type == TokenType::EQUAL)    return parseAssignment();
        if (next.type == TokenType::LBRACKET) return parseArrayAssignment();
        auto expr = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after expression");
        return expr;
    }

    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after statement");
    return expr;
}


std::unique_ptr<ASTNode> Parser::parseLetStatement()
{
    int line = currentToken.line; int col = currentToken.column; advance(); // consume 'let'

    bool isMutable = false;
    if (currentToken.type == TokenType::MUT) { isMutable = true; advance(); }

    if (currentToken.type != TokenType::IDENTIFIER)
        throw std::runtime_error("Expected variable name after 'let'");

    std::string varName = currentToken.lexeme;
    advance();

    auto node = std::make_unique<LetNode>(varName, nullptr, isMutable, line, col);

    if (currentToken.type == TokenType::COLON) {
        advance();
        node->hasTypeAnnotation = true;
        node->annotatedType     = parseType();

        if (currentToken.type == TokenType::LBRACKET) {
            advance();
            node->isArray = true;
            if (currentToken.type == TokenType::NUMBER) {
                node->arraySize = std::stoi(currentToken.lexeme);
                advance();
            }
            expect(TokenType::RBRACKET, "Expected ']' in array type");
        }
    }

    if (currentToken.type == TokenType::EQUAL) {
        advance();
        if (currentToken.type == TokenType::LBRACKET)
            node->value = parseArrayLiteral();
        else
            node->value = parseExpression();
    } else if (!node->isArray) {
        throw std::runtime_error(
            "Variable '" + varName + "' must have an initialiser");
    }

    expect(TokenType::SEMICOLON, "Expected ';' after let");
    return node;
}


std::unique_ptr<ASTNode> Parser::parseArrayLiteral()
{
    int line = currentToken.line; int col = currentToken.column; expect(TokenType::LBRACKET, "Expected '['");
    std::vector<std::unique_ptr<ASTNode>> elements;
    if (currentToken.type != TokenType::RBRACKET) {
        do { elements.push_back(parseExpression()); }
        while (match(TokenType::COMMA));
    }
    expect(TokenType::RBRACKET, "Expected ']'");
    return std::make_unique<ArrayLiteralNode>(std::move(elements), line, col);
}


std::unique_ptr<ASTNode> Parser::parseArrayAssignment()
{
    std::string name = currentToken.lexeme; int line = currentToken.line; int col = currentToken.column;
    advance();
    expect(TokenType::LBRACKET, "Expected '['");
    auto index = parseExpression();
    expect(TokenType::RBRACKET, "Expected ']'");

    if (currentToken.type == TokenType::EQUAL) {
        advance();
        auto val = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after array assignment");
        return std::make_unique<ArrayAssignNode>(name, std::move(index), std::move(val), line, col);
    }

    auto access = std::make_unique<ArrayAccessNode>(name, std::move(index), line, col);
    expect(TokenType::SEMICOLON, "Expected ';' after expression");
    return access;
}


std::unique_ptr<ASTNode> Parser::parseAssignment()
{
    std::string varName = currentToken.lexeme; int line = currentToken.line; int col = currentToken.column;
    advance();
    expect(TokenType::EQUAL, "Expected '='");
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';'");
    return std::make_unique<AssignmentNode>(varName, std::move(expr), line, col);
}


std::unique_ptr<ASTNode> Parser::parseExpression()  { return parseLogicalOr(); }

std::unique_ptr<ASTNode> Parser::parseLogicalOr()
{
    auto left = parseLogicalAnd();
    while (currentToken.type == TokenType::LOGICAL_OR) {
        TokenType op = currentToken.type; int line = currentToken.line; int col = currentToken.column; advance();
        left = std::make_unique<BinaryOpNode>(std::move(left), parseLogicalAnd(), op, line, col);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseLogicalAnd()
{
    auto left = parseComparison();
    while (currentToken.type == TokenType::LOGICAL_AND) {
        TokenType op = currentToken.type; int line = currentToken.line; int col = currentToken.column; advance();
        left = std::make_unique<BinaryOpNode>(std::move(left), parseComparison(), op, line, col);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseComparison()
{
    auto left = parseAdditive();
    while (currentToken.type == TokenType::GREATER    ||
           currentToken.type == TokenType::LESS        ||
           currentToken.type == TokenType::DOUBLE_EQUAL||
           currentToken.type == TokenType::NOT_EQUAL   ||
           currentToken.type == TokenType::LESS_EQUAL  ||
           currentToken.type == TokenType::GREATER_EQUAL) {
        TokenType op = currentToken.type; int line = currentToken.line; int col = currentToken.column; advance();
        left = std::make_unique<BinaryOpNode>(std::move(left), parseAdditive(), op, line, col);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseAdditive()
{
    auto left = parseTerm();
    while (currentToken.type == TokenType::PLUS || currentToken.type == TokenType::MINUS) {
        TokenType op = currentToken.type; int line = currentToken.line; int col = currentToken.column; advance();
        left = std::make_unique<BinaryOpNode>(std::move(left), parseTerm(), op, line, col);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseTerm()
{
    auto left = parseUnary();
    while (currentToken.type == TokenType::STAR || currentToken.type == TokenType::SLASH) {
        TokenType op = currentToken.type; int line = currentToken.line; int col = currentToken.column; advance();
        left = std::make_unique<BinaryOpNode>(std::move(left), parseUnary(), op, line, col);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseUnary()
{
    if (currentToken.type == TokenType::NOT || currentToken.type == TokenType::MINUS) {
        TokenType op = currentToken.type; advance();
        return std::make_unique<UnaryOpNode>(op, parseUnary(), currentToken.line, currentToken.column);
    }
    return parseFactor();
}

std::unique_ptr<ASTNode> Parser::parseFactor()
{
    if (currentToken.type == TokenType::INPUT_MACRO) {
        int line = currentToken.line; int col = currentToken.column; advance();
        expect(TokenType::LPAREN, "Expected '('");
        expect(TokenType::RPAREN, "Expected ')'");
        return std::make_unique<InputNode>(line, col);
    }
    if (currentToken.type == TokenType::AMP) {
        int line = currentToken.line; int col = currentToken.column; advance();
        bool isMut = false;
        if (currentToken.type == TokenType::MUT) { isMut = true; advance(); }
        if (currentToken.type != TokenType::IDENTIFIER)
            throw std::runtime_error("Expected variable name after '&'");
        std::string name = currentToken.lexeme; advance();
        return std::make_unique<BorrowNode>(name, isMut, line, col);
    }
    if (currentToken.type == TokenType::NUMBER) {
        const std::string lexeme = currentToken.lexeme; int line = currentToken.line; int col = currentToken.column;
        if (lexeme.find('.') != std::string::npos) {
            double val = std::stod(lexeme);
            advance();
            return std::make_unique<FloatNode>(val, line, col);
        }
        int val = std::stoi(lexeme);
        advance();
        return std::make_unique<NumberNode>(val, line, col);
    }
    if (currentToken.type == TokenType::STRING) {
        std::string val = currentToken.lexeme; int line = currentToken.line; int col = currentToken.column; advance();
        return std::make_unique<StringNode>(val, line, col);
    }
    if (currentToken.type == TokenType::IDENTIFIER) {
        std::string name = currentToken.lexeme; int line = currentToken.line; int col = currentToken.column; advance();
        // copy(varName) — deep copy for heap types
        if (name == "copy" && currentToken.type == TokenType::LPAREN) {
            advance();
            if (currentToken.type != TokenType::IDENTIFIER)
                throw std::runtime_error("Expected variable name in copy()");
            std::string copyName = currentToken.lexeme; advance();
            expect(TokenType::RPAREN, "Expected ')'");
            return std::make_unique<CopyNode>(copyName, line, col);
        }
        if (currentToken.type == TokenType::LPAREN) {
            advance();
            std::vector<std::unique_ptr<ASTNode>> args;
            if (currentToken.type != TokenType::RPAREN) {
                do { args.push_back(parseExpression()); }
                while (match(TokenType::COMMA));
            }
            expect(TokenType::RPAREN, "Expected ')'");
            return std::make_unique<CallNode>(name, std::move(args), line, col);
        }
        if (currentToken.type == TokenType::LBRACKET) {
            advance();
            auto index = parseExpression();
            expect(TokenType::RBRACKET, "Expected ']'");
            return std::make_unique<ArrayAccessNode>(name, std::move(index), line, col);
        }
        return std::make_unique<VariableNode>(name, line, col);
    }
    if (currentToken.type == TokenType::LPAREN) {
        int line = currentToken.line; int col = currentToken.column; advance();
        if (currentToken.type == TokenType::INT || currentToken.type == TokenType::FLOAT ||
            currentToken.type == TokenType::BOOL || currentToken.type == TokenType::STRING_TYPE) {
            std::string castType = currentToken.lexeme;
            advance();
            if (currentToken.type == TokenType::RPAREN) {
                advance();
                auto expr = parseFactor();
                return std::make_unique<CastNode>(castType, std::move(expr), line, col);
            }
            throw std::runtime_error("Invalid cast syntax");
        }
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }
    if (currentToken.type == TokenType::LBRACKET) return parseArrayLiteral();
    if (currentToken.type == TokenType::TRUE)  { int line = currentToken.line; int col = currentToken.column; advance(); return std::make_unique<BoolNode>(true, line, col);  }
    if (currentToken.type == TokenType::FALSE) { int line = currentToken.line; int col = currentToken.column; advance(); return std::make_unique<BoolNode>(false, line, col); }

    throw std::runtime_error(
        "Syntax Error at line " + std::to_string(currentToken.line) +
        ": unexpected token '" + currentToken.lexeme + "'");
}

//  Blocks / control flow / functions

std::unique_ptr<BlockNode> Parser::parseBlock()
{
    int line = currentToken.line; int col = currentToken.column; expect(TokenType::LBRACE, "Expected '{'");
    std::vector<std::unique_ptr<ASTNode>> stmts;
    while (currentToken.type != TokenType::RBRACE &&
           currentToken.type != TokenType::EOF_TOKEN)
        stmts.push_back(parseStatement());
    expect(TokenType::RBRACE, "Expected '}'");
    return std::make_unique<BlockNode>(std::move(stmts), line, col);
}

std::unique_ptr<IfNode> Parser::parseIfStatement()
{
    int line = currentToken.line; int col = currentToken.column; expect(TokenType::IF,     "Expected 'if'");
    expect(TokenType::LPAREN, "Expected '('");
    auto cond = parseExpression();
    expect(TokenType::RPAREN, "Expected ')'");
    auto thenBranch = parseBlock();
    std::unique_ptr<BlockNode> elseBranch;
    if (currentToken.type == TokenType::ELSE) {
        advance();
        if (currentToken.type == TokenType::IF) {
            auto nested = parseIfStatement();
            std::vector<std::unique_ptr<ASTNode>> stmts;
            stmts.push_back(std::move(nested));
            elseBranch = std::make_unique<BlockNode>(std::move(stmts), currentToken.line, currentToken.column); // approximate position
        } else {
            elseBranch = parseBlock();
        }
    }
    return std::make_unique<IfNode>(std::move(cond), std::move(thenBranch), std::move(elseBranch), line, col);
}

std::unique_ptr<WhileNode> Parser::parseWhileStatement()
{
    int line = currentToken.line; int col = currentToken.column; expect(TokenType::WHILE,  "Expected 'while'");
    expect(TokenType::LPAREN, "Expected '('");
    auto cond = parseExpression();
    expect(TokenType::RPAREN, "Expected ')'");
    return std::make_unique<WhileNode>(std::move(cond), parseBlock(), line, col);
}

std::unique_ptr<ASTNode> Parser::parseForStatement()
{
    int line = currentToken.line; int col = currentToken.column; expect(TokenType::FOR, "Expected 'for'");
    if (currentToken.type != TokenType::IDENTIFIER)
        throw std::runtime_error("Expected iterator name after 'for'");

    std::string iteratorName = currentToken.lexeme;
    advance();
    expect(TokenType::IN, "Expected 'in' after iterator name in for loop");

    auto iterable = parseExpression();
    std::unique_ptr<ASTNode> endExpr;
    if (currentToken.type == TokenType::RANGE) {
        advance();
        endExpr = parseExpression();
    }

    return std::make_unique<ForNode>(iteratorName, std::move(iterable), std::move(endExpr), parseBlock(), line, col);
}

std::unique_ptr<FunctionNode> Parser::parseFunction()
{
    int line = currentToken.line; int col = currentToken.column; expect(TokenType::FN, "Expected 'fn'");
    if (currentToken.type != TokenType::IDENTIFIER)
        throw std::runtime_error("Expected function name");
    std::string name = currentToken.lexeme; advance();
    expect(TokenType::LPAREN, "Expected '('");
    auto params = parseParameterList();
    expect(TokenType::RPAREN, "Expected ')'");
    expect(TokenType::ARROW,  "Expected '->'");
    std::string retType = parseType();
    return std::make_unique<FunctionNode>(name, std::move(params), retType, parseBlock(), line, col);
}


std::vector<Parameter> Parser::parseParameterList()
{
    std::vector<Parameter> params;
    if (currentToken.type == TokenType::RPAREN) return params;

    while (true) {
        if (currentToken.type != TokenType::IDENTIFIER)
            throw std::runtime_error("Expected parameter name");

        Parameter p;
        p.name = currentToken.lexeme;
        advance();

        expect(TokenType::COLON, "Expected ':' after parameter name");

        // Check for borrow prefix: & or &mut
        if (currentToken.type == TokenType::AMP) {
            advance();
            if (currentToken.type == TokenType::MUT) {
                p.borrowType = "mut";
                advance();
            } else {
                p.borrowType = "imm";
            }
        }

        p.type = parseType(); // "int", "bool", "string", …

        // Check for '[]' — array parameter
        if (currentToken.type == TokenType::LBRACKET) {
            advance(); // consume '['
            expect(TokenType::RBRACKET, "Expected ']' in array parameter type");
            p.isArray = true;
        }

        params.push_back(std::move(p));
        if (!match(TokenType::COMMA)) break;
    }

    return params;
}

std::string Parser::parseType()
{
    if (currentToken.type != TokenType::INT    &&
        currentToken.type != TokenType::BOOL   &&
        currentToken.type != TokenType::FLOAT  &&
        currentToken.type != TokenType::STRING_TYPE &&
        currentToken.type != TokenType::VOID)
        throw std::runtime_error("Expected type");
    std::string type = currentToken.lexeme; advance();
    return type;
}

std::unique_ptr<ASTNode> Parser::parseReturnStatement()
{
    int line = currentToken.line; int col = currentToken.column; expect(TokenType::RETURN, "Expected 'return'");
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';'");
    return std::make_unique<ReturnNode>(std::move(expr), line, col);
}

std::unique_ptr<ASTNode> Parser::parsePrint(bool newline)
{
    int line = currentToken.line; int col = currentToken.column; expect(newline ? TokenType::PRINTLN_MACRO : TokenType::PRINT_MACRO, "Expected print macro");
    expect(TokenType::LPAREN, "Expected '('");

    std::string format;
    if (currentToken.type == TokenType::STRING)
        format = expect(TokenType::STRING, "Expected format string").lexeme;

    int placeholders = 0;
    for (size_t i = 0; i + 1 < format.size(); i++)
        if (format[i] == '{' && format[i+1] == '}') { placeholders++; i++; }

    std::vector<std::unique_ptr<ASTNode>> args;
    if (placeholders > 0) {
        expect(TokenType::COMMA, "Expected ','");
        for (int i = 0; i < placeholders; i++) {
            args.push_back(parseExpression());
            if (i < placeholders - 1) expect(TokenType::COMMA, "Expected ','");
        }
    }

    expect(TokenType::RPAREN,    "Expected ')'");
    expect(TokenType::SEMICOLON, "Expected ';'");
    return std::make_unique<PrintNode>(format, std::move(args), newline, line, col);
}

std::unique_ptr<ASTNode> Parser::parseInput() {
    int line = currentToken.line; int col = currentToken.column; expect(TokenType::INPUT_MACRO, "Expected input macro");
    expect(TokenType::LPAREN, "Expected '(' after 'input!'");
    expect(TokenType::RPAREN, "Expected ')' after '('");
    expect(TokenType::SEMICOLON, "Expected ';' after 'input!()'");
    return std::make_unique<InputNode>(line, col);
}
