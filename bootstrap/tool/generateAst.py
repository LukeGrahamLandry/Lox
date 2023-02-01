from unicodedata import name


def output(s):
    print(s)

class Field:
    type: str
    name: str

    def __init__(self, s) -> None:
        self.type = s.split(" ")[0]
        self.name = s.split(" ")[1]
        if self.type == "Object":
            self.type = "Any"

def generate(baseName: str, entries: list[str]):
    output("from typing import Any")
    output("from Token import Token")
    output("")
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
    for entry in entries:
        className = entry.split(":")[0].strip()
        fields = entry.split(":")[1].strip().split(", ")

        generateVisitor(baseName, className, [Field(s) for s in fields])

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


generate("Expr", [
    "Binary   : Expr left, Token operator, Expr right",
    "Grouping : Expr expression",
    "Literal  : Object value",
    "Unary    : Token operator, Expr right"
])

