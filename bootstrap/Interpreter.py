from generated.expression import *
from Token import TokenType
from jlox import reportRuntimeError, LoxRuntimeError

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

class Interpreter(Visitor):
    def interpret(self, expression: Expr):
        try:
            value = self.evaluate(expression)
            print(self.stringify(value))
        except LoxRuntimeError as e:
            reportRuntimeError(e)

    def evaluate(self, expression: Expr):
        return expression.accept(self)

    def visitBinaryExpr(self, expr: Binary):
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
 
    def visitGroupingExpr(self, expr: Grouping):
        return self.evaluate(expr.expression)

    def visitLiteralExpr(self, expr: Literal):
        return expr.value

    def visitUnaryExpr(self, expr: Unary):
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
        return str(value)
