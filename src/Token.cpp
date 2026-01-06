#include "include/Token.h"


Token::~Token()
{

}
Token::Token(TokenType type, const std::string &lexeme, int line, int column) :
    m_type(type), m_lexeme(lexeme), m_line(line), m_column(column)
{
    
}
