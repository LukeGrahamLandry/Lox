## Expressions 

In reverse order of precedence. 

- or
- and
- ==, !=
- <, <=, >, >=
- +, -
- *, /
- **
- !, -
- function call, property access
- literal, variable identifier, bracketed expression

### Literals 

- Booleans: true, false
- Numbers: 1, 2.3
- Strings: "foo"
- nil
- Anonymous Functions
    ```
    fun (a, b){
        // body
    }
    ```
- Anonymous Classes
    ```
    class {
        // body
    }
    ```

## Statements

- Expression;
- print Expression;
- (variable declaration)
    ```
    var IDENTIFIER = Expression;
    ```
- (conditional)
    ```
    if (Expression){
        Statement
    } else {
        // optional
        Statement
    }
    ```
- (while loop)
    ```
    while (Expression){
        Statement
    }
    ```
- (for loop)
    ```
    for (Statement; Expression; Expression){
        Statement
    }
    ```
- break; continue;
    - only valid inside loop body
- (function declaration)
    ```
    fun IDENTIFIER(a, b){
        Statement
    }
    ```
- return Expression;
    - only valid inside function body
- (class declaration)
    ```
    class IDENTIFIER{
        fun init(){
            // constructor method body
        }
    }
    ```
    - (function declaration)
        - `this` is injected as a variable in the body scope referring to the current instance
    - static (variable/function/class declaration)
