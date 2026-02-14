#include "Lexer.h"

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
    else if(operators.find(currChar) != std::string::npos) { // check if char is an operator
        return lexOperatorOrDelimiter(false);
    }
    else if(delimiters.find(currChar) != std::string::npos) { // check if char is a delimiter
        return lexOperatorOrDelimiter(true);
    }
    else if(currChar == '"') {
        return lexString();
    }
    // Placeholder for unrecognized characters
    Token invalidToken;
    invalidToken.type = TokenType::INVALID_TOKEN;
    invalidToken.lexeme = std::string(1, currChar);
    invalidToken.line = m_line;
    invalidToken.column = m_column;
    advance();
    return invalidToken;
}

Token Lexer::peekNextToken()
{
    // save current lexer state
    size_t savedPos = m_pos;
    int savedLine = m_line;
    int savedColumn = m_column;

    // Get the next token
    Token nextToken = getNextToken();

    // Restore the state
    m_pos = savedPos;
    m_line = savedLine;
    m_column = savedColumn;

    return nextToken;
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
    int startLine = m_line;
    int startColumn = m_column;

    std::string value;
    bool hasDot = false;

    while (isdigit(currentChar()) || currentChar() == '.') {
        if (currentChar() == '.') {
            if (hasDot) {
                // second dot - invalid number
                return Token{
                    TokenType::INVALID_TOKEN,
                    value,
                    startLine,
                    startColumn
                };
            }
            hasDot = true;
        }

        value += currentChar();
        advance();
    }

    return Token{
        TokenType::NUMBER,
        value,
        startLine,
        startColumn
    };
}


Token Lexer::lexIdentifierOrKeyword()
{
    std::string value;
    int startLine = m_line;
    int startColumn = m_column;
    while (isalnum(currentChar()) || currentChar() == '_') { // char can be alphanumeric or underscore
        value += currentChar();
        advance();
    }
    // check if the identifier is a keyword
    auto keywordIt = keywords.find(value);
    if (keywordIt != keywords.end()) {
        return Token{
            keywordIt->second,
            value,
            startLine,
            startColumn,
        };
    }
    auto dataTypeIt = dataTypeKeywords.find(value);
    if (dataTypeIt != dataTypeKeywords.end()) {
        return Token{
            dataTypeIt->second,
            value,
            startLine,
            startColumn,
        };
    }
    return Token{
        TokenType::IDENTIFIER, // not a keyword, so it's an identifier for example variable name
        value,
        startLine,
        startColumn,
    };
}

Token Lexer::lexString()
{
    int startLine = m_line; 
    int startColumn = m_column;
    std::string value;
    advance(); // skip the opening quote
    while (currentChar() != '"' && currentChar() != '\0') {
        value += currentChar();
        advance();
    }
    if (currentChar() == '"') {
        advance(); // skip the closing quote
        return Token{
            TokenType::STRING,
            value,
            startLine,
            startColumn
        };
    } else {
        // Unterminated string literal
        return Token{
            TokenType::INVALID_TOKEN,
            value,
            startLine,
            startColumn
        };
    }
}

Token Lexer::lexOperatorOrDelimiter(bool isDelimiter)
{
    int startLine = m_line;
    int startColumn = m_column;

    char currChar = currentChar();
    if (currChar == '\0') {
        return Token{TokenType::EOF_TOKEN, "", startLine, startColumn};
    }

    std::string value;
    value += currChar;

    if (!isDelimiter) { // operator
        // Check for double-character operator
        char nextChar = peekChar();
        std::string doubleCharOp = value + nextChar;
        auto doubleIt = doubleCharOperatorTokens.find(doubleCharOp);
        if (doubleIt != doubleCharOperatorTokens.end()) {
            advance(); // consume first char
            advance(); // consume second char
            return Token{doubleIt->second, doubleCharOp, startLine, startColumn};
        }

        // Single-character operator
        auto singleIt = singleCharOperatorTokens.find(currChar);
        if (singleIt != singleCharOperatorTokens.end()) {
            advance(); // consume operator
            return Token{singleIt->second, value, startLine, startColumn};
        }
    } 
    else { // delimiter
        TokenType type;
        switch (currChar) {
            case '(': type = TokenType::LPAREN; break;
            case ')': type = TokenType::RPAREN; break;
            case '{': type = TokenType::LBRACE; break;
            case '}': type = TokenType::RBRACE; break;
            case ';': type = TokenType::SEMICOLON; break;
            case ',': type = TokenType::COMMA; break;
            case ':': type = TokenType::COLON; break;
            default: type = TokenType::INVALID_TOKEN; break;
        }
        advance(); // consume delimiter
        return Token{type, value, startLine, startColumn};
    }

    // If we reach here, it's an invalid operator/delimiter
    advance(); // consume the invalid char to avoid infinite loop
    return Token{TokenType::INVALID_TOKEN, value, startLine, startColumn};
}
