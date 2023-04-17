## Plan

What I care about is short and fast bytecode given to the runtime. 
Move smarts to the compiler so the vm can be as simple as possible. 
I don't care about compile time. Faster code is worth more passes. 

Judge accessing variable undeclared variable as though all functions were inlined. 
ie. The call must be after definition of all variables used but the body can reference variables 
that are declared after the function definition as long as they're in the same scope.

Write the bytecode generation code in lox. 

I still want to be able to compile code written in the book's version of lox.
So any extra syntax (types) I push to give hints to the optimiser, should be a super set, and it should just guess if it's missing. 
Guessing is good anyway ex. var x = call(); if I know call always returns an int, why am I writing the type of the variable again.  
The 

Add static typing as a pre-compilation scan so the vm can still run bytecode with dynamic types. 
Then the programmer can know there will be no runtime cast exceptions.
I want the vm to still work with dynamic bytecode because seems like cheating to make an easier vm by limiting the programmer.
But doing it this way means that the vm can't optimise. It still needs to keep the type tag on every value 
and do the type checks at runtime for every operation even if we know it was statically checked, so it can never fail. 

Ways to optimise that:
1. 
have an opcode to promise the code was prechecked. (or better OP_META_FLAGS that says read the next byte so i can have 8 flags if i want).
so when it sees that byte, it stops keeping type tags for values and doing runtime checks. 
the compiler just puts that byte at the beginning if it does the type checks. 
annoying that I'd have to check the flag repeatedly but one bool check must be faster than 
the type checking and makes the values use so much less memory (including in bytecode form) that it must be worth it.
Have the checking mode field as a local variable to make sure its on the stack for faster.
2.
The vm actually is statically typed. Every variable it sees is one type forever.
When the code says to reassign a variable to a new type, what its actually doing is creating a new variable 
and changing which the name refers to. That's what gets written as byte code. 
This option moves more brains to the compiler and makes a smaller vm, so it seems better. 
It's not cheating because what the language can do doesn't change, just where it gets interpreted.

Both of those or even forcing static types would save the space for the tag which might 4 bytes cause padding. 

The tag only exists for type checking. Obj would still need its tag, so it knows how to free. 
Need separate opcodes if they act on different types, like ADD vs CONCAT. 
The VM would just always cast to the type required by the operator so the Value struct doesn't need a ValueType field. 
== between different types would compile to false and for same types, take the ValueType as an extra argument. 

## Optimising Variables

Could have type tags for each thing in constants array packed in a separate array of longs, so it could look 
them up if it really wanted to. There's only 4 options and im afraid of the padding. 

A function is a variable whose value is a Chunk. 

Deduplication between function constant arrays and the main one? 
Should it deduplicate the constants arrays? You save memory but 
then have to spend time going through it each time a constant gets added 
but that's at compilation time so if its just interpreting source, don't 
but if you're compiling and saving the byte code, do extra work to make it shorter. 
Since it won't take more time at runtime. 

Guess the starting size for the hash table when parsing code. 

Why aren't classes like structs where all fields are just in order in an array and names correspond to indexes. 

Don't want to keep var name strings so just have a counter/random integer used instead. I can check for collisions at compile time 
but that doesn't work for dynamically loading code. Maybe it can only throw away names if it's compiling a whole scope at once. 

Should make sure I do importing files before I start messing with this, so I can test. 
Maybe scripts should explicitly export what they want to give to importers and the import keyword just 
runs the other script and pulls anything exported into our scope. 
Make sure you can do `export` as part of the variable definition. Maybe just use `public` to be the same as java. 

Other people's native calls need to keep the name string around>
But I can have an opcode for the standard library native calls where the compiler just puts the name's hash code 
and then since the vm knows all those names ahead of time i can have a table where those codes go to function pointers.
So two opcodes: OP_VM_NATIVE_CALL OP_FFI_NATIVE_CALL

Can do types from the bytecode by running through it and making a stack where the values are types. 
When you hit a condition, try both paths. 

### Line number tracking

