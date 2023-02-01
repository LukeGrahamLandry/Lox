# A TREE-WALK INTERPRETER

import sys
from Scanner import Scanner

def run(src_code: str):
    pass

filename = sys.argv[1]
with open(filename, "r") as f:
    src_code = f.read()


print([str(x) for x in Scanner(src_code).scanTokens()])
