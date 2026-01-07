#include "include/Lexer.h"


Lexer::~Lexer()
{

}
Lexer::Lexer(const std::string &source) : m_source(source), m_pos(0), m_line(1), m_column(1)
{  }
