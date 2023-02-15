#ifndef clox_scanner_h
#define clox_scanner_h

typedef enum {
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    TOKEN_LEFT_SQUARE_BRACKET, TOKEN_RIGHT_SQUARE_BRACKET, TOKEN_COLON, TOKEN_QUESTION,
    // One or two character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_EXPONENT,
    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,
    TOKEN_DEBUGGER, TOKEN_EXIT, TOKEN_FINAL, TOKEN_CONTINUE, TOKEN_BREAK,

    TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
    int col;
} Token;

class Scanner {
    public:
        Scanner(char* src);
        ~Scanner();
        Token scanToken();
private:
    char* start;
    char* current;
    int line;
    int col;

    void nextLine();
    void skipWhitespace();
    Token string();
    Token number();
    Token identifier();
    TokenType identifierType();
    TokenType checkKeyword(int offset, int length, const char *rest, TokenType type);
    Token errorToken(const char *message) const;
    Token makeToken(TokenType type);
    static bool isDigit(char c);
    static bool isAlpha(char c);bool isAtEnd();
    char advance();
    bool match(char expected);
    char peek();
    char peekNext();
};

#endif