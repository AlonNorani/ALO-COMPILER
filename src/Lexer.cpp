#include "Lexer.h"

// Lexer class: Converts source code string into a sequence of tokens.
// Tracks position, line, and column for error reporting.
Lexer::Lexer(const std::string &source)
    : m_source(source), m_pos(0), m_line(1), m_column(1)
{}

// Main tokenization function: Analyzes the next character and returns the appropriate token.
Token Lexer::getNextToken()
{
    skipWhitespace();

    char currChar = currentChar();

    if (currChar == '\0')
        return Token{TokenType::EOF_TOKEN, "", m_line, m_column};
    else if (isdigit(currChar))
        return lexNumber();
    else if (isalpha(currChar) || currChar == '_')
        return lexIdentifierOrKeyword();
    else if (operators.find(currChar) != std::string::npos)
        return lexOperatorOrDelimiter(false);
    else if (delimiters.find(currChar) != std::string::npos)
        return lexOperatorOrDelimiter(true);
    else if (currChar == '"')
        return lexString();

    Token invalid;
    invalid.type   = TokenType::INVALID_TOKEN;
    invalid.lexeme = std::string(1, currChar);
    invalid.line   = m_line;
    invalid.column = m_column;
    advance();
    return invalid;
}

Token Lexer::peekNextToken()
{
    size_t savedPos    = m_pos;
    int    savedLine   = m_line;
    int    savedColumn = m_column;

    Token next = getNextToken();

    m_pos    = savedPos;
    m_line   = savedLine;
    m_column = savedColumn;

    return next;
}

char Lexer::currentChar() const
{
    return (m_pos < m_source.size()) ? m_source[m_pos] : '\0';
}

char Lexer::peekChar() const
{
    return (m_pos + 1 < m_source.size()) ? m_source[m_pos + 1] : '\0';
}

void Lexer::advance()
{
    if (currentChar() == '\n') {
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    m_pos++;
}

void Lexer::skipWhitespace()
{
    while (isspace(currentChar()))
        advance();
}

Token Lexer::lexNumber()
{
    int startLine   = m_line;
    int startColumn = m_column;
    std::string value;
    bool hasDot = false;

    while (isdigit(currentChar()) ||
          (currentChar() == '.' && !hasDot && peekChar() != '.')) {
        if (currentChar() == '.') {
            if (hasDot)
                return Token{TokenType::INVALID_TOKEN, value, startLine, startColumn};
            hasDot = true;
        }
        value += currentChar();
        advance();
    }

    return Token{TokenType::NUMBER, value, startLine, startColumn};
}

Token Lexer::lexIdentifierOrKeyword()
{
    std::string value;
    int startLine   = m_line;
    int startColumn = m_column;

    while (isalnum(currentChar()) || currentChar() == '_') {
        value += currentChar();
        advance();
    }

    // Check for print! / println! macros
    if (currentChar() == '!') {
        if (value == "print") {
            advance();
            return Token{TokenType::PRINT_MACRO, "print!", startLine, startColumn};
        }
        if (value == "println") {
            advance();
            return Token{TokenType::PRINTLN_MACRO, "println!", startLine, startColumn};
        }
        if (value == "input") {
            advance();
            return Token{TokenType::INPUT_MACRO, "input!", startLine, startColumn};
        }
    }

    auto kwIt = keywords.find(value);
    if (kwIt != keywords.end())
        return Token{kwIt->second, value, startLine, startColumn};

    auto dtIt = dataTypeKeywords.find(value);
    if (dtIt != dataTypeKeywords.end())
        return Token{dtIt->second, value, startLine, startColumn};

    return Token{TokenType::IDENTIFIER, value, startLine, startColumn};
}

Token Lexer::lexString()
{
    int startLine   = m_line;
    int startColumn = m_column;
    std::string value;
    advance(); // skip opening "

    while (currentChar() != '"' && currentChar() != '\0') {
        value += currentChar();
        advance();
    }

    if (currentChar() == '"') {
        advance();
        return Token{TokenType::STRING, value, startLine, startColumn};
    }
    return Token{TokenType::INVALID_TOKEN, value, startLine, startColumn};
}

Token Lexer::lexOperatorOrDelimiter(bool isDelimiter)
{
    int  startLine   = m_line;
    int  startColumn = m_column;
    char currChar    = currentChar();

    if (currChar == '\0')
        return Token{TokenType::EOF_TOKEN, "", startLine, startColumn};

    std::string value(1, currChar);

    if (!isDelimiter) {
        // Try two-character operator first
        std::string doubleOp = value + peekChar();
        auto dblIt = doubleCharOperatorTokens.find(doubleOp);
        if (dblIt != doubleCharOperatorTokens.end()) {
            advance();
            advance();
            return Token{dblIt->second, doubleOp, startLine, startColumn};
        }

        auto sglIt = singleCharOperatorTokens.find(currChar);
        if (sglIt != singleCharOperatorTokens.end()) {
            advance();
            return Token{sglIt->second, value, startLine, startColumn};
        }
    } else {
        // Delimiter — look up in the map (handles [ and ] automatically)
        auto delIt = delimiterTokens.find(currChar);
        if (delIt != delimiterTokens.end()) {
            advance();
            return Token{delIt->second, value, startLine, startColumn};
        }
    }

    advance();
    return Token{TokenType::INVALID_TOKEN, value, startLine, startColumn};
}
