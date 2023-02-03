from typing import Any
from Token import Token
from generated.s import Stmt

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

class Get(Expr):
    object: Expr
    name: Token

    def __init__(self, object: Expr, name: Token):
      self.object = object
      self.name = name

    def accept(self, visitor: Any) -> Any:
        return visitor.visitGetExpr(self)

class FunctionLiteral(Expr):
    params: list[Token]
    body: list[Stmt]

    def __init__(self, params: list[Token], body: list[Stmt]):
      self.params = params
      self.body = body

    def accept(self, visitor: Any) -> Any:
        return visitor.visitFunctionLiteralExpr(self)

class Set(Expr):
    object: Expr
    name: Token
    value: Expr

    def __init__(self, object: Expr, name: Token, value: Expr):
      self.object = object
      self.name = name
      self.value = value

    def accept(self, visitor: Any) -> Any:
        return visitor.visitSetExpr(self)

class This(Expr):
    keyword: Token

    def __init__(self, keyword: Token):
      self.keyword = keyword

    def accept(self, visitor: Any) -> Any:
        return visitor.visitThisExpr(self)

class ClassLiteral(Expr):
    methods: list[Any]
    staticFields: list[Any]

    def __init__(self, methods: list[Any], staticFields: list[Any]):
      self.methods = methods
      self.staticFields = staticFields

    def accept(self, visitor: Any) -> Any:
        return visitor.visitClassLiteralExpr(self)


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
    def visitGetExpr(self, expr: Get) -> Any:
        raise NotImplementedError()
    def visitFunctionLiteralExpr(self, expr: FunctionLiteral) -> Any:
        raise NotImplementedError()
    def visitSetExpr(self, expr: Set) -> Any:
        raise NotImplementedError()
    def visitThisExpr(self, expr: This) -> Any:
        raise NotImplementedError()
    def visitClassLiteralExpr(self, expr: ClassLiteral) -> Any:
        raise NotImplementedError()

__all__ = ['Expr', 'Binary', 'Grouping', 'Literal', 'Unary', 'Variable', 'Assign', 'Logical', 'Call', 'Get', 'FunctionLiteral', 'Set', 'This', 'ClassLiteral']
