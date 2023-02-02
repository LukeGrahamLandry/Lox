import imp
from typing import Any


import generated.expr as expr
import generated.stmt as stmt
from Token import TokenType
from jlox import reportRuntimeError, LoxRuntimeError
from Environment import Environment

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

class Interpreter(expr.Visitor, stmt.Visitor):
    environment: Environment

    def __init__(self):
        self.environment = Environment()
        

    def interpret(self, statements: list[stmt.Stmt]):
        try:
            for statement in statements:
                self.execute(statement)
        except LoxRuntimeError as e:
            reportRuntimeError(e)

    def evaluate(self, expression: expr.Expr):
        return expression.accept(self)
    
    def execute(self, statement: stmt.Stmt):
        return statement.accept(self)


    def visitBlockStmt(self, stmt: stmt.Block):
        self.executeBlock(stmt.statements, Environment(self.environment))
    
    def executeBlock(self, statements: list[stmt.Stmt], env: Environment):
        previous = self.environment

        try:
            self.environment = env
            for statement in statements:
                self.execute(statement)
        finally:
            self.environment = previous
            


    def visitVarStmt(self, stmt: stmt.Var):
        value = self.evaluate(stmt.initializer)
        self.environment.define(stmt.name.lexeme, value)

    def visitExpressionStmt(self, stmt: stmt.Expression):
        self.evaluate(stmt.expression)

    def visitPrintStmt(self, stmt: stmt.Print):
        value = self.evaluate(stmt.expression)
        print(self.stringify(value))

    def visitAssignExpr(self, expr: expr.Assign) -> Any:
        value = self.evaluate(expr.value)
        self.environment.assign(expr.name, value)
        return value

    def visitVariableExpr(self, expr: expr.Variable) -> Any:
        return self.environment.get(expr.name)

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
