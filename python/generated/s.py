
from typing import Any

class Stmt:
    def accept(self, visitor: Any):
        raise NotImplementedError()
