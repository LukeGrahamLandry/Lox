from generated.expr import *
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


class Visitor:
    def visitExpressionStmt(self, stmt: Expression) -> Any:
        raise NotImplementedError()
    def visitPrintStmt(self, stmt: Print) -> Any:
        raise NotImplementedError()
    def visitVarStmt(self, stmt: Var) -> Any:
        raise NotImplementedError()
    def visitBlockStmt(self, stmt: Block) -> Any:
        raise NotImplementedError()
