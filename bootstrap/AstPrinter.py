from generated.expr import *
import generated.stmt as stmt
from Token import TokenType

class AstPrinter(Visitor, stmt.Visitor):
    def printStatement(self, statement: stmt.Stmt) -> str:
        return statement.accept(self)
            
    def print(self, expr: Expr) -> str:
        return expr.accept(self)

    def parenthesize(self, name: str, *exprs) -> str:
        out = "(" + name

        for expr in exprs:
            out += " " + expr.accept(self)
        
        out += ")"
        return out

    def visitBinaryExpr(self, expr: Binary) -> str:
        return self.parenthesize(expr.operator.lexeme, expr.left, expr.right)

    def visitGroupingExpr(self, expr: Grouping) -> str:
        return self.parenthesize("group", expr.expression)

    def visitLiteralExpr(self, expr: Literal) -> str:
        if expr.value == None:
             return "nil"
        return str(expr.value)

    def visitUnaryExpr(self, expr: Unary) -> str:
        return self.parenthesize(expr.operator.lexeme, expr.right)

    def visitAssignExpr(self, expr: Assign) -> str:
        return expr.name.lexeme + " = " + self.print(expr.value)
    
    def visitVariableExpr(self, expr: Variable) -> str:
        return expr.name.lexeme
    
    def visitLogicalExpr(self, expr: Logical) -> str:
        return self.parenthesize(expr.operator.lexeme, expr.left, expr.right)

    def visitExpressionStmt(self, stmt: stmt.Expression) -> str:
        return stmt.expression.accept(self)

    def visitPrintStmt(self, stmt: stmt.Print) -> str:
        return "print " + self.print(stmt.expression)

    def visitVarStmt(self, stmt: stmt.Var) -> str:
        return stmt.name.lexeme + "=" + self.print(stmt.initializer)

    def visitBlockStmt(self, stmt: stmt.Block) -> str:
        s = ""
        for statement in stmt.statements:
            s += statement.accept(self) + ", "
        
        return s[:-2]
    
    def visitIfStmt(self, stmt: stmt.If) -> Any:
        return "if (" + self.print(stmt.condition) + ") then {" + self.printStatement(stmt.thenBranch) + "} else {" + self.printStatement(stmt.elseBranch) + "}"

    def visitWhileStmt(self, stmt: stmt.While) -> Any:
        return "while (" + self.print(stmt.condition) + ") do {" + self.printStatement(stmt.body) + "}"

    def visitThrowableStmt(self, stmt: stmt.Throwable) -> str:
        return "throw " + str(stmt.token.type)
