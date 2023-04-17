### Language Additions

- block comments (/* ... */)
- exponent operator (**) 
- jump keywords for loops (break, continue)
- anonymous functions (`fun (a, b) {return a + b;}`)
- more native functions
    - environment: returns string of all variables
- more static analysis errors
    - jump keyword outside loop
    - unused function or local variable
    - access undefined variable
    - unreachable code after return/break/continue statement
- class methods must be preceded by the fun keyword. 
    - doesn't actually make parsing easier, i just think its more consistent. 
- classes can have static fields, functions, and classes (static var, static fun, static class)
    - each class is an instance of MetaClass so static things are accessed as properties 
- anonymous classes (`class {fun init(a){ this.a = a;}}`)

## Scanner

Converts a string of source code into a list of tokens. 

## Parser

Converts a list of tokens into an expression AST.

(low index = high level = low precedence)

1. equality (==, !=)
2. comparison (>, >=, <, <=)
3. term (+, -) 
4. factor (*, /)
5. exponent (**)
6. unary (!, -)
7. primary (literal or bracketed expression)

Find an thing of the highest level. If not found, go one lower. 
- binary operators need two operands
    - for left, find a thing of the next highest level. that will end up consuming everything to the left of the operator 
    - for right, find a thing of the next highest level, now you're after the operator, consuming the rest of the statement. 
    - operators with the same level happen left to right
- unary operators need one operand
    - find a thing of the next highest level. it will be after the operator. 
- primary
    - create string/float literal node
    - recursively parse the bracketed expression

A negative sign doesn't get caught as a subtraction because before `term` can consume the operator it reads an expression of the next level down which will consume the minus sign if there's no expression before it. 

It sees the lowest precedence thing first but that means it has to evaluate everything around it first. So the highest precedence things are seen last but put first into the tree and then evaluated first. 

## Interpreter

Converts an expression AST into a value.

It takes an expression and evaluates it recursively. 

- literal: return the value
- grouped: evaluate the expression 
- unary: evaluate the expression then switch over the operator token type to modify the return value
- binary: evaluate both expressions then switch over the operator token type to combine the values and return the result

So that starts at the top of the tree, which is the lowest precedence thing but the bottom leaves of the tree are evaluated first and it works its way up as it pops out of the recursion. 
