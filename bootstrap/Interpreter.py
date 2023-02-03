from typing import Any
import generated.expr as expr
import generated.stmt as stmt
from Token import TokenType, Token
from jlox import LoxRuntimeError
from Environment import Environment
import LoxCallable as natives
from LoxCallable import LoxCallable

numberBinaryOperators = [
    TokenType.MINUS,
    TokenType.STAR,
    TokenType.SLASH,
    TokenType.GREATER,
    TokenType.GREATER_EQUAL,
    TokenType.LESS,
    TokenType.LESS_EQUAL,
    TokenType.EXPONENT
]

class LoxExpectedException(LoxRuntimeError):
    type: TokenType
    def __init__(self, token: Token):
        super().__init__(token, "uncaught thrown " + str(token.type))
        self.type = token.type

class LoxReturn(LoxExpectedException):
    value: Any
    def __init__(self, token: Token, value):
        super().__init__(token)
        self.value = value

class Interpreter(expr.Visitor, stmt.Visitor):
    currentScope: Environment
    globalScope: Environment
    errors: list[LoxRuntimeError]
    locals: dict[expr.Expr, int]

    def __init__(self):
        self.globalScope = Environment()
        self.currentScope = self.globalScope
        self.errors = []
        self.locals = {}

        for name, value in self.getLoxGlobals().items():
            self.globalScope.rawDefine(name, value)
    
    def resolve(self, expr: expr.Expr, depth: int):
        self.locals[expr] = depth
    
    def lookUpVariable(self, name: Token, expr: expr.Expr) -> Any:
        if expr in self.locals:
            distance = self.locals[expr]
            return self.currentScope.getAt(distance, name.lexeme)
        else:
            return self.globalScope.get(name)

    def getLoxGlobals(self) -> dict[str, Any]:
        return {
            "clock": natives.Clock(),
            "environment": natives.GetEnvironmentString()
        }

    def interpret(self, statement: stmt.Stmt):
        # since statement will be a Block, the script's outer most scope will not actually be the interpreter's global scope
        # but that's fine. it just means multiple scripts on the same interpreter can't affect each other 
        try:
            self.execute(statement)
        except LoxRuntimeError as e:
            self.errors.append(e)

    def evaluate(self, expression: expr.Expr) -> Any:
        return expression.accept(self)
    
    def execute(self, statement: stmt.Stmt) -> None:
        statement.accept(self)

    def visitReturnStmt(self, stmt: stmt.Return):
        value = self.evaluate(stmt.value)
        raise LoxReturn(stmt.keyword, value)
    
    def visitFunctionDefStmt(self, stmt: stmt.FunctionDef):
        func = LoxFunction(stmt.callable, self.currentScope, name=stmt.name.lexeme)
        self.currentScope.define(stmt.name, func)
    
    def visitThrowableStmt(self, stmt: stmt.Throwable):
        raise LoxExpectedException(stmt.token)  # break / continue

    def visitWhileStmt(self, stmt: stmt.While):
        try:
            while self.isTruthy(self.evaluate(stmt.condition)):
                try:
                    self.execute(stmt.body)
                except LoxExpectedException as e:
                    if e.type != TokenType.CONTINUE:
                        raise e
        except LoxExpectedException as e:
            if e.type != TokenType.BREAK:
                raise e

    def visitIfStmt(self, stmt: stmt.If):
        condition = self.evaluate(stmt.condition)
        if self.isTruthy(condition):
            self.execute(stmt.thenBranch)
        else:
            self.execute(stmt.elseBranch)

    def visitBlockStmt(self, stmt: stmt.Block):
        self.executeBlock(stmt.statements, Environment(self.currentScope))
    
    def executeBlock(self, statements: list[stmt.Stmt], env: Environment):
        previous = self.currentScope

        try:
            self.currentScope = env
            for statement in statements:
                self.execute(statement)
        finally:
            self.currentScope = previous

    def visitVarStmt(self, stmt: stmt.Var):
        value = self.evaluate(stmt.initializer)
        self.currentScope.define(stmt.name, value)

    def visitExpressionStmt(self, stmt: stmt.Expression):
        self.evaluate(stmt.expression)

    def visitPrintStmt(self, stmt: stmt.Print):
        value = self.evaluate(stmt.expression)
        print(self.stringify(value))

    def visitCallExpr(self, expr: expr.Call) -> Any:
        callee = self.evaluate(expr.callee)
        arguments = []
        for arg in expr.arguments:
            arguments.append(self.evaluate(arg))

        if not isinstance(callee, LoxCallable):
            raise LoxRuntimeError(expr.paren, "Can only call functions and classes.")
        
        if len(arguments) != callee.arity():
            raise LoxRuntimeError(expr.paren, f"Expected {callee.arity()} arguments but got {len(arguments)}.")
        
        return callee.call(self, arguments)

    # this returns something with the correct truthy value, not necessarily a boolean 
    def visitLogicalExpr(self, expr: expr.Logical) -> Any:
        left = self.evaluate(expr.left)
        match expr.operator.type:
            case TokenType.OR:
                if self.isTruthy(left):
                    return left
            case TokenType.AND:
                if not self.isTruthy(left):
                    return left
        
        return self.evaluate(expr.right)

    def visitAssignExpr(self, expr: expr.Assign) -> Any:
        value = self.evaluate(expr.value)

        if expr in self.locals:
            distance = self.locals[expr]
            self.currentScope.assignAt(distance, expr.name.lexeme, value)
        else:
            self.globalScope.assign(expr.name, value)
        
        return value

    def visitVariableExpr(self, expr: expr.Variable) -> Any:
        return self.lookUpVariable(expr.name, expr)

    def visitBinaryExpr(self, expr: expr.Binary):
        left = self.evaluate(expr.left)
        right = self.evaluate(expr.right)

        if expr.operator.type in numberBinaryOperators:
            if not isinstance(left, float) or not isinstance(right, float):
                raise LoxRuntimeError(expr.operator, "Operands must be numbers.")

        match expr.operator.type:
            case TokenType.PLUS:
                if isinstance(left, float) and isinstance(right, float):
                    return left + right
                elif isinstance(left, str) and isinstance(right, str):
                    return left + right

                raise LoxRuntimeError(expr.operator, "Operands must be two numbers or two strings.")

            case TokenType.MINUS:
                return left - right
            case TokenType.STAR:
                return left * right
            case TokenType.SLASH:
                if right == 0:
                    raise LoxRuntimeError(expr.operator, "Right operand must not be zero.")
                return left / right
            case TokenType.EXPONENT:
                return left ** right
            case TokenType.GREATER:
                return left > right
            case TokenType.GREATER_EQUAL:
                return left >= right
            case TokenType.LESS:
                return left < right
            case TokenType.LESS_EQUAL:
                return left <= right
            case TokenType.EQUAL_EQUAL:
                return self.isEqual(left, right)
            case TokenType.BANG_EQUAL:
                return not self.isEqual(left, right)
 
    def visitGroupingExpr(self, expr: expr.Grouping):
        return self.evaluate(expr.expression)

    def visitLiteralExpr(self, expr: expr.Literal):
        return expr.value

    def visitFunctionLiteralExpr(self, expr: expr.FunctionLiteral):
        return LoxFunction(expr, self.currentScope, name="anonymous")

    def visitUnaryExpr(self, expr: expr.Unary):
        value = self.evaluate(expr.right)

        match expr.operator.type:
            case TokenType.MINUS:
                if not isinstance(value, float):
                    raise LoxRuntimeError(expr.operator, "Operand must be a number.")
                return -value
            case TokenType.BANG:
                return not self.isTruthy(value)
        
        return None

    def isTruthy(self, value) -> bool:
        if isinstance(value, bool):
            return value
         
        if value == None or value == 0:
            return False
        
        if isinstance(value, str):
            return value != ""

        return True
    
    def isEqual(self, left, right) -> bool:
        return left == right

    def stringify(self, value) -> str:
        s = str(value)

        if isinstance(value, bool):
            return s.lower()
        
        if isinstance(value, float) and s.endswith(".0"):
            return s[:-2]
        
        return s

class LoxFunction(LoxCallable):
    callable: expr.FunctionLiteral
    closure: Environment
    name: str

    def __init__(self, callable: expr.FunctionLiteral, closure: Environment, name: str) -> None:
        self.callable = callable
        self.closure = closure
        self.name = name

    def call(self, interpreter: Interpreter, arguments: list) -> Any:
        environment = Environment(self.closure)

        for i, identifier in enumerate(self.callable.params):
            environment.define(identifier, arguments[i])
        
        try:
            interpreter.executeBlock(self.callable.body, environment)
            return None
        except LoxReturn as returnValue:
            return returnValue.value

    def arity(self) -> int:
        return len(self.callable.params)
    
    def __str__(self) -> str:
        return "<fn " + self.name + ">"
