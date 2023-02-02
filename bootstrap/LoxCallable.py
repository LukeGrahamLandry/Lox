from pickletools import int4
from typing import Any
from time import time
import generated.stmt as stmt
from Environment import Environment

class LoxCallable:
    def call(self, interpreter, arguments: list) -> Any:
        raise NotImplementedError()
    
    def arity(self) -> int:
        raise NotImplementedError()

class LoxNativeFunction(LoxCallable):
    def __str__(self) -> str:
        return super().__str__().split(" ")[0] + ">"

class Clock(LoxNativeFunction):
    def call(self, interpreter, arguments: list) -> float:
        return time()
    
    def arity(self) -> int:
        return 0

class GetEnvironmentString(LoxNativeFunction):
    def call(self, interpreter, arguments: list) -> str:
        return str(interpreter.currentScope)
    
    def arity(self) -> int:
        return 0
