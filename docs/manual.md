# The Logical Reference Manual

Version: 2020-05-06

## Running Logical

## Building Logical

## Running tests

## Command line arguments



# The Logical Language

## Unary predicates

A *predicate* defines something to be true, and a predicate in Logical is simply a table of data. A *unary predicate* is a table with a single column and a variable number of rows. If the table is empty, then the predicate is said to be empty and contains zero rows.

Predicates can be defined using data, or rules. Unlike regular Datalog, Logical defines its rules using a natural syntax.

Facts are statements that simply define a certain thing to be true, and add the values directly to the table. Here are some facts:

```
"Fred" is a person.
3.14 is pi.
@ape1 is an ape.
```

`is`, `a` and `an` are keywords. `a` and `an` are optional and are used to make the statement read more naturally. It is also possible to write the predicate first, as follows:

```
person "Fred".
pi 3.14.
ape @ape1.
```

In these statements, `person`, `pi` and `ape` are unary predicates, and `"Fred"`, `3.14` and `@ape1` are entites that are stored in the table.

## Entities

An entity is something that is stored in a table, and represents one of the values in a Logical program. The types of entity are

* Strings, for example `"abc"`. Strings can contain special escape characters `\"`, `\t`, `\n`, `\r`.
* Integers, for example `1234`.
* Floating point numbers, for example `3.14`. Numbers are in IEEE notation, however they must not end in a trailing dot.
* @-entities, for example `@ape1`. These can be used for synthesised identifiers, and can contain any character except `.` and whitespace. They can also consist of a single `@`.
* Boolean values, `true` and `false`.

In Logical, predicates are untyped, and it is possible to mix any type of data in a table, for example

```
person "Fred".
person @person1.
```

## Compound predicates

Logical supports an unusual syntax where multiple predicates can be defined at the same time. For example

```
large mouse "Mickey".
small red mouse "Mini".
```

alternatively,

```
"Mickey" is a large mouse.
"Mini" is a small red mouse.
```

This is equivalent to writing out

```
"Mickey" is large.
"Mickey" is a mouse.
"Mini" is small.
"Mini" is red.
"Mini" is a mouse.
```

Remember, `a` is simply syntactic sugar and can be omitted.

Compound syntax also appears in rules, where `X is a large mouse` means `X is large and X is a mouse` - more on that later.

Note that `large-mouse "Mickey"` and `large mouse "Mickey"` mean different things. The first defines a single predicate called `large-mouse` and the second defines two predicates `large` and `mouse`.

## Rules with unary predicates

## Binding

## Special predicates

## Negation

## Rules

## or

## and

## Binary predicates

## Datalog syntax

## Recursion

## Transitive closures

Because the transitive closure pattern is so common, Logical has introduced a special keyword for it. `reaches`

Grammar:

> reaches-clause:
>     unary-predicate-list entity *reaches* binarypredicate entity


## New entities
The `new` keyword creates new entities in the database. This is unlike rules, which only use existing entities, or facts that use hard-coded entities. New entities offer a convenient way to package together a set of attributes into a single entity.

`new` can be used in facts as follows:
```
new person has name "Fred", age 99.
```
This creates a new `person` entity that is guaranteed to be unique, and there is no need to synthesise an identifier for it.

`new` can also be used in rules, for example

```
new woman has name n, age a if person p has name n, age a, gender "female" and a>=18.

new parameter has name n, function f, index i if function f has parameter-name n, parameter-index i.

new number has value 0.
new number has value v+1, predecessor p if number p has value v and v<10.

new location has
    file f,
    start-line l1,
    start-column c1,
    end-line l2,
    end-column c2
if
    function _ has file f, start-line l1, start-column c1, end-line l2, end-column c2.
```
As you can see, `new` entities can be defined recursively.

If there are multiple rules, then this will define multiple entities, even with the same attributes.
```
new person has name "Fred".
new person has name "Fred".

new file has name f, parent d if directory d has file f.

new person has name n+" Smith" if person _ has name "Fred".
```

# External functions

## Using external functions

External functions (externs) allow Logical code to call external C++ code, for example to implement string functions, math functions, load external data, read CSV files, query databases and so on.

Externs are implemented in `.dll` or `.so` files, that are compiled separately to Logical itself. An external module is loaded using the `load-module` predicate. For example

```
load-module "stdlib".
```

This will look for the file `stdlib.dll` or `stdlib.so` (depending on your platform), and load the externs defined in it.

Once loaded, the externs will be available as any normal predicate (there is no concept of a module/namespace system), although the caller must ensure that the binding is correct or you will get an error.

Externs cannot be redefined as normal predicates, so it is an error for a predicate to be both an extern or one that has been defined using statements (facts and rules). Attempts to "define" an externs actually call the function, or run the rules immediately.

External functions can have different implementations for different bindings, although semantically they should behave consistently.

