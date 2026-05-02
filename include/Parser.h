#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <stdexcept>
#include "Lexer.h"
#include <memory>
#include "ASTNodes.h"
#include <vector>

/**
 * @file Parser.h
 * @brief Defines the Parser class that builds an AST from tokens.
 *
 * The parser implements a recursive descent parser that converts the token
 * stream from the lexer into an Abstract Syntax Tree (AST) representation
 * of the ALO program.
 */

/**
 * @class Parser
 * @brief Parses ALO source code tokens into an Abstract Syntax Tree.
 *
 * Uses recursive descent parsing to build AST nodes from the token stream.
 * Handles operator precedence, statement parsing, and expression evaluation.
 */
class Parser
{
public:
    /**
     * @brief Construct a parser with the given lexer.
     * @param lexer The lexer providing tokens to parse
     */
    explicit Parser(Lexer& lexer);

    ~Parser() = default;

    /**
     * @brief Parse the entire program into a list of AST nodes.
     * @return Vector of top-level statements/expressions
     */
    std::vector<std::unique_ptr<ASTNode>> parseProgram();

private:
    Lexer& lexer;         ///< Reference to the token source
    Token  currentToken;  ///< The current token being processed

    // Token management
    bool  match(TokenType expectedType);  ///< Check if current token matches expected type
    Token expect(TokenType expectedType, const std::string& errorMessage);  ///< Consume expected token or error
    void  advance();  ///< Move to the next token

    // Statement parsing
    std::unique_ptr<ASTNode>      parseStatement();       ///< Parse any statement
    std::unique_ptr<ASTNode>      parseLetStatement();    ///< Parse variable declarations
    std::unique_ptr<ASTNode>      parseAssignment();      ///< Parse variable assignments
    std::unique_ptr<ASTNode>      parseArrayAssignment(); ///< Parse array element assignments
    std::unique_ptr<BlockNode>    parseBlock();           ///< Parse statement blocks
    std::unique_ptr<IfNode>       parseIfStatement();     ///< Parse if/else statements
    std::unique_ptr<WhileNode>    parseWhileStatement();  ///< Parse while loops
    std::unique_ptr<ASTNode>      parseForStatement();    ///< Parse for loops
    std::unique_ptr<FunctionNode> parseFunction();        ///< Parse function definitions
    std::unique_ptr<ASTNode>      parseReturnStatement(); ///< Parse return statements
    std::unique_ptr<ASTNode>      parsePrint(bool newline); ///< Parse print statements
    std::unique_ptr<ASTNode>      parseInput();            ///< Parse input statements

    // Expression parsing (with precedence)
    std::unique_ptr<ASTNode> parseExpression();   ///< Parse any expression
    std::unique_ptr<ASTNode> parseLogicalOr();    ///< Parse logical OR expressions
    std::unique_ptr<ASTNode> parseLogicalAnd();   ///< Parse logical AND expressions
    std::unique_ptr<ASTNode> parseComparison();   ///< Parse comparison expressions
    std::unique_ptr<ASTNode> parseAdditive();     ///< Parse addition/subtraction
    std::unique_ptr<ASTNode> parseTerm();         ///< Parse multiplication/division
    std::unique_ptr<ASTNode> parseUnary();        ///< Parse unary operators
    std::unique_ptr<ASTNode> parseFactor();       ///< Parse primary expressions

    std::unique_ptr<ASTNode> parseArrayLiteral(); ///< Parse array literals

    // Helper functions
    std::vector<Parameter> parseParameterList();  ///< Parse function parameter lists
    std::string            parseType();           ///< Parse type annotations
};

#endif
