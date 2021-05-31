# Synopsis of the Logical language

This article gives a condensed overview of the *Logical* language.

Logical is a logic programming language based on Datalog. It is largely a syntactic sugar for regular Datalog, with a natural syntax.

# Facts
Facts define the data to reason about.

```
// Defining a single unary predicate
"Fred" is a person.
3.14 is a number.

// Combined predicates define many predicates in the same statement
@Mickey is a large mouse.
@Mini is a small red mouse.
aggressive cat @Tom.
@Tom is an aggressive cat.

// Attributes define n-ary predicates
large dog @Pluto has name "Pluto", colour @brown.
colour @brown has red 168, green 109, blue 50.  // List of attributes (binary predicates)
colour @red, red 255, green 0, blue 0.  // Omitting the "has"
```
Entities are values that can be stored in predicates. They can be strings (`"Tom"`), at-strings (`@Tom`), integers (`123`), floats (`3.14`) or Boolean (`true`|`false`).

# Rules
There is no distinction between extensionals and intensionals in Logical. All predicates can have both facts and rules. Predicates with no facts or rules are empty.

Simple rules:
```
X is large if X is human.
large X if human X.
if X is human then X is large and X is an omnivore.
if X is a tiger then X is a large predator.

A has predator B if animal A is small and animal B is large.

A has predator B if 
    A is an animal and
    A is small and
    B is an animal and
    B is large.

B has food A if A has predator B.
A is a herbivore if A has food B and B is a plant.

A is a herbivore if A has food @salad or A has food @cabbage.
```

Negation and range:
```
A is a herbivore if A has food F and F is not meat.

P is unemployed if not P has job _ and P has age A and 18<=A and A<65.
P is unemployed if P has no job and P has age A and 18<=A<65.

// The range syntax is binding
prime N if 
    2<=N<=1000 and
    not (2<=M<N and N%M=0).

prime N if 
    2<=N<=1000 and
    all (N%M!=0) in 2<=M<N.
```

Count and sum:


Transitive closure:

```
x has reachable-function y if x has call-to y or.

x has reachable-function y if x reaches call-to y.
```

# Recursion


## Builtin predicates

```
print "Hello, world!".
```