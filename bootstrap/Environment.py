from Token import Token
from jlox import LoxRuntimeError

class Environment:
    def __init__(self, enclosing=None) -> None:
        self.values = {}
        self.enclosing = enclosing
    
    def __str__(self) -> str:
        s = str(self.values)
        if self.enclosing is not None:
            s += " (" + str(self.enclosing) + ")"

        return s
    
    def define(self, name: str, value):
        self.values[name] = value  # TODO: don't allow redefining 
    
    def get(self, name: Token):
        if name.lexeme in self.values:
            return self.values[name.lexeme]
        
        if self.enclosing is not None:
            return self.enclosing.get(name)
        
        raise LoxRuntimeError(name, "Undefined variable '" + name.lexeme + "'.")
    
    def assign(self, name: Token, value):
        if name.lexeme in self.values:
            self.values[name.lexeme] = value
        elif self.enclosing is not None:
            self.enclosing.assign(name, value)
        else:
            raise LoxRuntimeError(name, "Undefined variable '" + name.lexeme + "'.")
