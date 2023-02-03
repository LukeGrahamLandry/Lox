from jlox import LoxSyntaxError
import generated.expr as expr
import generated.stmt as stmt
from Interpreter import Interpreter
from Token import Token, TokenType
from enum import Enum, auto

class DictStack:
    data: list[dict[str, bool]]
    used: list[set[str]]
    declared: list[set[Token]]

    def __init__(self) -> None:
        self.data = []
        self.used = []
        self.declared = []

    def push(self):
        self.data.append({})
        self.used.append(set())
        self.declared.append(set())
    
    def pop(self) -> tuple[set[Token], set[str]]:
        del self.data[-1]
        u = self.used[-1]
        del self.used[-1]
        d = self.declared[-1]
        del self.declared[-1]
        return d, u
    
    def peek(self) -> dict[str, bool]:
        return self.data[-1]
    
    def isEmpty(self) -> bool:
        return len(self.data) == 0
    
    def put(self, key: str, value: bool):
        self.peek()[key] = value
    
    def putDeclaration(self, name: Token):
        self.declared[-1].add(name)
    
    def size(self) -> int:
        return len(self.data)
    
    def get(self, i: int):
        return self.data[i]
    
    def markUsed(self, i, name: str):
        self.used[i].add(name)

class FunctionType(Enum):
    NONE = auto()
    FUNCTION = auto()
    METHOD = auto()
    INITIALIZER = auto()

class ClassType(Enum):
    NONE = auto()
    CLASS = auto()