run-length encoding is much better than the original but still feels dumb since the lines are almost always sequential
should be able to imply that the next is just one more but have an escape code encase how the compiling works changes.
feels like im slowly reinventing bytecode again. I could just use that,
put a chunk of bytecode to generate this list as a constant and if I need to show an error,
execute it then look up in the array. Good for when I want to write to file at least even if its overkill for runtime. 
don't want to recompute it every instruction while debugging, and it can't change so cache it. 
It's a lazily loaded value encoded as a pure function that gets cached. 
Every reference is inlined as `(name == nil ? name = computeName() : name)`, but if multiple references in same function, only the first needs the check. 
`lazy` modifier for a var. have it be context dependent. `lazy var` -> TOKEN_LAZY_VAR, `var` -> TOKEN_VAR, `lazy` -> TOKEN_IDENTIFIER

### Names

No global variables. I don't like the giant hash table in the sky.
Imports (and implied ones like native functions) take a stack slot.
Figure out a way of storing names of variables somewhere if it's told to save debug symbols.
Do that by default because it lets you have good error messages. Special compiler flag to omit names.
When generating bytecode it should tell you how much space each category takes
and remind you about the flags to optimise that but warn that it makes error messages and debugging worse.
Have a way for it to export just the name mapping information, so you can retroactively apply it to a bug report. 

### Bytecode Export

Instead of LOAD_INLINE_CONSTANT that just does one, so you have to repeat it for each. 
Have LOAD_INLINE_CONSTANT_ARRAY with a short argument for the length. 
Can grow the array to the correct length so a bit faster since no resize. 
Then it loops through that many times and loads them in sequence. 


### Macros

Written as functions that return code as a string. They run at compile time, arguments must be constants. 
When the compiler hits a macro call it stops, runs the code, and inserts the returned string instead to be compiled. 
Probably need some special keyword for recognising that the identifier should be treated specially. 
So the compiling function definition could add the name to the macro list and then whenever compiles a function call 
it checks against those names to see if it should run now instead of emitting the normal bytecode. 
Extends naturally to returning a string or just raw bytecode but that's probably annoying because I'd have to 
update any macros I write that way whenever I change by mind about the bytecode format. 
And also writing raw bytecode is annoying, there's a reason I have a compiler. 
If I find myself wanting to write bytecode because I can optimise it better, that's a sign that I need to add 
another pass to the optimiser that recognises that pattern and does the same transformation that I would do by hand. 
Would be cool if macros could return an abstract syntax tree instead. 
Could switch to that if I make one for type checking, etc. 

### Python block syntax

Have a way to signal to the scanner that it should use whitespace and colons to guess where to put
braces and semicolons. Python's syntax is still unambiguous ,and it's a context free transformation so
allowing it is just a change in the scanner. It can still emit the correct tokens to the compiler, so I
don't actually have to change the hard part.

Would be cool to write this scanner reimplementation in my language. Good way to make myself add the language
constructs for making it convenient to write the type of code that I want to. Just simple string manipulation
and a switch statement. Have a way to define a matcher for words that compiles down to the same failing early
type thing as the c one. That awkward character checking should be trivial to produce from something that looks
reasonable. Also gives me a reason to make an elegant story for calling lox code as a c++ function/class. 

### Annotations

Convenient way of writing some repeatable transformation to run on function or class definitions. 

```
@Whatever(1, 2)
fun something(a, b) {
    // do stuff
}
```

That would compile to the same as 

```
fun something(a, b) {
    // do stuff
}

something = Whatever(something, 1, 2);
```

Make it easy to add them in the same way. 

```
@FunctionAnnotationProcessor
fun Whatever(function, c, d) {
    fun replacement(a, b){
        // inject other stuff to do
        return function(a, b);
    }
    return replacement;
}
```

Actually, I don't even need to force declaring them with an annotation that way. 
There's nothing different about them. It just compiles them like a normal function call, 
and you just have to define the function that takes those arguments. 

As I get more advanced with type checking I could have a FunctionPrototype data class that gets passed into 
the annotation functions with extra information like arity and argument/return types. Same idea for classes 
it could give you names, setters, and getters for fields / methods. So this is how you'd do some of the stuff 
that reflection would be useful for. Like if the compiler always throws away names so reflection wouldn't make 
sense but the compiled annotation call gets a structure that keeps that information. Maybe keep the registering 
annotation as a way to say what information you need so the compiler can still throw it away sometimes. 
Make sure it only makes the object once per function/class even if you have multiple annotations on it, otherwise 
it would sometimes be more wasteful than just natively keeping the reflection information. Could have a basic 
@Reflectable annotation that just saves the info to be accessed later at runtime. 

