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
    Lexer& lexer;
    Token  currentToken;

    bool  match(TokenType expectedType);
    Token expect(TokenType expectedType, const std::string& errorMessage);
    void  advance();

    // Statements
    std::unique_ptr<ASTNode>      parseStatement();
    std::unique_ptr<ASTNode>      parseLetStatement();
    std::unique_ptr<ASTNode>      parseAssignment();      // x = expr;
    std::unique_ptr<ASTNode>      parseArrayAssignment(); // x[i] = expr;  (NEW)
    std::unique_ptr<BlockNode>    parseBlock();
    std::unique_ptr<IfNode>       parseIfStatement();
    std::unique_ptr<WhileNode>    parseWhileStatement();
    std::unique_ptr<FunctionNode> parseFunction();
    std::unique_ptr<ASTNode>      parseReturnStatement();
    std::unique_ptr<ASTNode>      parsePrint(bool newline);

    // Expressions
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseLogicalOr();
    std::unique_ptr<ASTNode> parseLogicalAnd();
    std::unique_ptr<ASTNode> parseComparison();
    std::unique_ptr<ASTNode> parseAdditive();
    std::unique_ptr<ASTNode> parseTerm();
    std::unique_ptr<ASTNode> parseUnary();
    std::unique_ptr<ASTNode> parseFactor();     // handles arr[i] reads too

    // Array literal  [e1, e2, …]  (NEW)
    std::unique_ptr<ASTNode> parseArrayLiteral();

    // Helpers
    std::vector<Parameter> parseParameterList();
    std::string            parseType();
};

#endif
