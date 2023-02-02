from Token import Token
from jlox import LoxRuntimeError
from typing import Any
import json

# applies to variables and functions
# applies to reusing parameter name in functions
# False is like JS (var). True is like Java, JS (let).
# TODO: this would prevent function overloading but i don't have that yet anyway
prevent_redeclare_var_in_same_scope = True

class Environment:
    values: dict[str, Any]
    def __init__(self, enclosing=None) -> None:
        self.values = {}
        self.enclosing = enclosing
    
    def define(self, name: Token, value):
        if name.lexeme in self.values and prevent_redeclare_var_in_same_scope: 
            raise LoxRuntimeError(name, "Cannot declare variable '" + name.lexeme + "' twice in the same scope.")
        
        self.values[name.lexeme] = value
    
    def rawDefine(self, name: str, value):
        self.values[name] = value
    
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
    
    def __str__(self) -> str:
        return self.toString(0)

    def toString(self, indent: int) -> str:
        info = [{}]

        s = (" " * indent) + "* " + str(indent) +"\n"
        for name, value in self.values.items():
            s += (" " * (indent+1)) + "- " + name + ": " + str(value) + "\n"
        
        if self.enclosing is not None:
            s += self.enclosing.toString(indent + 1)
        
        return s
