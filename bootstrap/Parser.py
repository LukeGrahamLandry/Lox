from Token import Token, TokenType
from jlox import LoxSyntaxError
from generated.expr import *
from generated.stmt import *
from typing import Callable

literals = {
    TokenType.FALSE: False,
    TokenType.TRUE: True,
    TokenType.NIL: None
}

loopJumpTokens = [TokenType.BREAK, TokenType.CONTINUE]

class Parser:
    tokens: list[Token]
    current: int
    currentLoopDepth: int
    errors: list[LoxSyntaxError]

    def __init__(self, tokens: list[Token]) -> None:
        self.tokens = tokens
        self.current = 0
        self.currentLoopDepth = 0
        self.errors = []

        self.exponent = self.createBinaryPrecedenceLevel(Binary, [TokenType.EXPONENT], self.unary)
        self.factor = self.createBinaryPrecedenceLevel(Binary, [TokenType.SLASH, TokenType.STAR], self.exponent)
        self.term = self.createBinaryPrecedenceLevel(Binary, [TokenType.MINUS, TokenType.PLUS], self.factor)
        self.comparison = self.createBinaryPrecedenceLevel(Binary, [TokenType.GREATER, TokenType.GREATER_EQUAL, TokenType.LESS, TokenType.LESS_EQUAL], self.term)
        self.equality = self.createBinaryPrecedenceLevel(Binary, [TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL], self.comparison)
        self.andExpr = self.createBinaryPrecedenceLevel(Logical, [TokenType.AND], self.equality)
        self.orExpr = self.createBinaryPrecedenceLevel(Logical, [TokenType.OR], self.andExpr)

        self.keywordStatements = {
            TokenType.LEFT_BRACE: self.blockStatement,
            TokenType.IF: self.ifStatement,
            TokenType.WHILE: self.whileStatement,
            TokenType.FOR: self.forStatement,
            TokenType.PRINT: lambda: self.expressionStatement(Print)
        }

        self.tokenKeywordStatements = {type:Throwable for type in loopJumpTokens}

    def createBinaryPrecedenceLevel(self, factory, matchingTokens: list[TokenType], nextDown: Callable[[], Expr]) -> Callable[[], Expr]:
        def expand():
            expr = nextDown()

            while self.match(*matchingTokens):
                operator = self.previous()
                right = nextDown()
                expr = factory(expr, operator, right)
            
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
            
        except Exception as e:
            self.synchronize()

    def statement(self) -> Stmt:
        for type, supplier in self.keywordStatements.items():
            if self.match(type):
                return supplier()
        
        for type, supplier in self.tokenKeywordStatements.items():
            if self.match(type):
                token = self.previous()
                statement = supplier(token)
                if self.currentLoopDepth <= 0 and token.type in loopJumpTokens:
                    # TODO: if i add new types of throwable, check if its a loop one
                    raise self.error(token, "Unexpected jump outside loop.")
                self.consume(TokenType.SEMICOLON, "Expect ';' after keyword statement.")
                return statement

        return self.expressionStatement(Expression)
    
    def loopBodyStatement(self) -> Stmt:
        self.currentLoopDepth += 1
        statement = self.statement()
        self.currentLoopDepth -= 1
        return statement
    
    def expressionStatement(self, factory) -> Stmt:
        value = self.expression()
        self.consume(TokenType.SEMICOLON, "Expect ';' after value.")
        return factory(value)

    # i might actually want a tree node for this later so that i keep the information for the transpiler 
    def forStatement(self) -> Stmt:
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after 'for'.")
        
        if self.match(TokenType.VAR):
            initializer = self.varDeclaration()
        elif self.match(TokenType.SEMICOLON):
            initializer = Expression(Literal(None))
        else:
            initializer = self.expressionStatement(Expression)

        if self.check(TokenType.SEMICOLON):
            condition = Literal(True)
        else:
            condition = self.expression()
        self.consume(TokenType.SEMICOLON, "Expect ';' after 'for' condition.")
        
        if self.check(TokenType.RIGHT_PAREN):
            increment = Expression(Literal(None))
        else:
            increment = Expression(self.expression())

        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after 'for' clauses.")

        body = self.loopBodyStatement()

        body = Block([
            body,
            increment
        ])

        return Block([
            initializer,
            While(condition, body)
        ])

    def whileStatement(self) -> Stmt:
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after 'while'.")
        condition = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after 'while' condition.")

        body = self.loopBodyStatement()

        return While(condition, body)
    
    def ifStatement(self) -> Stmt:
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after 'if'.")
        condition = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after 'if' condition.")

        thenBranch = self.statement()
        elseBranch = Expression(Literal(None))
        if self.match(TokenType.ELSE):
            elseBranch = self.statement()
        
        return If(condition, thenBranch, elseBranch)

    
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
        expr = self.orExpr()

        if self.match(TokenType.EQUAL):
            equals = self.previous()
            value = self.assignment()

            if isinstance(expr, Variable):
                name = expr.name
                return Assign(name ,value)
            
            # TODO: why no throw? go back in the book
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
            where = " at end"
        else:
            where = " at '" + token.lexeme + "'"

        self.errors.append(LoxSyntaxError(token.line, message, where))
        
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