Allow annotations to make you implement an interface. But then I'd need to keep class properties dynamic at 
runtime and can't do good static checking if you don't know what methods will exist until you run the code. 
Maybe better to allow implementing interfaces to inherit class annotations, but they act on the child class now.
So an interface can choose to also be an annotation where it generates the body of its added methods based on the 
reflection type information. So I need a way for an interface to declare certain methods as generated like annotations. 
Allow interfaces to have static methods that apply to the child class. Static methods on an interface are like 
saying that the instance of Class should actually be a subclass that implements the static part of the interface. 

Use automatic json serialization as the target example and design the system I need to make its implementation elegant. 

```
class Point < Serializable {
    var x: Number;
    var y: Number;
}
```

That simple class needs to expand out into the code below. 

```
class Point < Serializable {
    var x: Number;
    var y: Number;
    
    fun toJson(): String {
        return JsonHelper.dumpJson({
            "x": this.x,
            "y": this.y
        });
    }
    
    static fun fromJson(str: String): Point {
        var data = JsonHelper.loadJson(str);
        return Point(data["x"], data["y"]);
    }
}
```

The question is how to allow the interface to state that it's actually an annotation processor elegantly. 
So some methods are default implementations, some are abstract the child must implement, and some are 
abstract but the implementation is generated based on the child. Static methods on an interface are inherited as
static methods on the child.

```
class ClassTransformer<T>{
    fun init(klass: ClassDefinition<T>);
    fun transform();
}

class JsonHelper<T> < ClassTransformer<T> {
    static loadRawJson(str: String): Map<String, Value> {
        // do stuff
        return data;
    }
    
    static dumpRawJson(data: Map<String, Value>): String {
        // do stuff
        return str;
    }
    
    fun init(var klass: ClassDefinition<T>);
        
    fun read(data: Map<String, Value>): T {
        var self = klass.createInstance();
        for (var field in klass.getFields()){
            var value = data[field.getName()];
            field.setValue(self, value);
        }
        return self;
    }
    
    fun write(self: T): Map<String, Value> {
        var data = {};
        for (var field in klass.getFields()){
            data[field.getName()] = field.getValue(self).toJson();
        }
        return data;
    }
    
    fun transform(){
        klass.getStaticField("fromJson").set(this.read);
        klass.getField("toJson").set(this.write);
    }
}

import JsonHelper
@JsonHelper
class Serializable {
    fun toJson(): Map<String, Value>;
    static fun fromJson(str: Map<String, Value>): THIS;
}

import Serializable
class Point < Serializable {
    var x: Number;
    var y: Number;
    
}
// Generated
{
    import ClassDefinition
    var _classdef_ = /* native instance from compiler */;
    JsonHelper(_classdef_).transform();
    Point = _classdef_.build();
}
```

Feels reasonable to just have another interface the interface can implement to transform the ClassDefinition. 
It can only call setSuperMethod to replace an existing/inherited method. Not allowing the prototypes to be dynamic 
means I can still do type checking. Should have setSuperMethod so the child class can still override 
the method and call the injected one to do part of the work. 

Maybe it's wierd that you never know when implementing an interface will generate code for you. 
But I like the idea of hiding the implementation details because as the programmer, I really don't care 
where the json comes from, I just want to not spend time writing repetitive code. It would be wierd if 
the annotation could implement an interface for you so you never know what things you implement. 
It would be annoying to duplicate the name by needing to state the interface and also have the annotation. 

To solidify my idea, macros are compile time code generation. They add no overhead at runtime but can only use constants. 
Annotations are runtime code generation, they run once when the function/class is defined and replace the object the 
symbol refers to. That gives them more power since they can use any normal program state before that definition. 
Maybe there's no point in macros, annotations are always a safer way to do it. 

### Use it for minecraft mods

Encourages writing as much of my standard library in my language instead of c, so I can reuse it. 
Maybe things like string manipulation should still be native since I care about them being fast and the 
java ones can generally just delegate to the java String class so that side of the implementation is easy.

I like the idea of escaping the annoying boilerplate of setting up the project by just having it be a script. 
Expose the lowest common api, so it can work for many loaders and versions. Be able to just drop it in the mods folder.
Have a way to bundle it into a jar for curseforge and nice automation for publishing. 

