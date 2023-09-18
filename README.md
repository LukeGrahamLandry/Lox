# Lox-lang

Several implementations of Lox, the language described in [Crafting Interpreters by Robert Nystrom](https://craftinginterpreters.com). Lox is a dynamically typed, garbage collected, programming language that runs on a virtual machine. 

# A byte-code virtual machine 

- clox implemented in c++

> Perhaps the most awkward hodgepodge of c++, a language I learned largely by following a book that used c. 

## Make Targets

### web

Compiled to web assembly (requires emscripten installed). There's a little ui that loads the vm in a web worker and lets you run scripts. The js/html files will be in the `out` directory. 

### native 

Builds a native executable for running lox programs. 

- Run with no arguments to enter REPL
- Run with path to script as argument to execute it. 

### debug

Builds an unoptimised executable with address sanitizer and runs gc as often as possible to help catch bugs. 
This is the build used by the test runner. It's ~20x slower than the normal version. 

### test

Runs [the tests from the book](https://github.com/munificent/craftinginterpreters/tree/master/test) and files in `tests/cases` and checks their output. 

For the book tests that expect errors, it doesn't check that the message matches exactly, just that there's any error at all. 
It also skips a few about global variables because my implementation doesn't give them special treatment. 

## Extensions 

- `continue` and `break` from loops. 
- `s[index]` or `s[start:end]` (Index and slice strings. Negative indexes start from the end)
- `condition ? true_value : false_value`
- `a ** b` (a to the power of b)
- `debugger;` (print info about the state of the vm)
- `import function_name;` or `import ClassName` (to import builtins)
	- `time() -> number`: Get the number of seconds since the UNIX epoch.  
<!--
	- `getc() -> number`: Read a single character from stdin and return the character code as an integer. Returns -1 at end of input. 
	- `chr(ch: number) -> string`: Convert given character code number to a single-character string. 
	- `exit(status: number)`: Exit the process and return the given status code.
	- `print_error(msg: string)`: Print message string on stderr.
	- `sleep(ms: number)`: Suspend the program for some number of milliseconds. 
-->

# A tree-walk interpreter

- jlox implemented in python

Less interesting than clox but a good starting point. Some additional features are added, see the sub-directory's README. 
