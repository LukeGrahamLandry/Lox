from typing import Any
from Token import Token

class Expr:
    def accept(self, visitor: Any):
        raise NotImplementedError()

class Binary(Expr):
    left: Expr
    operator: Token
    right: Expr

    def __init__(self, left: Expr, operator: Token, right: Expr):
      self.left = left
      self.operator = operator
      self.right = right

    def accept(self, visitor: Any) -> Any:
        return visitor.visitBinaryExpr(self)

class Grouping(Expr):
    expression: Expr

    def __init__(self, expression: Expr):
      self.expression = expression

    def accept(self, visitor: Any) -> Any:
        return visitor.visitGroupingExpr(self)

class Literal(Expr):
    value: Any

    def __init__(self, value: Any):
      self.value = value

    def accept(self, visitor: Any) -> Any:
        return visitor.visitLiteralExpr(self)

class Unary(Expr):
    operator: Token
    right: Expr

    def __init__(self, operator: Token, right: Expr):
      self.operator = operator
      self.right = right

    def accept(self, visitor: Any) -> Any:
        return visitor.visitUnaryExpr(self)

class Variable(Expr):
    name: Token

    def __init__(self, name: Token):
      self.name = name

    def accept(self, visitor: Any) -> Any:
        return visitor.visitVariableExpr(self)

class Assign(Expr):
    name: Token
    value: Expr

    def __init__(self, name: Token, value: Expr):
      self.name = name
      self.value = value

    def accept(self, visitor: Any) -> Any:
        return visitor.visitAssignExpr(self)

class Logical(Expr):
    left: Expr
    operator: Token
    right: Expr

    def __init__(self, left: Expr, operator: Token, right: Expr):
      self.left = left
      self.operator = operator
      self.right = right

    def accept(self, visitor: Any) -> Any:
        return visitor.visitLogicalExpr(self)

class Call(Expr):
    callee: Expr
    paren: Token
    arguments: list[Expr]

    def __init__(self, callee: Expr, paren: Token, arguments: list[Expr]):
      self.callee = callee
      self.paren = paren
      self.arguments = arguments

    def accept(self, visitor: Any) -> Any:
        return visitor.visitCallExpr(self)


class Visitor:
    def visitBinaryExpr(self, expr: Binary) -> Any:
        raise NotImplementedError()
    def visitGroupingExpr(self, expr: Grouping) -> Any:
        raise NotImplementedError()
    def visitLiteralExpr(self, expr: Literal) -> Any:
        raise NotImplementedError()
    def visitUnaryExpr(self, expr: Unary) -> Any:
        raise NotImplementedError()
    def visitVariableExpr(self, expr: Variable) -> Any:
        raise NotImplementedError()
    def visitAssignExpr(self, expr: Assign) -> Any:
        raise NotImplementedError()
    def visitLogicalExpr(self, expr: Logical) -> Any:
        raise NotImplementedError()
    def visitCallExpr(self, expr: Call) -> Any:
        raise NotImplementedError()
