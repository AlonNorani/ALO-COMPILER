#ifndef LEXER_H
#define LEXER_H
#include <string>
#include "Token.h"
class Lexer
{
public:
    // expicit constructor to prevent implicit conversions
    explicit Lexer(const std::string &source); 
    // function to get the next token from the source code
    Token getNextToken();

private:
    const std::string m_source;
    size_t m_pos;
    int m_line; // to track line numbers for error reporting
    int m_column; // to track column numbers for error reporting

    // helper functions
    char currentChar() const;
    char peekChar() const;

    void advance();
    void skipWhitespace();

    Token lexNumber();
    Token lexIdentifierOrKeyword();
    Token lexString();
    Token lexOperatorOrDelimiter();

};

#endif