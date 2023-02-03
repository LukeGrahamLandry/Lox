from Token import Token
from jlox import LoxRuntimeError
from typing import Any

class Environment:
    values: dict[str, Any]
    def __init__(self, enclosing=None) -> None:
        self.values = {}
        self.enclosing = enclosing
    
    def define(self, name: Token, value):
        self.values[name.lexeme] = value
    
    def rawDefine(self, name: str, value):
        self.values[name] = value
    
    def get(self, name: Token):
        if name.lexeme in self.values:
            return self.values[name.lexeme]
        
        if self.enclosing is not None:
            return self.enclosing.get(name)
        
        raise LoxRuntimeError(name, "Undefined variable '" + name.lexeme + "'.")
    
    def getAt(self, distance: int, name: str) -> Any:
        try: 
            return self.ancestor(distance).values[name]
        except KeyError as e:
            print("variable", name, "not found at distance", distance)
            print(self)
            raise LoxRuntimeError(None, "")
               
        
    def assignAt(self, distance: int, name: str, value: Any):
        self.ancestor(distance).values[name] = value
    
    def ancestor(self, distance: int) -> Any:
        env: Any = self
        for i in range(distance):
            env = env.enclosing
        return env
    
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
