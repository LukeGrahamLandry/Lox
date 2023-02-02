from generated.expr import *
from Token import *
from typing import Any
from Token import Token

class Stmt:
    def accept(self, visitor: Any):
        raise NotImplementedError()

class Expression(Stmt):
    expression: Expr

    def __init__(self, expression: Expr):
      self.expression = expression

    def accept(self, visitor: Any) -> Any:
        return visitor.visitExpressionStmt(self)

class Print(Stmt):
    expression: Expr

    def __init__(self, expression: Expr):
      self.expression = expression

    def accept(self, visitor: Any) -> Any:
        return visitor.visitPrintStmt(self)

class Var(Stmt):
    name: Token
    initializer: Expr

    def __init__(self, name: Token, initializer: Expr):
      self.name = name
      self.initializer = initializer

    def accept(self, visitor: Any) -> Any:
        return visitor.visitVarStmt(self)

class Block(Stmt):
    statements: list[Stmt]

    def __init__(self, statements: list[Stmt]):
      self.statements = statements

    def accept(self, visitor: Any) -> Any:
        return visitor.visitBlockStmt(self)

class If(Stmt):
    condition: Expr
    thenBranch: Stmt
    elseBranch: Stmt

    def __init__(self, condition: Expr, thenBranch: Stmt, elseBranch: Stmt):
      self.condition = condition
      self.thenBranch = thenBranch
      self.elseBranch = elseBranch

    def accept(self, visitor: Any) -> Any:
        return visitor.visitIfStmt(self)

class While(Stmt):
    condition: Expr
    body: Stmt

    def __init__(self, condition: Expr, body: Stmt):
      self.condition = condition
      self.body = body

    def accept(self, visitor: Any) -> Any:
        return visitor.visitWhileStmt(self)

class Throwable(Stmt):
    token: Token

    def __init__(self, token: Token):
      self.token = token

    def accept(self, visitor: Any) -> Any:
        return visitor.visitThrowableStmt(self)

class Function(Stmt):
    name: Token
    params: list[Token]
    body: list[Stmt]

    def __init__(self, name: Token, params: list[Token], body: list[Stmt]):
      self.name = name
      self.params = params
      self.body = body

    def accept(self, visitor: Any) -> Any:
        return visitor.visitFunctionStmt(self)

class Return(Stmt):
    keyword: Token
    value: Expr

    def __init__(self, keyword: Token, value: Expr):
      self.keyword = keyword
      self.value = value

    def accept(self, visitor: Any) -> Any:
        return visitor.visitReturnStmt(self)


class Visitor:
    def visitExpressionStmt(self, stmt: Expression) -> Any:
        raise NotImplementedError()
    def visitPrintStmt(self, stmt: Print) -> Any:
        raise NotImplementedError()
    def visitVarStmt(self, stmt: Var) -> Any:
        raise NotImplementedError()
    def visitBlockStmt(self, stmt: Block) -> Any:
        raise NotImplementedError()
    def visitIfStmt(self, stmt: If) -> Any:
        raise NotImplementedError()
    def visitWhileStmt(self, stmt: While) -> Any:
        raise NotImplementedError()
    def visitThrowableStmt(self, stmt: Throwable) -> Any:
        raise NotImplementedError()
    def visitFunctionStmt(self, stmt: Function) -> Any:
        raise NotImplementedError()
    def visitReturnStmt(self, stmt: Return) -> Any:
        raise NotImplementedError()
