# A TREE-WALK INTERPRETER

import sys

from Token import Token, TokenType

class LoxRuntimeError(RuntimeError):
    displayMsg: str
    
    def __init__(self, operator: Token | None, message: str):
        if operator is None:
            self.displayMsg = "Unspecified LoxRuntimeError Error"
        else:
            self.displayMsg = f"{message}\n[line {operator.line}]"
        super().__init__(self.displayMsg)
    
    def __str__(self) -> str:
        return self.displayMsg


class LoxSyntaxError:
    displayMsg: str
    def __init__(self, line: int, message: str, where: str = ""):
        self.displayMsg = "[line " + str(line) + "] Error" + where + ": " + message
    
    @staticmethod
    def of(token: Token, message: str):
        if (token.type == TokenType.EOF):
            where = " at end"
        else:
            where = " at '" + token.lexeme + "'"

        return LoxSyntaxError(token.line, message, where)

    def __str__(self) -> str:
        return self.displayMsg


def printErrorsAndMaybeExit(errors: list, exitCode: int):
    for error in errors:
        print(error)
    if len(errors) > 0:
        sys.exit(exitCode)

if __name__ == "__main__":
    from Scanner import Scanner
    from Parser import Parser
    from AstPrinter import AstPrinter
    from Interpreter import Interpreter
    from Resolver import Resolver

    interpreter = Interpreter()

    if len(sys.argv) > 1:
        filename = sys.argv[1]
        with open(filename, "r") as f:
            src_code = f.read()

        scanner = Scanner(src_code)
        tokens = scanner.scanTokens()
        parser = Parser(tokens)
        statements = parser.parse()
        # print(AstPrinter().printStatement(statements))
        # print("========")
        printErrorsAndMaybeExit(scanner.errors + parser.errors, 65)

        resolver = Resolver(interpreter)
        resolver.resolveStmt(statements)

        printErrorsAndMaybeExit(resolver.errors, 1)
        
        interpreter.interpret(statements)
        printErrorsAndMaybeExit(interpreter.errors, 70)
        
    else:
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

            statements = parser.parse()
            interpreter.interpret(statements)

            
