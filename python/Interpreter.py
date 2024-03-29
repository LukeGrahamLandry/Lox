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
    metaClass: "LoxClass"
    objectClass: "LoxClass"

    def __init__(self):
        self.globalScope = Environment()
        self.currentScope = self.globalScope
        self.errors = []
        self.locals = {}
        self.metaClass = MetaClass()
        self.objectClass = LoxClass("Object", {}, superClass=None, metaClass=self.metaClass)  # i can put like toString or whatever here to let people override it once i do non-dynamic fields 

        self.globalScope.rawDefine("Object", self.objectClass)
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

    def visitClassStmt(self, stmt: stmt.Class):
        self.declareClass(stmt.klass, classNameToken=stmt.name)
    
    def declareClass(self, klassLiteral: expr.ClassLiteral, classNameToken: Token | None = None) -> "LoxClass":
        superclass = self.evaluate(klassLiteral.superclass)
        if not isinstance(superclass, LoxClass):
            raise LoxRuntimeError(klassLiteral.superclass.name, "Superclass must be a class.")
           
        className = "anon"
        if classNameToken is not None:
            className = classNameToken.lexeme
            self.currentScope.define(classNameToken, None)  # allow backward reference of yourself. TODO: can i remove this when i split up the resolver?
            # TODO: currently since static classes are parsed into var assignment to literal, it wont know its name so you cant refer to it inside
            # recursion for static functions of non static classs works tho 
            # because its not referring to the function as a variable, it has to refer to the class which i carefully allowed self reference for and fields are done at runtime so its fine
        
        # methods are not just shoved in a field.
        # because we want to treat them differently later to inject 'this' in a new environment when referenced.
        methods: dict[str, LoxFunction] = {}
        methodsScope = Environment(self.currentScope)
        methodsScope.rawDefine("super", superclass)
        for method in klassLiteral.methods:
            className = className + "::" + method.name.lexeme  # just used for string representation when printed
            isInitializer = method.name.lexeme == "init"
            func = LoxFunction(method.callable, methodsScope, className, isInitializer=isInitializer)
            methods[method.name.lexeme] = func
        
        klass = LoxClass(className, methods, superClass=superclass, metaClass=self.metaClass)
        if classNameToken is not None:
            self.currentScope.define(classNameToken, klass)

        # this mirrors the resolver's structure.
        # static initializers are evaluated in a new scope but don't actually add themselves to it.
        # so you still reference earlier statics with the class name prefix but referencing things from the scope above works out to the right number of hops.
        previous = self.currentScope
        try:
            self.currentScope = Environment(self.currentScope)
            for field in klassLiteral.staticFields:
                value = self.evaluate(field.initializer)
                if isinstance(value, LoxFunction):
                    value = LoxFunction(value.callable, self.currentScope, "static " + className + "::" + field.name.lexeme)
                klass.set(field.name, value)
        finally:
            self.currentScope = previous

        return klass
        

    def visitGetExpr(self, expr: expr.Get) -> Any:
        instance = self.evaluate(expr.object)
        # TODO: this is where i could inject methods on primitive types
        if isinstance(instance, LoxInstance):
            return instance.get(expr.name)
        
        raise LoxRuntimeError(expr.name, "Only instances have properties.")  # terminology: properties refers to fields and methods? 
    
    def visitSetExpr(self, expr: expr.Set) -> Any:
        instance = self.evaluate(expr.object)

        if isinstance(instance, LoxInstance):
            # the right side is only evaluated if the left was actually an instance of a class.
            # i guess it would be visible behaviour if the right was a function with a side effect 
            # and you were using try/catch over the error to treat primitives differently. 
            value = self.evaluate(expr.value)
            instance.set(expr.name, value)
            return value
        
        raise LoxRuntimeError(expr.name, "Only instances have fields.")
    
    def visitThisExpr(self, expr: expr.This) -> Any:
        return self.lookUpVariable(expr.keyword, expr)
    
    def visitSuperExpr(self, expr: expr.Super) -> Any:
        distance = self.locals.get(expr)
        if distance is None:
            raise LoxRuntimeError(expr.keyword, "Used super outside method?")
        
        superClass: LoxClass = self.currentScope.getAt(distance, "super")
        instance: LoxInstance = self.currentScope.getAt(distance - 1, "this")

        method = superClass.findMethod(expr.method.lexeme)
        if method is None:
            raise LoxRuntimeError(expr.method, "Undefined property '" + expr.method.lexeme + "'.")

        return method.bind(instance)

    
    def visitReturnStmt(self, stmt: stmt.Return):
        value = self.evaluate(stmt.value)
        raise LoxReturn(stmt.keyword, value)
    
    def visitClassLiteralExpr(self, expr: expr.ClassLiteral) -> Any:
        return self.declareClass(expr)

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

        if value is None:
            return "nil"

        if isinstance(value, bool):
            return s.lower()
        
        if isinstance(value, float) and s.endswith(".0"):
            return s[:-2]
        
        return s

