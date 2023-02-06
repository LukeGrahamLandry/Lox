#include <cstring>
#include "scanner.h"
#include "common.h"

Scanner::Scanner(char* src){
    start = src;
    current = start;
    line = 0;
}

Scanner::~Scanner(){

}

Token Scanner::scanToken() {
    skipWhitespace();
    start = current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);
    char c = advance();

    if (isDigit(c)) return number();
    if (isAlpha(c)) return identifier();

    #define SINGLE_TOKEN(expect, type)  \
            case expect:                \
                return makeToken(type); \

    #define DOUBLE_TOKEN(firstLexeme, secondLexeme, first, second)                 \
            case firstLexeme:                                                      \
                return match(secondLexeme) ? makeToken(second) : makeToken(first); \

    switch (c) {
        SINGLE_TOKEN('(', TOKEN_LEFT_PAREN)
        SINGLE_TOKEN(')', TOKEN_RIGHT_PAREN)
        SINGLE_TOKEN('{', TOKEN_LEFT_BRACE)
        SINGLE_TOKEN('}', TOKEN_RIGHT_BRACE)
        SINGLE_TOKEN(';', TOKEN_SEMICOLON)
        SINGLE_TOKEN(',', TOKEN_COMMA)
        SINGLE_TOKEN('.', TOKEN_DOT)
        SINGLE_TOKEN('-', TOKEN_MINUS)
        SINGLE_TOKEN('+', TOKEN_PLUS)
        SINGLE_TOKEN('/', TOKEN_SLASH)
        DOUBLE_TOKEN('*', '*', TOKEN_STAR, TOKEN_EXPONENT)
        DOUBLE_TOKEN('!', '=', TOKEN_BANG, TOKEN_BANG_EQUAL)
        DOUBLE_TOKEN('=', '=', TOKEN_EQUAL, TOKEN_EQUAL_EQUAL)
        DOUBLE_TOKEN('<', '=', TOKEN_LESS, TOKEN_LESS_EQUAL)
        DOUBLE_TOKEN('>', '=', TOKEN_GREATER, TOKEN_GREATER_EQUAL)

        case '"':
            return string();


        default:
            break;
    }

    #undef SINGLE_TOKEN
    #undef DOUBLE_TOKEN

    return errorToken("Unexpected character.");
}

void Scanner::skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                advance();
                line++;
                break;
            case '/':  // TODO: block comments
                if (peekNext() == '/'){
                    while (peek() != '\n' && !isAtEnd()){
                        advance();
                    }
                } else {
                    return;
                }
            default:
                return;
        }
    }
}

Token Scanner::string() {
    while (peek() != '"' && !isAtEnd()){
        if (peek() == '\n') line++;
        // TODO: support string interpolation
        //       "Hello ${code} \${escape}" gets parsed as "Hello " + code + " ${escape}"
        //       so i'd have to do some sort of recursion thing
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated string.");

    advance(); // closing "
    return makeToken(TOKEN_STRING);
}

Token Scanner::number() {
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peekNext())) {
        advance();
        while (isDigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}


Token Scanner::identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

TokenType Scanner::identifierType(){
    #define CHECK(first, offset, length, rest, type) case first: return checkKeyword(offset, length, rest, type);
    #define KEYWORD(first, length, rest, type) CHECK(first, 1, length, rest, type)
    #define LEAF(first, length, rest, type) CHECK(first, 2, length, rest, type)

    #define START_BRANCH(first)        \
        case first:                    \
            if (current - start > 1){  \
                switch (start[1]){     \

    #define END_BRANCH } } else break;

    switch (*start){
        KEYWORD('a', 2, "nd", TOKEN_AND)
        KEYWORD('c', 4, "lass", TOKEN_CLASS)
        KEYWORD('e', 3, "lse", TOKEN_ELSE)
        KEYWORD('i', 1, "f", TOKEN_IF)
        KEYWORD('n', 2, "il", TOKEN_NIL)
        KEYWORD('o', 1, "r", TOKEN_OR)
        KEYWORD('p', 4, "rint", TOKEN_PRINT)
        KEYWORD('r', 5, "eturn", TOKEN_RETURN)
        KEYWORD('s', 4, "uper", TOKEN_SUPER)
        KEYWORD('v', 2, "ar", TOKEN_VAR)
        KEYWORD('w', 4, "hile", TOKEN_WHILE)
        START_BRANCH('f')
            LEAF('a', 3, "lse", TOKEN_FALSE)
            LEAF('o', 1, "r", TOKEN_FOR)
            LEAF('u', 1, "n", TOKEN_FUN)
        END_BRANCH
        START_BRANCH('t')
            LEAF('h', 2, "is", TOKEN_THIS)
            LEAF('r', 2, "ue", TOKEN_TRUE)
        END_BRANCH
    }

    #undef CHECK
    #undef KEYWORD
    #undef LEAF
    #undef START_BRANCH
    #undef END_BRANCH
    return TOKEN_IDENTIFIER;
}

// Once we found the prefix of a keyword, that could only be one option, compare the rest of the identifier.
// If they don't match, it's just a regular identifier.
TokenType Scanner::checkKeyword(int offset, int length, const char* rest, TokenType type) {
    // Compare the text we've scanned to the text of the possible keyword.
    // If they're the same length and the values in memory are the same, the strings are the same.
    // Without the length check, "var" would match "variable", etc.
    // Note: strcmp is expects null terminated so wouldn't work here.
    if (current - start == offset + length && memcmp(start + offset, rest, length) == 0) return type;
    else return TOKEN_IDENTIFIER;
}

bool Scanner::isDigit(char c){
    return c >= '0' && c <= '9';
}

bool Scanner::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

Token Scanner::errorToken(const char *message) const {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = line;
    return token;
}

Token Scanner::makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = start;
    token.length = (int)(current - start);
    token.line = line;
    return token;
}

bool Scanner::isAtEnd() {
    return peek() == '\0';
}

char Scanner::advance() {
    current++;
    return current[-1];
}

char Scanner::peek(){
    return *current;
}

char Scanner::peekNext() {
    if (isAtEnd()) return '\0';
    return current[1];
}

bool Scanner::match(char expected){
    if (isAtEnd()) return false;
    if (*current == expected){
        advance();
        return true;
    }
    return false;
}
