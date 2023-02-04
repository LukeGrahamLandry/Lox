from logging.config import IDENTIFIER
from Token import Token, TokenType
from jlox import LoxSyntaxError
import generated.expr as expr
import generated.stmt as stmt
from typing import Callable, Any

literals = {
    TokenType.FALSE: False,
    TokenType.TRUE: True,
    TokenType.NIL: None
}

loopJumpTokens = [TokenType.BREAK, TokenType.CONTINUE]

# no technical reason at this point. for consistency with bytecode interpreter in book
max_function_args_count = 255

class Parser:
    tokens: list[Token]
    current: int
    errors: list[LoxSyntaxError]

    def __init__(self, tokens: list[Token]) -> None:
        self.tokens = tokens
        self.current = 0
        self.errors = []

        self.exponent = self.createBinaryPrecedenceLevel(expr.Binary, [TokenType.EXPONENT], self.unary)
        self.factor = self.createBinaryPrecedenceLevel(expr.Binary, [TokenType.SLASH, TokenType.STAR], self.exponent)
        self.term = self.createBinaryPrecedenceLevel(expr.Binary, [TokenType.MINUS, TokenType.PLUS], self.factor)
        self.comparison = self.createBinaryPrecedenceLevel(expr.Binary, [TokenType.GREATER, TokenType.GREATER_EQUAL, TokenType.LESS, TokenType.LESS_EQUAL], self.term)
        self.equality = self.createBinaryPrecedenceLevel(expr.Binary, [TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL], self.comparison)
        self.andExpr = self.createBinaryPrecedenceLevel(expr.Logical, [TokenType.AND], self.equality)
        self.orExpr = self.createBinaryPrecedenceLevel(expr.Logical, [TokenType.OR], self.andExpr)

        self.requireIdentifier = lambda: self.consume(TokenType.IDENTIFIER, "Expect parameter name.")

        self.keywordStatements = {
            TokenType.VAR: self.varDeclaration,
            TokenType.LEFT_BRACE: self.blockStatement,
            TokenType.IF: self.ifStatement,
            TokenType.WHILE: self.whileStatement,
            TokenType.FOR: self.forStatement,
            TokenType.PRINT: lambda: self.expressionStatement(stmt.Print),
            TokenType.RETURN: self.returnStatement,
            TokenType.CLASS: self.classDeclaration
        }

        self.tokenKeywordStatements = {type: stmt.Throwable for type in loopJumpTokens}

    def createBinaryPrecedenceLevel(self, factory, matchingTokens: list[TokenType], nextDown: Callable[[], expr.Expr]) -> Callable[[], expr.Expr]:
        def expand():
            expr = nextDown()

            while self.match(*matchingTokens):
                operator = self.previous()
                right = nextDown()
                expr = factory(expr, operator, right)
            
            return expr
        
        return expand

    def parse(self) -> stmt.Block:
        statements: list[stmt.Stmt] = []
        
        while not self.isAtEnd():
            s = self.declaration()
            if s is not None:
                statements.append(s)
        
        return stmt.Block(statements)

    def declaration(self) -> stmt.Stmt | None:
        try: 
            return self.statement()
        except Exception as e:
            self.synchronize()

    def statement(self) -> stmt.Stmt:
        if self.check(TokenType.FUN) and self.checkNext(TokenType.IDENTIFIER):
            self.advance()
            return self.functionDefinition("function")

        for type, supplier in self.keywordStatements.items():
            if self.match(type):
                return supplier()
        
        for type, supplier in self.tokenKeywordStatements.items():
            if self.match(type):
                token = self.previous()
                statement = supplier(token)
                self.consume(TokenType.SEMICOLON, "Expect ';' after keyword statement.")
                return statement

        return self.expressionStatement(stmt.Expression)
    
    def loopBodyStatement(self) -> stmt.Stmt:
        return self.statement()
    
    def expressionStatement(self, factory) -> stmt.Stmt:
        value = self.expression()
        self.consume(TokenType.SEMICOLON, "Expect ';' after value.")
        return factory(value)
    
    def returnStatement(self) -> stmt.Stmt:
        keyword = self.previous()

        value = stmt.Literal(None)
        if not self.check(TokenType.SEMICOLON):
            value = self.expression()
        self.consume(TokenType.SEMICOLON, "Expect ';' after return value.")
        
        return stmt.Return(keyword, value)

    def functionDefinition(self, kind: str) -> stmt.FunctionDef:
        """
        already consumed: fun
        consumes: IDENTIFIER (IDENTIFIER, *) { STMT; * }
        """

        name = self.consume(TokenType.IDENTIFIER, "Expect " + kind + " name.")
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after" + kind + " name.")

        parameters, closingBracket = self.separatedSequence(entrySupplier=self.requireIdentifier, entryName="parameters")

        self.consume(TokenType.LEFT_BRACE, "Expect '{' before " + kind + " body.")
        body = self.blockStatement()

        return stmt.FunctionDef(name, expr.FunctionLiteral(parameters, body.statements))

    def classDeclaration(self) -> stmt.Class:
        name = self.consume(TokenType.IDENTIFIER, "Expect class name.")

        klass = self.classBody()
        return stmt.Class(name, klass)
    
    def classBody(self) -> expr.ClassLiteral:
        if self.match(TokenType.LESS):
            self.consume(TokenType.IDENTIFIER, "Expect superclass name.")
            superclass = expr.Variable(self.previous())
        else:
            superclass = expr.Variable(Token(TokenType.IDENTIFIER, "Object", None, self.previous().line))

        self.consume(TokenType.LEFT_BRACE, "Expect '{' before class body.")

        methods: list[stmt.FunctionDef] = []
        staticFields: list[stmt.Var] = []

        while not self.check(TokenType.RIGHT_BRACE) and not self.isAtEnd():
            if self.match(TokenType.STATIC):
                if self.match(TokenType.VAR):
                    staticFields.append(self.varDeclaration())
                    
                elif self.match(TokenType.CLASS):
                    klass = self.classDeclaration()
                    staticFields.append(stmt.Var(klass.name, klass.klass))
                    
                elif self.match(TokenType.FUN):
                    func = self.functionDefinition("method")
                    staticFields.append(stmt.Var(func.name, func.callable))
                    
                else:
                    raise self.error(self.peek(), "Static class members must begin with 'fun', 'var' or 'class'")
            
            else:
                self.match(TokenType.FUN)  # specifying is encouraged but we assume anyway for consistency with lox
                methods.append(self.functionDefinition("method"))

            # changed my mind about this rule so that i can use the book's test/examples without tedious editing
            # raise self.error(self.peek(), "Statements in class body must begin with 'fun' or 'static'")
        
        self.consume(TokenType.RIGHT_BRACE, "Expect '}' after class body.")

        return expr.ClassLiteral(methods, staticFields, superclass)

    # i might actually want a tree node for this later so that i keep the information for the transpiler 
    def forStatement(self) -> stmt.Stmt:
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after 'for'.")
        
        if self.match(TokenType.VAR):
            initializer = self.varDeclaration()
        elif self.match(TokenType.SEMICOLON):
            initializer = stmt.Expression(expr.Literal(None))
        else:
            initializer = self.expressionStatement(stmt.Expression)

        if self.check(TokenType.SEMICOLON):
            condition = expr.Literal(True)
        else:
            condition = self.expression()
        self.consume(TokenType.SEMICOLON, "Expect ';' after 'for' condition.")
        
        if self.check(TokenType.RIGHT_PAREN):
            increment = stmt.Expression(expr.Literal(None))
        else:
            increment = stmt.Expression(self.expression())

        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after 'for' clauses.")

        body = self.loopBodyStatement()

        body = stmt.Block([
            body,
            increment
        ])

        return stmt.Block([
            initializer,
            stmt.While(condition, body)
        ])

    def whileStatement(self) -> stmt.Stmt:
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after 'while'.")
        condition = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after 'while' condition.")

        body = self.loopBodyStatement()

        return stmt.While(condition, body)
    
    def ifStatement(self) -> stmt.Stmt:
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after 'if'.")
        condition = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after 'if' condition.")

        thenBranch = self.statement()
        elseBranch = stmt.Expression(expr.Literal(None))
        if self.match(TokenType.ELSE):
            elseBranch = self.statement()
        
        return stmt.If(condition, thenBranch, elseBranch)

    def blockStatement(self) -> stmt.Block:
        """
        already consumed: {
        consumes: STMT; * }
        """

        statements: list[stmt.Stmt] = []

        while not self.check(TokenType.RIGHT_BRACE) and not self.isAtEnd():
            s = self.declaration()
            if s is not None:
                statements.append(s)
        
        self.consume(TokenType.RIGHT_BRACE, "Expect '}' after block.")

        return stmt.Block(statements)

    def varDeclaration(self) -> stmt.Var:
        name = self.consume(TokenType.IDENTIFIER, "Expect variable name.")

        initializer = expr.Literal(None)
        if self.match(TokenType.EQUAL):
            initializer = self.expression()
        
        self.consume(TokenType.SEMICOLON, "Expect ';' after variable declaration.")

        return stmt.Var(name, initializer)

    def expression(self):
        return self.assignment()
    
    def assignment(self):
        expression = self.orExpr()

        if self.match(TokenType.EQUAL):
            equals = self.previous()
            value = self.assignment()

            if isinstance(expression, expr.Variable):
                name = expression.name
                return expr.Assign(name, value)
            
            if isinstance(expression, expr.Get):
                return expr.Set(expression.object, expression.name, value)
            
            # Don't actually raise the error because the parser doesn't need to panic here.
            # It still understands the state, we just know that an assignment target must be a variable so this code is invalid. 
            self.error(equals, "Invalid assignment target."); 

        return expression

    def unary(self) -> expr.Expr:
        if self.match(TokenType.BANG, TokenType.MINUS):
            operator = self.previous()
            right = self.unary()
            return expr.Unary(operator, right)
        
        return self.call()
    
    def call(self) -> expr.Expr:
        expression = self.primary()

        while True:
            if self.match(TokenType.LEFT_PAREN):
                arguments, closingBracket = self.separatedSequence(entrySupplier=self.expression, entryName="arguments")
                expression = expr.Call(expression, closingBracket, arguments)
            elif self.match(TokenType.DOT):
                name = self.consume(TokenType.IDENTIFIER, "Expect property name after '.'.")
                expression = expr.Get(expression, name)
            else:
                break

        return expression

    def primary(self) -> expr.Expr:
        for key, value in literals.items():
            if self.match(key):
                return expr.Literal(value)
        
        if self.match(TokenType.NUMBER, TokenType.STRING):
            return expr.Literal(self.previous().literal)
        
        if self.match(TokenType.LEFT_PAREN):
            expression = self.expression()
            self.consume(TokenType.RIGHT_PAREN, "Expect ')' after expression.")
            return expr.Grouping(expression)
        
        if self.match(TokenType.IDENTIFIER):
            return expr.Variable(self.previous())
        
        if self.match(TokenType.THIS):
            return expr.This(self.previous())
        
        if self.match(TokenType.SUPER):
            keyword = self.previous()
            self.consume(TokenType.DOT, "Expect '.' after 'super'.")
            method = self.consume(TokenType.IDENTIFIER, "Expect superclass method name.")
            return expr.Super(keyword, method)

        # anonymous function
        if self.check(TokenType.FUN) and self.checkNext(TokenType.LEFT_PAREN):
            # consume: fun (
            self.advance()
            self.advance()

            # consume: IDENTIFIER, *) {
            parameters, closingBracket = self.separatedSequence(entrySupplier=self.requireIdentifier, entryName="parameters")
            self.consume(TokenType.LEFT_BRACE, "Expect '{' before anonymous function body.")

            # consume: STMT; * }
            body = self.blockStatement()

            return expr.FunctionLiteral(parameters, body.statements)

        # anonymous class
        if self.check(TokenType.CLASS) and not self.checkNext(TokenType.IDENTIFIER):
            # consume: class 
            self.advance()

            return self.classBody()

        raise self.error(self.peek(), "Expect expression.")
    
    def separatedSequence(self, *, entrySupplier: Callable[[], Any], separator=TokenType.COMMA, terminator=TokenType.RIGHT_PAREN, limit: int | None = max_function_args_count, terminatorName = ")", entryName = "entries") -> tuple[list[Any], Token]:
        """
        consumes: <entrySupplier><separator>* <terminator>
        defaults to function syntax: a, b, c)
        """

        results = []

        if not self.check(terminator):
            while True:
                # If i fail the book's tests here it's because i only log the error once instead of using >=.
                if limit is not None and len(results) == limit:  
                    # Don't actually raise the error because the parser doesn't need to panic here.
                    # It still understands the state so there's nothing to recover from but we arbitrarily don't see it as valid code.
                    self.error(self.peek(), f"Can't have more than {limit} {entryName}.")
                
                results.append(entrySupplier())

                if not self.match(separator):
                    break
        
        closing = self.consume(terminator, f"Expect '{terminatorName}' after {entryName}.")
        return results, closing
    
    def consume(self, type: TokenType, message: str):
        if self.check(type):
            return self.advance()
        
        raise self.error(self.peek(), message)
    
    def error(self, token: Token, message: str):
        self.errors.append(LoxSyntaxError.of(token, message))
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
    
    def checkNext(self, type: TokenType):
        if self.isAtEnd(): 
            return False
        return self.tokens[self.current + 1].type == type

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
