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
    Lexer& lexer; // reference to a Lexer instance
    Token currentToken;
    bool match(TokenType expectedType);
    void expect(TokenType expectedType, const std::string& errorMessage);
    void advance();

    // parse functions for different grammar rules
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseLogicalOr();
    std::unique_ptr<ASTNode> parseLogicalAnd();
    std::unique_ptr<ASTNode> parseComparison();
    std::unique_ptr<ASTNode> parseAdditive();
    std::unique_ptr<ASTNode> parseTerm();
    std::unique_ptr<ASTNode> parseUnary();
    std::unique_ptr<ASTNode> parseFactor();
    // assignments and variable declarations
    std::unique_ptr<ASTNode> parseAssignment(); // for simple assignments like x = 5;
    std::unique_ptr<ASTNode> parseLetStatement(); // for variable declarations with 'let' keyword
    // parse blcoks and control flow statements
    std::unique_ptr<BlockNode> parseBlock();       
    std::unique_ptr<IfNode> parseIfStatement();  
    std::unique_ptr<WhileNode> parseWhileStatement(); 
    //functions
    std::unique_ptr<FunctionNode> parseFunction();
    std::vector<Parameter> parseParameterList();
    std::string parseType();
    std::unique_ptr<ASTNode> parseReturnStatement();




};

#endif