External functions must always be called with a qualified "object", for example
```
number 2 has sqrt X.
```
is correct, but
```
2 has sqrt X  // Incorrect
```
is incorrect because `2` has not been qualified, and will try to call a user-defined predicate instead. (The reason for this is to ensure that externs don't interfere with user-defined predicates too much.).

## Defining externs

Externs are written in C++, and compiled to a `dll`/`so` file. The module must contain a `void RegisterFunctions(Logical::Module&)` C++ function that is run when the module is loaded.

The supplied `RegisterFunctions()` function calls `RegisterFunction()` to register each extern. `RegisterFunctions()` calls `RegisterFunction()` on the supplied module, passing in the extern, the parameter names and the parameter bindings for each extern it defines.

Externs can be defined multiple times. If the same extern is defined with a different binding (a different combination of `In`/`Out`), then Logical will pick the correct version to call. If the extern is redefined with the same names and bindings, then the previous extern will be replaced without error.

The header file to include is `Logical.hpp`, which defines the external API for Logical.

Externs must have the signature `void(Logical::Call&)`, and do not need to be public. The extern can do the following:

- Call `call.YieldResult()` to "return" a result. This can be called multiple times by the same extern to "return" multiple results.
- Call `call.Get()` to read a value
- Call `call.Set()` to output a value. (You must still call `YieldResult` to pass the value to the program.)

This is better explained with an example. Here is a module that defines a `sqrt` function, but there are a couple of interesting twists. The first twist is that the `sqrt` function returns both the positive and the negative square roots, so it calls `YieldResult()` twice. (For the case where we compute the root of `0`, it yields `0` twice. This is not a big problem, but as an exercise, how would the code change to fix this?)

The second twist is that we can also run the algorithm "backwards" to compute the square of a number. This version only yields one result, so only calls `YieldResult()` once.

```
#include <Logical.hpp>
#include <math>

using namespace Logical;

// Defines the "sqrt" extern for the case where the object is bound and the subject is unbound.
// For example `number 2 has sqrt X`
static void sqrtBF(Call & call)
{
    double number;

    // Read the first argument as a double
    if(call.Get(0, number))
    {
        auto s = std::sqrt(number);

        // Write the second argument
        call.Set(1, s);

        // Return the first result
        call.YieldResult();

        // Write the second argument for the second result
        call.Set(1, -s);

        // Return the second result
        call.YieldResult();
    }
    // else: The first argument is not a double, so "fail" by never calling `YieldResult()`
}

// Defines the "sqrt" extern for the case where the object is unbound and the subject is bound.
// For example `number X has sqrt 2`
static void sqrtFB(Call & call)
{
    double number;

    // Retrieve the second argument as a double
    if(call.Get(1,number))
    {
        // Write the result into the first argument
        call.Set(0, number*number);

        // Return the result
        call.YieldResult();
    }
    // else: The second argument is not a number, so "fail" by never calling `YieldResult()`
}

extern "C" void RegisterFunctions(Module & module)
{
    module.RegisterFunction(sqrtBF, In, "number", Out, "sqrt");
    module.RegisterFunction(sqrtFB, Out, "number", In, "sqrt");
}
```

If an integer is passed as an argument, then `Get()` will silently convert the `int` to a `double`, but it will not silently convert a `double` to an `int`.

External functions must be thread-safe and reentrant.

Calls can register new functions, for example a call to open a database could read the tables in the database and define new externs for the tables in the database.

Calls cannot (yet) store arbitrary objects, and are limited to the Logical datatypes.

It is an error to `Set()` a value that is marked as `In`. It is an error to call `YieldResult` without calling `Set()` on all `Out` parameters.

# The standard library



# Reading and writing data

There are many different ways to read and write data from Logical. The most basic way is to define the data itself in Logical syntax, by asserting facts. Facts files can be generated by another script and read by Logical. The `import` predicate can read the generated fact file. The `query` predicate can be used to print the results to the console, and the `find` statement can be used for ad-hoc queries. But as part of a production system, you probably want to read and write datafiles in a specific format, or interface directly to a database.

The most general way to read and write data is to use external predicates, written in C++. This is actually not too complicated, and is very powerful and efficient. All of the other ways to read and write data are build on externs written in C++.

## MySQL interface

```
// Load the "mysql" module
load-module "mysql".

// Open a connection to the database
mysql:open @connection1, hostname "localhost", database "db1", username "admin", password "whoops".

mysql:exec @connection1, sql "DROP TABLE Person".

person p has name n, age a if
    mysql:read @connection1, table "Person", row r, column "id", value p and
    mysql:read @connection1, table "Person", row r, column "name", value n and
    mysql:read @connection1, table "Person", row r, column "Age", value a.

person p has id id, name n, age a if
    mysql:Person p has ID id, Name n, Age a.

// Writing data
mysql:write:Person @connection1 has Name n, Age a if
    person _ has name n, age a.

mysql:close @connection1.
```

## Reading CSV files

```
// Open a CSV file for reading
read-csv @csv1, filename "fubar.csv".

// Get the data
@csv1 has line L, column C, value V.

new person has name n, age a if
    csv-file "file1.csv" has line p, column 3, value n and
    csv-file "file1.csv" has line p, column 5, value a.

string s has intvalue i.
string s has doublevalue d.
```

## Writing CSV files

csv:write "

## Externs

## Reading XML

## Writing XML

# Grammar

## Lexical analysis