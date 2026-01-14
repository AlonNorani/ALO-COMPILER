#ifndef TOKEN_H
#define TOKEN_H
#include <string>
#include <unordered_map>
//enum of all tokens
enum class TokenType
{
    // general Types
    INT, 
    FLOAT,
    STRING_TYPE,
    BOOL,
    VOID,
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
    COLON,           


};
const std::string operators = "+-*/=<>!&|"; // set of operator characters
const std::string delimiters = "();,{}[]:"; // set of delimiter characters
const std::unordered_map<std::string, TokenType> dataTypeKeywords = {
    {"int", TokenType::INT},
    {"float", TokenType::FLOAT},
    {"string", TokenType::STRING_TYPE},
    {"bool", TokenType::BOOL},
    {"void", TokenType::VOID}
};
static std::unordered_map<std::string, TokenType> keywords = {
    {"let", TokenType::LET},
    {"fn", TokenType::FN},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"return", TokenType::RETURN}
};
static std::unordered_map<char, TokenType> singleCharOperatorTokens = {
    {'+', TokenType::PLUS},
    {'-', TokenType::MINUS},
    {'*', TokenType::STAR},
    {'/', TokenType::SLASH},
    {'=', TokenType::EQUAL},
    {'<', TokenType::LESS},
    {'>', TokenType::GREATER}
};
static std::unordered_map<std::string, TokenType> doubleCharOperatorTokens = {
    {"==", TokenType::DOUBLE_EQUAL},
    {"!=", TokenType::NOT_EQUAL},
    {"<=", TokenType::LESS_EQUAL},
    {">=", TokenType::GREATER_EQUAL}
};
static std::unordered_map<char, TokenType> delimiterTokens = {
    {'(', TokenType::LPAREN},
    {')', TokenType::RPAREN},
    {'{', TokenType::LBRACE},
    {'}', TokenType::RBRACE},
    {';', TokenType::SEMICOLON},
    {',', TokenType::COMMA}
};
struct Token
{
    TokenType type;
    std::string lexeme;
    int line;           // Line number where token starts (for errors)
    int column;  
};

#endif