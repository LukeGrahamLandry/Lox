from re import L
from turtle import right
from Token import Token, TokenType
from jlox import reportError, LoxRuntimeError
from generated.expr import *
from generated.stmt import *
from typing import Callable

literals = {
    TokenType.FALSE: False,
    TokenType.TRUE: True,
    TokenType.NIL: None
}

class Parser:
    tokens: list[Token]
    current: int

    def __init__(self, tokens: list[Token]) -> None:
        self.tokens = tokens
        self.current = 0

        self.exponent = self.createBinaryPrecedenceLevel([TokenType.EXPONENT], self.unary)
        self.factor = self.createBinaryPrecedenceLevel([TokenType.SLASH, TokenType.STAR], self.exponent)
        self.term = self.createBinaryPrecedenceLevel([TokenType.MINUS, TokenType.PLUS], self.factor)
        self.comparison = self.createBinaryPrecedenceLevel([TokenType.GREATER, TokenType.GREATER_EQUAL, TokenType.LESS, TokenType.LESS_EQUAL], self.term)
        self.equality = self.createBinaryPrecedenceLevel([TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL], self.comparison)

    def createBinaryPrecedenceLevel(self, matchingTokens: list[TokenType], nextDown: Callable[[], Expr]) -> Callable[[], Expr]:
        def expand():
            expr = nextDown()

            while self.match(*matchingTokens):
                operator = self.previous()
                right = nextDown()
                expr = Binary(expr, operator, right)
            
            return expr
        
        return expand

    def parse(self) -> list[Stmt]:
        statements: list[Stmt] = []
        
        while not self.isAtEnd():
            s = self.declaration()
            if s is not None:
                statements.append(s)
        
        return statements

    def declaration(self) -> Stmt | None:
        try: 
            if self.match(TokenType.VAR):
                return self.varDeclaration()
            return self.statement()
            
        except:
            print("Panic!")
            self.synchronize()
    
    def statement(self) -> Stmt:
        if self.match(TokenType.LEFT_BRACE):
            return self.blockStatement()
        
        isPrint = self.match(TokenType.PRINT)

        value = self.expression()
        self.consume(TokenType.SEMICOLON, "Expect ';' after value.")

        if isPrint:
            return Print(value)
        
        return Expression(value)
    
    def blockStatement(self) -> Stmt:
        statements: list[Stmt] = []

        while not self.check(TokenType.RIGHT_BRACE) and not self.isAtEnd():
            s = self.declaration()
            if s is not None:
                statements.append(s)
        
        self.consume(TokenType.RIGHT_BRACE, "Expect '}' after block.")

        return Block(statements)

    def varDeclaration(self) -> Stmt:
        name = self.consume(TokenType.IDENTIFIER, "Expect variable name.")

        initializer = Literal(None)
        if self.match(TokenType.EQUAL):
            initializer = self.expression()
        
        self.consume(TokenType.SEMICOLON, "Expect ';' after variable declaration.")

        return Var(name, initializer)

    def expression(self):
        return self.assignment()
    
    def assignment(self):
        expr = self.equality()

        if self.match(TokenType.EQUAL):
            equals = self.previous()
            value = self.assignment()

            if isinstance(expr, Variable):
                name = expr.name
                return Assign(name ,value)
            
            self.error(equals, "Invalid assignment target."); 

        return expr

    def unary(self) -> Expr:
        if self.match(TokenType.BANG, TokenType.MINUS):
            operator = self.previous()
            right = self.unary()
            return Unary(operator, right)
        
        return self.primary()
    
    def primary(self) -> Expr:
        for key, value in literals.items():
            if self.match(key):
                return Literal(value)
        
        if self.match(TokenType.NUMBER, TokenType.STRING):
            return Literal(self.previous().literal)
        
        if self.match(TokenType.LEFT_PAREN):
            expr = self.expression()
            self.consume(TokenType.RIGHT_PAREN, "Expect ')' after expression.")
            return Grouping(expr)
        
        if self.match(TokenType.IDENTIFIER):
            return Variable(self.previous())
        
        raise self.error(self.peek(), "Expect expression.")
    
    def consume(self, type: TokenType, message: str):
        if self.check(type):
            return self.advance()
        
        raise self.error(self.peek(), message)
    
    def error(self, token: Token, message: str):
        if (token.type == TokenType.EOF):
            reportError(token.line, message, where=" at end")
        else:
            reportError(token.line, message, where=" at '" + token.lexeme + "'")
        
        return RuntimeError()

    def synchronize(self): 
        self.advance()

        while not self.isAtEnd():
            if self.previous().type == TokenType.SEMICOLON:
                return

            if self.peek().type in [TokenType.CLASS, TokenType.FUN, TokenType.VAR, TokenType.FOR, TokenType.IF, TokenType.WHILE, TokenType.PRINT, TokenType.RETURN]:
                return
            
            self.advance()
    
    def match(self, *types):
        for ttype in types:
            if self.check(ttype):
                self.advance()
                return True
        return False

    def check(self, type: TokenType):
        if self.isAtEnd(): 
            return False
        return self.peek().type == type

    def advance(self) -> Token:
        if not self.isAtEnd():
            self.current += 1 
        return self.previous()
    
    def isAtEnd(self):
        return self.peek().type == TokenType.EOF

    def peek(self):
        return self.tokens[self.current]
    
    def previous(self):
        return self.tokens[self.current - 1]
    

  
    
