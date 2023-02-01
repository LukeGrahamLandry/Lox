from curses.ascii import isalpha, isdigit
from math import fabs
from Token import Token, TokenType, MatchRule


require_closing_nested_comments = False  # True is like Dart. False is like C/Java/JS.

tokenChars = {
    "(": TokenType.LEFT_PAREN,
    ")": TokenType.RIGHT_PAREN,
    "{": TokenType.LEFT_BRACE,
    "}": TokenType.RIGHT_BRACE,
    ",": TokenType.COMMA,
    ".": TokenType.DOT,
    "-": TokenType.MINUS,
    "+": TokenType.PLUS,
    ";": TokenType.SEMICOLON,
    "*": TokenType.STAR,
}

doubleCharTokens = {
    "!": MatchRule("=", TokenType.BANG_EQUAL, TokenType.EQUAL),
    "=": MatchRule("=", TokenType.EQUAL_EQUAL, TokenType.EQUAL),
    "<": MatchRule("=", TokenType.LESS_EQUAL, TokenType.LESS),
    ">": MatchRule("=", TokenType.GREATER_EQUAL, TokenType.GREATER)
}

keywords = {
    "and": TokenType.AND,
    "class": TokenType.CLASS,
    "else": TokenType.ELSE,
    "false": TokenType.FALSE,
    "for": TokenType.FOR,
    "fun": TokenType.FUN,
    "if": TokenType.IF,
    "nil": TokenType.NIL,
    "or": TokenType.OR,
    "print": TokenType.PRINT,
    "return": TokenType.RETURN,
    "super": TokenType.SUPER,
    "this": TokenType.THIS,
    "true": TokenType.TRUE,
    "var": TokenType.VAR,
    "while": TokenType.WHILE
}


class Scanner:
    source: str
    start: int
    current: int
    line: int
    tokens: list[Token]
    hadError: bool

    def __init__(self, source: str) -> None:
        self.source = source
        self.start = 0
        self.current = 0
        self.line = 1
        self.hadError = False
        self.tokens = []

    def scanTokens(self) -> list[Token]:
        while (not self.isAtEnd()):
            self.start = self.current
            self.scanToken()
        
        self.tokens.append(Token(TokenType.EOF, "", None, self.line))

        return self.tokens
    
    def scanToken(self):
        c = self.advance()

        match c:
            case "/":
                # single line comments
                if self.match("/"):
                    # consume all of a comment
                    # doesn't consume the new line character so the next loop updates line
                    while (self.peek() != "\n" and not self.isAtEnd()):
                        self.advance()
                
                # block comments supporting nesting
                elif self.match("*"):
                    open_count = 1
                    while not self.isAtEnd():
                        check = self.peek() + self.peekNext()
                        if check == "/*" and require_closing_nested_comments:
                            open_count += 1
                        elif check == "*/":
                            open_count -= 1
                        elif check[0] == "\n":
                            self.line += 1

                        self.advance()
                        
                        if open_count == 0:
                            self.advance()  # consume final /
                            break
                        
                else:
                    self.addToken(TokenType.SLASH)
            
            case " " | "\r" | "\t":
                pass
                
            case "\n":
                self.line += 1
            
            case "\"":
                self.scanString()
            
            case _:
                if c in tokenChars:
                    self.addToken(tokenChars[c])
                
                elif c in doubleCharTokens:
                    t = doubleCharTokens[c]
                    if self.match(t.expected):
                        self.addToken(t.multi)
                    else:
                        self.addToken(t.single)
                
                elif c.isdigit():
                    self.scanNumber()

                elif c.isalpha() or c == "_":
                    self.scanIdentifier()

                else:
                    self.error(self.line, "Unexpected character.")
           
    
    def scanString(self):
        while self.peek() != "\"" and not self.isAtEnd():
            if self.peek() == "\n":
                self.line += 1
            
            self.advance() 
        
        if self.isAtEnd():
            self.error(self.line, "Unterminated string.")
        else:
            self.advance()  # consume the closing quote
            value = self.source[self.start+1:self.current-1]  # TODO: escape codes? make new lines really be new lines, etc. 
            self.addToken(TokenType.STRING, value)
    
    def scanNumber(self):
        while self.peek().isdigit():
            self.advance()
        
        if self.peek() == "." and self.peekNext().isdigit():
            self.advance()  # consume the dot

            while self.peek().isdigit():
                self.advance()
        
        self.addToken(TokenType.NUMBER, float(self.source[self.start:self.current]))  # TODO: should ints exist?

    def scanIdentifier(self):
        while self.peek().isalnum() or self.peek() == "_":
            self.advance()
        
        text = self.source[self.start:self.current]
        if text in keywords:
            self.addToken(keywords[text])
        else:
            self.addToken(TokenType.IDENTIFIER)

    
    def addToken(self, ttype: TokenType, literal=None):
        text = self.source[self.start:self.current]  # TODO: not sure if bounds are right
        self.tokens.append(Token(ttype, text, literal, self.line))
    
    def isAtEnd(self):
        return self.current >= len(self.source)
    
    def advance(self):
        c = self.peek()
        self.current += 1
        return c

    def match(self, expected: str) -> bool:
        if self.peek() != expected:
            return False
        
        self.current += 1
        return True
    
    def peek(self) -> str:
        if self.isAtEnd():
            return "\0"
        return self.source[self.current]
    
    def peekNext(self) -> str:
        if (self.current + 1 >= len(self.source)):
            return "\0"
        return self.source[self.current + 1]


    def error(self, line: int, message: str, where: str = ""):
        print("[line " + str(line) + "] Error" + where + ": " + message)
        self.hadError = True
