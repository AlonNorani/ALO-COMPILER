#include "../include/Lexer.h"

Lexer::Lexer(const std::string &source) : m_source(source), m_pos(0), m_line(1), m_column(1)
{  }

Token Lexer::getNextToken()
{
    skipWhitespace();

    char currChar = currentChar();

    if (currChar == '\0') {
        return Token{TokenType::EOF_TOKEN, "", m_line, m_column};
    }
    else if(isdigit(currChar)) {
        return lexNumber();
    }
    else if(isalpha(currChar) || currChar == '_') {
        return lexIdentifierOrKeyword();
    }
    else if(currChar == '"') {
        return lexString();
    }

    // Placeholder for unrecognized characters
    Token invalidToken;
    invalidToken.m_type = TokenType::INVALID_TOKEN;
    invalidToken.m_lexeme = std::string(1, currChar);
    invalidToken.m_line = m_line;
    invalidToken.m_column = m_column;
    advance();
    return invalidToken;
}

char Lexer::currentChar() const
{
    if (m_pos >= m_source.size()) {
        return '\0'; // null char to indicate end of source
    }
    return m_source[m_pos];
}

char Lexer::peekChar() const
{
    if (m_pos + 1 >= m_source.size()) {
        return '\0'; // null char to indicate end of source
    }
    return m_source[m_pos + 1];
    
}

void Lexer::advance()
{
    if(currentChar() == '\n') { // indicate new line
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    m_pos++;
}

void Lexer::skipWhitespace()
{
    while (isspace(currentChar())) {
        advance();
    }
}

Token Lexer::lexNumber()
{
    
}

Token Lexer::lexIdentifierOrKeyword()
{
    
}

Token Lexer::lexString()
{
    
}

Token Lexer::lexOperatorOrDelimiter()
{
    
}
