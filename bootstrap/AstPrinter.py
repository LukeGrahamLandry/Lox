from generated.expr import *
import generated.stmt as stmt
from Token import TokenType
from generated.expr import Visitor

class AstPrinter(Visitor, stmt.Visitor):
    indent: int
    def __init__(self):
        self.indent = 0

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

    def visitCallExpr(self, expr: Call) -> str:
        return "(func_call " + self.print(expr.callee) + " " + str([self.print(x) for x in expr.arguments]) + ")"

    def visitBinaryExpr(self, expr: Binary) -> str:
        return self.parenthesize(expr.operator.lexeme, expr.left, expr.right)

    def visitGroupingExpr(self, expr: Grouping) -> str:
        return self.parenthesize("group", expr.expression)

    def visitLiteralExpr(self, expr: Literal) -> str:
        if expr.value == None:
             return "nil"
        return str(expr.value)
    
    def visitFunctionLiteralExpr(self, expr: FunctionLiteral) -> str:
        return "anon_func"

    def visitUnaryExpr(self, expr: Unary) -> str:
        return self.parenthesize(expr.operator.lexeme, expr.right)

    def visitAssignExpr(self, expr: Assign) -> str:
        return self.parenthesize("=", Literal(expr.name.lexeme), expr.value)
    
    def visitVariableExpr(self, expr: Variable) -> str:
        return expr.name.lexeme
    
    def visitLogicalExpr(self, expr: Logical) -> str:
        return self.parenthesize(expr.operator.lexeme, expr.left, expr.right)

    def visitExpressionStmt(self, stmt: stmt.Expression) -> str:
        return stmt.expression.accept(self) + ";"

    def visitPrintStmt(self, stmt: stmt.Print) -> str:
        return self.parenthesize("print", stmt.expression)

    def visitVarStmt(self, stmt: stmt.Var) -> str:
        return self.parenthesize("var", Literal(stmt.name.lexeme), stmt.initializer)

    def visitBlockStmt(self, stmt: stmt.Block) -> str:
        self.indent += 1

        s = "\n"
        for statement in stmt.statements:
            s += ("  " * self.indent) + "| " + statement.accept(self) + "\n"
        
        self.indent -= 1
        
        return s[:-1]
    
    def visitIfStmt(self, stmt: stmt.If) -> str:
        return "if " + self.print(stmt.condition) + " then {" + self.printStatement(stmt.thenBranch) + "} else {" + self.printStatement(stmt.elseBranch) + "}"

    def visitWhileStmt(self, stmt: stmt.While) -> str:
        return "while (" + self.print(stmt.condition) + ") do " + self.printStatement(stmt.body) + ""

    def visitThrowableStmt(self, stmt: stmt.Throwable) -> str:
        return "throw (" + str(stmt.token.type) + ")"
    
    def visitFunctionDefStmt(self, statement: stmt.FunctionDef) -> str:
        return "(func_def " + statement.name.lexeme + " " + str([x.lexeme for x in statement.callable.params]) + ") " + self.printStatement(stmt.Block(statement.callable.body))

    def visitReturnStmt(self, stmt: stmt.Return) -> str:
        return self.parenthesize("return", stmt.value)