Go through all my mods and have an api for doing those things. 

```
record LoxMethod {
    Class self;
    String name;
    int arity;
    LoxCallable function;
}

interface LoxCallable {
    Value call(Object self, Value[] arguments)
}

class ObjPlayer {
    private PlayerEntity player;
    public ObjPlayer(PlayerEntity player) {
        this.player = player;
    }
    
    float getHealth(){
        return player.getHealth();
    }
    
    void setHealth(float health){
        player.setHealth(health);
    }
}
```

Annotation processor for events. 
Should be easy to write event objects in java and expose them to the vm as natives with reflection.

```
class PlayerEvent {
    public ObjPlayer player;
    
    public static class Login extends PlayerEvent {}
    public static class Logout extends PlayerEvent {}
}

// script api

enum LogicalSide {
    CLIENT,
    SERVER,
    COMMON
}

Map<LogicalSide, Map<String, List<Callable>>> handlers;

fun triggerEvent(name: String, side: LogicalSide, data: Value){
    for (var handler in handlers.get(side).get(name)){
        handler(data);
    }
    
    for (var handler in handlers.get(LogicalSide.COMMON).get(name)){
        handler(data);
    }
}

class Subscriber {
    static fun register(action: Callable, event: String, side: LogicalSide){
        handlers.get(side).get(event).add(action);
    }
    
    static fun auto(func: FunctionDefinition, side: LogicalSide){
        var event = func.getArguments()[0].getTypeName();
        register(func.function, event, side);
    }
    
    static fun Client(func: FunctionDefinition) auto(func, LogicalSide.CLIENT);
    static fun Server(func: FunctionDefinition) auto(func, LogicalSide.SERVER);
    static fun Common(func: FunctionDefinition) auto(func, LogicalSide.COMMON);
}

// main.llox

from minecraft import Subscriber, PlayerEvent

@Subscriber.Server
fun onEvent(event: PlayerEvent.Login){
    if (event.player.getHealth() > 10){
        event.player.setHealth(10);
    }
}

import ModMetaData

return ModMetaData(
    name = "Example Mod",
    modid = "examplemod",
    version = "1.0.0",
    author = "Unknown",
    license = "ARR"
);
```

The quest is to make things more expressive than java. 
Sum types instead of exceptions could be interesting. Force a match statement to handle all cases to unwrap. 

Don't even need a complicated dev environment with decompiled code, it's just a script, and you can use the 
reload command. Means I need a compiler written in my language. 

Some sort of namespacing where you can reference code from other script mods by mod id. 
Allow directly referencing java classes by name.

It would be very nice to allow loading scripts from the server but then security becomes a problem. 
Could have permissions system: files, network, reflection. 

Have a way to toggle on development mode that watches files on disk and reloads. 

Have a consistent networking api that works by having data objects that get encoded to json. 
Have an implementation as minecraft packets and as normal http, so I could extend to using it for websites. 

Simple template jar that registers itself with the mod's system. Just have the scripts as resource files in the jar 
and I can probably create them by just manipulating it as a zip file. All the fabric versions at least should 
be able to use the same jar. 

If I write wrappers for all the minecraft classes, I don't have to deal with mappings. And it helps to do 
everything that way to not worry about consistency when versions change things. 

Make sure I have comprehensive tests, so I don't have to worry about bug reports. 

I want nice integration with geckolib and smartbrainlib. 
WrapperLib already did a bunch of the work for consistent apis for data manipulation stuff. 

None of the fucking server vs client side classes stuff. If it tries to reflect on a class that doesn't exist 
just log it. 

Do anything that doesn't have to talk to minecraft in java/lang here. Anything that does talk to minecraft 
needs many versions so should have its own repo for branches. Anything that doesn't need to talk to java should 
go in src/lang written in the language, so I can reuse for any other backend implementations I end up making. 

## Self-hosted compiler

```
// Better interface for writing the scanner. Compiles down into that switch statement. 
class StringMatcher {
    fun add(str, value);
    fun match(str): Value;
}
```

Abstract syntax tree is probably better for optimising and type checking. 
Good application for having the sum types stuff. 

Tempting to not open source it. Do I want to support obfuscation? I suppose super easy to allow 
those jar files to have the precompiled bytecode. When doing release builds, it could probably just generate java 
source and compile that, so it doesn't have to deal with the slow vm. 
