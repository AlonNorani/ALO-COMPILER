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
    MUT, // mut is for mutable variable declaration
    FN,
    ARROW, // for function return type annotation
    IF,
    ELSE,
    RETURN,
    // binary operators
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
    LOGICAL_AND,
    LOGICAL_OR,   
    // Unary operators
    NOT,

    // Delimiters
    LPAREN,          
    RPAREN,           
    LBRACE,           
    RBRACE,          
    SEMICOLON,        
    COMMA, 
    COLON, 
    // control flow
    WHILE,
    FOR,          
    // printing
    PRINT_MACRO,
    PRINTLN_MACRO,


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
    {"mut", TokenType::MUT},
    {"fn", TokenType::FN},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"return", TokenType::RETURN},
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"print!", TokenType::PRINT_MACRO},
    {"println!", TokenType::PRINTLN_MACRO}
};
static std::unordered_map<char, TokenType> singleCharOperatorTokens = {
    {'+', TokenType::PLUS},
    {'-', TokenType::MINUS},
    {'*', TokenType::STAR},
    {'/', TokenType::SLASH},
    {'=', TokenType::EQUAL},
    {'<', TokenType::LESS},
    {'>', TokenType::GREATER},
    {'!', TokenType::NOT},

};
static std::unordered_map<std::string, TokenType> doubleCharOperatorTokens = {
    {"==", TokenType::DOUBLE_EQUAL},
    {"!=", TokenType::NOT_EQUAL},
    {"<=", TokenType::LESS_EQUAL},
    {">=", TokenType::GREATER_EQUAL},
    {"&&", TokenType::LOGICAL_AND},
    {"||", TokenType::LOGICAL_OR},
    {"->", TokenType::ARROW},
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