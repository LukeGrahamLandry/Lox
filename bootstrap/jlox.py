# A TREE-WALK INTERPRETER

import sys

from Token import Token

class LoxRuntimeError(RuntimeError):
    def __init__(self, operator: Token, message: str) -> None:
        super().__init__(message + " " + str(operator))
        self.operator = operator
        self.message = message
    

hadError = False
hadRuntimeError = False
def reportError(line: int, message: str, where: str = ""):
    print("[line " + str(line) + "] Error" + where + ": " + message)
    hadError = True

def reportRuntimeError(error: LoxRuntimeError):
    print(f"{error.message}\n[line {error.operator.line}]")
    hadRuntimeError = True

def run(src_code: str):
    pass

if __name__ == "__main__":
    from Scanner import Scanner
    from Parser import Parser
    from generated.expression import Expr
    from AstPrinter import AstPrinter
    from Interpreter import Interpreter

    if len(sys.argv) > 1:
        filename = sys.argv[1]
        with open(filename, "r") as f:
            src_code = f.read()

        tokens = Scanner(src_code).scanTokens()
        parser = Parser(tokens)
        expression = parser.parse()

        if not hadError and expression is not None:
            print(AstPrinter().print(expression))
            Interpreter().interpret(expression)
    
    else:
        interpreter = Interpreter()
        while True:
            hadError = False
            src_code = input(">>> ")
            if src_code in ["exit", "exit()"]:
                break

            tokens = Scanner(src_code).scanTokens()
            if hadError:
                continue
            parser = Parser(tokens)
            if hadError:
                continue
            expression = parser.parse()

            if expression is not None:
                print(AstPrinter().print(expression))
                interpreter.interpret(expression)
            
