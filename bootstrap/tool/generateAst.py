from unicodedata import name

current = ""

generated_code = {}

def output(s):
    generated_code[current] += s + "\n"

class Field:
    type: str
    name: str

    def __init__(self, s) -> None:
        self.type = s.split(" ")[0]
        self.name = s.split(" ")[1]
        if self.type == "Object":
            self.type = "Any"

def generate(baseName: str, entries: list[str]):
    generated_code[baseName] = ""
    global current
    current = baseName

    output("from typing import Any")
    output("from Token import Token")
    output("from generated.s import Stmt")
    output("")
    if baseName != "Stmt":
        output(f"class {baseName}:")
        output("    def accept(self, visitor: Any):")
        output("        raise NotImplementedError()")
        output("")

    for entry in entries:
        className = entry.split(":")[0].strip()
        fields = entry.split(":")[1].strip().split(", ")

        generateType(baseName, className, [Field(s) for s in fields])
    
    output("")
    output("class Visitor:")
    entryNames = [baseName]
    for entry in entries:
        className = entry.split(":")[0].strip()
        fields = entry.split(":")[1].strip().split(", ")
        entryNames.append(className)

        generateVisitor(baseName, className, [Field(s) for s in fields])
    
    output("")
    output("__all__ = " + str(entryNames))

def generateType(baseName: str, className: str, fields: list[Field]):
    output(f"class {className}({baseName}):")
    constructor_header = "    def __init__(self"
    constructor_body = ""
    
    for field in fields:
        output(f"    {field.name}: {field.type}")
        constructor_header += f", {field.name}: {field.type}"
        constructor_body += f"      self.{field.name} = {field.name}\n"

    output("")
    output(constructor_header + "):\n" + constructor_body)

    output("    def accept(self, visitor: Any) -> Any:")
    output(f"        return visitor.visit{className}{baseName}(self)")
    output("")

def generateVisitor(baseName: str, className: str, fields: list[Field]):
    output(f"    def visit{className}{baseName}(self, {baseName.lower()}: {className}) -> Any:")
    output("        raise NotImplementedError()")


generated_code["s"] = """
from typing import Any

class Stmt:
    def accept(self, visitor: Any):
        raise NotImplementedError()
"""

generate("Expr", [
    "Binary   : Expr left, Token operator, Expr right",
    "Grouping : Expr expression",
    "Literal  : Object value",
    "Unary    : Token operator, Expr right",
    "Variable : Token name",
    "Assign   : Token name, Expr value",
    "Logical  : Expr left, Token operator, Expr right",
    "Call     : Expr callee, Token paren, list[Expr] arguments",
    "FunctionLiteral : list[Token] params, list[Stmt] body"
])

generate("Stmt", [
      "Expression : Expr expression",
      "Print      : Expr expression",
      "Var        : Token name, Expr initializer",
      "Block      : list[Stmt] statements",
      "If         : Expr condition, Stmt thenBranch, Stmt elseBranch",
      "While      : Expr condition, Stmt body",
      "Throwable  : Token token",
      "FunctionDef   : Token name, FunctionLiteral callable",
      "Return     : Token keyword, Expr value"
])

for current, code in generated_code.items():
    with open("bootstrap/generated/" + current.lower() + ".py", "w") as f:
        if current == "Stmt":
            f.write("from generated.expr import *\n")
            f.write("from Token import *\n")
        f.write(code)