class Resolver(expr.Visitor, stmt.Visitor):
    interpreter: Interpreter
    scopes: DictStack
    errors: list[LoxSyntaxError]
    currentFunction: FunctionType
    currentClass: ClassType
    currentLoopDepth: int
    activeJumpStatement: Token | None

    def __init__(self, interpreter: Interpreter):
        self.interpreter = interpreter
        self.scopes = DictStack()
        self.errors = []
        self.currentFunction = FunctionType.NONE
        self.currentClass = ClassType.NONE
        self.currentLoopDepth = 0
        self.activeJumpStatement = None
    
    def resolveStmt(self, stmt: stmt.Stmt):
        if self.activeJumpStatement is not None:
            self.error(self.activeJumpStatement, "Jump causes unreachable code.")
            self.activeJumpStatement = None  # don't just spam errors every line after the return. 
        stmt.accept(self)

    def resolveExpr(self, expr: expr.Expr):
        expr.accept(self)
    
    def resolve(self, statements: list[stmt.Stmt]):
        for statement in statements:
            self.resolveStmt(statement)
    
    def beginScope(self):
        self.scopes.push()
    
    def endScope(self):
        declared, used = self.scopes.pop()
        for var in declared:
            if not var.lexeme in used:
                self.error(var, "Unused local variable.")

    def declare(self, name: Token):
        if self.scopes.isEmpty():
            return
        
        if name.lexeme in self.scopes.peek():
            self.error(name, "Already a variable with this name in this scope.")
        
        self.scopes.put(name.lexeme, False)
        self.scopes.putDeclaration(name)

    def define(self, token: Token):
        if self.scopes.isEmpty():
            return
        
        self.scopes.put(token.lexeme, True)
    
    def error(self, name: Token, message: str):
        self.errors.append(LoxSyntaxError.of(name, message))
    
    def resolveLocal(self, expr: expr.Expr, token: Token):
        for i in range(self.scopes.size()):
            i = self.scopes.size() - 1 - i
            scope = self.scopes.get(i)
            if token.lexeme in scope:
                self.scopes.markUsed(i, token.lexeme)
                distance = self.scopes.size() - 1 - i
                self.interpreter.resolve(expr, distance)
                return
        
        # the script is its own block and the globalScope only has the native functions so we know all of them ahead of time. 
        if not token.lexeme in self.interpreter.globalScope.values:
            self.error(token, "Cannot access undeclared variable.")
    
    def resolveFunction(self, expr: expr.FunctionLiteral, type: FunctionType):
        enclosingFunction = self.currentFunction
        self.currentFunction = type

        self.beginScope()
        for token in expr.params:
            self.declare(token)
            self.define(token)
        self.resolve(expr.body)
        self.endScope()
        self.activeJumpStatement = None

        self.currentFunction = enclosingFunction

    def visitBlockStmt(self, stmt: stmt.Block):
        self.beginScope()
        self.resolve(stmt.statements)
        self.endScope()

    def visitVarStmt(self, stmt: stmt.Var):
        self.declare(stmt.name)
        self.resolveExpr(stmt.initializer)
        self.define(stmt.name)

    def visitVariableExpr(self, expr: expr.Variable):
        if not self.scopes.isEmpty() and expr.name.lexeme in self.scopes.peek() and self.scopes.peek()[expr.name.lexeme] is False:
            self.error(expr.name, "Can't read local variable in its own initializer.")
        
        self.resolveLocal(expr, expr.name)
    
    def visitAssignExpr(self, expr: expr.Assign):
        self.resolveExpr(expr.value)
        self.resolveLocal(expr, expr.name)
    
    def visitFunctionDefStmt(self, stmt: stmt.FunctionDef):
        self.declare(stmt.name)
        self.define(stmt.name)
        self.resolveFunction(stmt.callable, FunctionType.FUNCTION)
    
    def visitExpressionStmt(self, stmt: stmt.Expression):
        self.resolveExpr(stmt.expression)
    
    def visitIfStmt(self, stmt: stmt.If):
        self.resolveExpr(stmt.condition)
        self.resolveStmt(stmt.thenBranch)
        returnIfBranch = self.activeJumpStatement
        self.activeJumpStatement = None
        self.resolveStmt(stmt.elseBranch)
        if returnIfBranch is None or self.activeJumpStatement is None:
            self.activeJumpStatement = None

    def visitPrintStmt(self, stmt: stmt.Print):
        self.resolveExpr(stmt.expression)

    def visitReturnStmt(self, stmt: stmt.Return):
        if self.currentFunction == FunctionType.NONE:
            self.error(stmt.keyword, "Can't return from top-level code.")
        elif self.currentFunction == FunctionType.INITIALIZER and not (isinstance(stmt.value, expr.Literal) and stmt.value.value is None):
            self.error(stmt.keyword, "Can't return a value from an initializer.")
        else:
            self.activeJumpStatement = stmt.keyword
        
        self.resolveExpr(stmt.value)
    
    def visitWhileStmt(self, stmt: stmt.While):
        self.resolveExpr(stmt.condition)
        self.currentLoopDepth += 1
        self.resolveStmt(stmt.body)
        self.activeJumpStatement = None
        self.currentLoopDepth -= 1
    
    def visitBinaryExpr(self, expr: expr.Binary):
        self.resolveExpr(expr.left)
        self.resolveExpr(expr.right)
    
    def visitLogicalExpr(self, expr: expr.Logical):
        self.resolveExpr(expr.left)
        self.resolveExpr(expr.right)
    
    def visitCallExpr(self, expr: expr.Call):
        self.resolveExpr(expr.callee)
        for arg in expr.arguments:
            self.resolveExpr(arg)

    def visitGroupExpr(self, expr: expr.Grouping):
        self.resolveExpr(expr.expression)
    
    def visitUnaryExpr(self, expr: expr.Unary):
        self.resolveExpr(expr.right)

    def visitLiteralExpr(self, expr: expr.Literal):
        pass
    
    def visitFunctionLiteralExpr(self, expr: expr.FunctionLiteral):
        self.resolveFunction(expr, FunctionType.FUNCTION)
    
    def visitThrowableStmt(self, stmt: stmt.Throwable):
        if self.currentLoopDepth <= 0:
            self.error(stmt.token, "Can't jump from outside loop.")
        else:
            self.activeJumpStatement = stmt.token

    def visitClassStmt(self, stmt: stmt.Class):
        enclosing = self.currentClass
        self.currentClass = ClassType.CLASS

        self.declare(stmt.name)
        self.define(stmt.name)

        self.beginScope()
        self.scopes.put("this", True)
        self.scopes.markUsed(self.scopes.size() - 1, "this")  # don't worry about unused 'this' variable

        for method in stmt.methods:
            declaration = FunctionType.METHOD
            if method.name.lexeme == "init":
                declaration = FunctionType.INITIALIZER
            
            self.resolveFunction(method.callable, declaration)
        
        self.endScope()
        self.currentClass = enclosing

    def visitGetExpr(self, expr: expr.Get):
        # TODO: static field checking. currently its done dynamically so we just ignore and trust that it will exist at runtime
        self.resolveExpr(expr.object)
    
    def visitSetExpr(self, expr: expr.Set):
        # TODO: static field checking 
        self.resolveExpr(expr.object)
        self.resolveExpr(expr.value)
    
    def visitThisExpr(self, expr: expr.This):
        if self.currentClass == ClassType.NONE:
            self.error(expr.keyword, "Can't use 'this' outside of a class.")
        else:
            self.resolveLocal(expr, expr.keyword)
