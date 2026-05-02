#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <iostream>
#include "Token.h"

/**
 * @file Lexer.h
 * @brief Defines the Lexer class that tokenizes ALO source code.
 *
 * The lexer converts a stream of characters into a stream of tokens that
 * represent the syntactic elements of the language (keywords, operators,
 * identifiers, literals, etc.).
 */

/**
 * @class Lexer
 * @brief Tokenizes ALO source code into a sequence of tokens.
 *
 * The lexer reads the source code character by character and groups them
 * into meaningful tokens. It handles whitespace skipping, comment processing,
 * and various token types including keywords, operators, and literals.
 */
class Lexer
{
public:
    /**
     * @brief Construct a lexer for the given source code.
     * @param source The ALO source code to tokenize
     */
    explicit Lexer(const std::string &source);

    /**
     * @brief Get the next token from the source code.
     * @return The next token, advancing the lexer's position
     */
    Token getNextToken();

    /**
     * @brief Peek at the next token without consuming it.
     * @return The next token without advancing the lexer's position
     */
    Token peekNextToken();

private:
    const std::string m_source;  ///< The source code being tokenized
    size_t m_pos;                ///< Current position in the source
    int m_line;                  ///< Current line number for error reporting
    int m_column;                ///< Current column number for error reporting

    // Helper functions for character access and movement
    char currentChar() const;    ///< Get the current character
    char peekChar() const;       ///< Peek at the next character without advancing
    void advance();              ///< Move to the next character
    void skipWhitespace();       ///< Skip whitespace and comments

    // Tokenization functions for different token types
    Token lexNumber();                          ///< Tokenize numeric literals
    Token lexIdentifierOrKeyword();             ///< Tokenize identifiers and keywords
    Token lexString();                          ///< Tokenize string literals
    Token lexOperatorOrDelimiter(bool isDelimiter = false);  ///< Tokenize operators/delimiters
};

#endif