class LoxFunction(LoxCallable):
    callable: expr.FunctionLiteral
    closure: Environment
    name: str
    isInitializer: bool

    def __init__(self, callable: expr.FunctionLiteral, closure: Environment, name: str, isInitializer = False) -> None:
        self.callable = callable
        self.closure = closure
        self.name = name
        self.isInitializer = isInitializer

    def call(self, interpreter: Interpreter, arguments: list) -> Any:
        environment = Environment(self.closure)

        for i, identifier in enumerate(self.callable.params):
            environment.define(identifier, arguments[i])
        
        result = None
        try:
            interpreter.executeBlock(self.callable.body, environment)
        except LoxReturn as returnValue:
            result = returnValue.value
        
        # constructors return 'this' implicitly, even when called directly.
        # not returning values in constructor is enforced by static analysis.
        if self.isInitializer and result is None:
            result = self.closure.getAt(0, "this")
        
        return result

    def arity(self) -> int:
        return len(self.callable.params)
    
    # only gets called on methods but like technically it could take any function and decide what object 'this' refers to. 
    # but for that do be interesting the resolver would have to allow it, and there's no like actual meaning for why you'd want that.
    def bind(self, instance: "LoxInstance") -> "LoxFunction":
        env = Environment(self.closure)
        env.rawDefine("this", instance)
        return LoxFunction(self.callable, env, self.name, isInitializer=self.isInitializer)
    
    def __str__(self) -> str:
        return "<fn " + self.name + ">"


class LoxInstance:
    klass: "LoxClass"
    fields: dict[str, Any]

    def __init__(self, klass: "LoxClass"):
        self.klass = klass
        self.fields = {}
    
    def get(self, name: Token) -> Any:
        if name.lexeme in self.fields:
            return self.fields[name.lexeme]
        
        method = self.klass.findMethod(name.lexeme)
        if method is not None:
            return method.bind(self)

        # If we were more like JS it would just return nil.
        # Maybe if i switch to more static checking i could make the interpreter more forgiving.
        # but then compiling to readable version of stricter language would't work without wrapping everything in a stupid way
        raise LoxRuntimeError(name, "Undefined property '" + name.lexeme + "'.")
    
    def set(self, name: Token, value: Any):
        self.fields[name.lexeme] = value
    
    def __str__(self) -> str:
        return "<inst " + self.klass.name + ">"


class LoxClass(LoxInstance, LoxCallable):
    name: str
    methods: dict[str, LoxFunction]
    superClass: "LoxClass | None"  # Object and MetaClass do not extend anything

    def __init__(self, name: str, methods: dict[str, LoxFunction], *, superClass: "LoxClass | None", metaClass: "LoxClass"):
        super().__init__(metaClass)
        self.name = name
        self.methods = methods
        self.superClass = superClass
    
    def findMethod(self, name: str) -> LoxFunction | None:  # split to do inheritance later
        if name in self.methods:
            return self.methods[name]
        
        if self.superClass is not None:
            return self.superClass.findMethod(name)
        
        return None
    
    def call(self, interpreter: Interpreter, arguments: list) -> LoxInstance:
        instance = LoxInstance(self)

        # run the constructor 
        initializer = self.findMethod("init")
        if initializer is not None:
            initializer.bind(instance).call(interpreter, arguments)
        
        return instance
    
    def arity(self) -> int:
        initializer = self.findMethod("init")
        if initializer is None:
            return 0
        else:
            return initializer.arity()
    

class MetaClass(LoxClass):
    def __init__(self):
        super().__init__("lang.Class", {}, metaClass=self, superClass=None)
    
    def call(self, interpreter: Interpreter, arguments: list) -> LoxInstance:
        raise NotImplementedError()
    
    def arity(self) -> int:
        raise NotImplementedError()
