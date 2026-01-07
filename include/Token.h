#ifndef TOKEN_H
#define TOKEN_H
#include <string>
//enum of all tokens
enum class TokenType
{
    // special tokens
    EOF_TOKEN,
    INVALID_TOKEN,
     // Identifiers and literals
    IDENTIFIER,
    NUMBER,
    STRING,
    // Boolean literals 
    TRUE,
    FALSE,
    // Keywords
    LET, // let is for variable declaration
    FN,
    IF,
    ELSE,
    RETURN,
    // Operators
    PLUS,            
    MINUS,            
    STAR,             
    SLASH,            
    EQUAL,            
    DOUBLE_EQUAL,     
    NOT_EQUAL,        
    LESS,             
    LESS_EQUAL,       
    GREATER,          
    GREATER_EQUAL,    
    // Delimiters
    LPAREN,          
    RPAREN,           
    LBRACE,           
    RBRACE,          
    SEMICOLON,        
    COMMA,            


};

typedef struct Token
{
    TokenType m_type;
    std::string m_lexeme;
    int m_line;           // Line number where token starts (for errors)
    int m_column;  
};

#endif