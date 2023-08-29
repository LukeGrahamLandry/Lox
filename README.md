# Lox-lang

Several implementations of Lox, the language described in [Crafting Interpreters by Robert Nystrom](https://craftinginterpreters.com). Lox is a dynamically typed, garbage collected, programming language that runs on a virtual machine. 

# A byte-code virtual machine 

- clox implemented in c++

There are three targets that can be built with make. Build artifacts will be in the `out` folder. 

### web

Compiled to web assembly (requires emscripten installed). There's a little ui that loads the vm in a web worker and lets you run scripts. The js/html files will be in the `out` directory. 

### native 

A native executable for running lox programs. 

- Run with no arguments to enter REPL
- Run with path to script as argument to execute it. 

### tests

Runs the lox files in `tests/cases` and checks their output. Requires being run from the cpp directory so it can find the test scripts. 

## Extensions 

- `continue` and `break` from loops. 
- `s[index]` or `s[start:end]` (Index and slice strings. Negative indexes start from the end)
- `condition ? true_value : false_value`
- `a ** b` (a to the power of b)
- `debugger;` (enter repl to inspect a certain point in script)
- `import function_name;` or `import ClassName` (to import builtins)
	- `clock() -> number`: Get the number of seconds since the program started. 
	- `time() -> number`: Get the number of seconds since the UNIX epoch.  
	- `getc() -> number`: Read a single character from stdin and return the character code as an integer. Returns -1 at end of input. 
	- `chr(ch: number) -> string`: Convert given character code number to a single-character string. 
	- `exit(status: number)`: Exit the process and return the given status code.
	- `print_error(msg: string)`: Print message string on stderr.
	- `sleep(ms: number)`: Suspend the program for some number of milliseconds. 
	- `typeof(any)`
	- `Array`

# A tree-walk interpreter

- jlox implemented in python

Less interesting than clox but a good starting point. Some additional features are added, see the sub-directory's README. 
