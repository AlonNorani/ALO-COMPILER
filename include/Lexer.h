#ifndef LEXER_H
#define LEXER_H
#include <string>

class Lexer
{
public:
    explicit Lexer(const std::string &source);
    ~Lexer();

private:
    const std::string m_source;
    size_t m_pos;
    int m_line;
    int m_column;

};

#endif