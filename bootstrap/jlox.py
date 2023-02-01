# A TREE-WALK INTERPRETER

import sys

hadError = False
def reportError(line: int, message: str, where: str = ""):
    print("[line " + str(line) + "] Error" + where + ": " + message)
    hadError = True


def run(src_code: str):
    pass

if __name__ == "__main__":
    from Scanner import Scanner
    from Parser import Parser
    from generated.expression import Expr
    from AstPrinter import AstPrinter

    filename = sys.argv[1]
    with open(filename, "r") as f:
        src_code = f.read()

    tokens = Scanner(src_code).scanTokens()
    parser = Parser(tokens)
    expression = parser.parse()

    if not hadError and expression is not None:
        print(AstPrinter().print(expression))
