# File format of dlc files

Logical supports a compiled format for its data and rules, which makes loading and running them a lot faster. Each file is self-contained and can be loaded independently.

## Numeric data types

Integers are stored in little-endian format. Signed integers are stored in twos-complement format. Integer operations operate on twos-complement integers that may silently overflow. Floating point numbers are stored in IEEE 754 format.

The basic numeric types are as follows:

 - `i8`
 - `u8`
 - `i16`
 - `u16`
 - `i32`
 - `i32`
 - `i64`
 - `u64`
 - `f32`
 - `f64`

## File structure

The file starts with 4 magic bytes, followed by 4-byte version number.

Following this is: the string table, the at-string table, data, rules and queries. The overall file format is as follows:

```
file ::= magic-bytes version-number stringsopt atstringsopt dataopt rulesopt queriesopt eof

magic-bytes ::= 0x78 0x32 0x9a 0x11

version-number ::= u32

eof ::= 0x00
```

## String table

The string table is used to map indexes to strings. Strings are used for relation names, and for string constants in queries and data.

The at-string table is similar to the string table, and is stored in the same way. Strings are stored without the enclosing quotes or a preceding `@`. Control codes like `\n` are stored as their ASCII values and are not escaped.

```
stringsopt ::= | 0x01 string-table

atstringsopt ::= | 0x02 string-table

string-table ::= number-of-strings stringseq

number-of-strings ::= u16

stringseq ::= string | stringseq string

string ::= 0x00 | string-char string

string-char ::= any byte except 0x00
```

## Entity format

A *entity* is an item of data that can be stored in a table or in a local variable. Recall that the basic data types of Logical are integers, strings, at-strings, Booleans and floating point. Strings and at-strings are considered to be different, thus `"a"` and `@a` are considered to be different. The empty string `""` and the empty at-string `@` are valid.

Strings and at-strings are stored as indexes into the string-table or at-string table. Strings are encoded in UTF-8. Strings are compared for equality based on byte sequence, so if there are alternative encodings for the same string, then they will be considered to be different for the purposes of rule evaluation. If identical strings appear in two different indexes in the string table, then they are considered to be identical.

Entites are encoded as a byte representing the entity type, followed by additional data depending on the entity type. Small integers are encoded as a single byte. If the first byte is in the range 0x00 - 0xdf, then the byte is interpreted as an integer value.

```
entity ::= integer | boolean | string | atstring | float | char | byte | none

integer ::= i8 (in the range 0x00 - 0xdf) | int16 | int32 | int64
int16 ::= 0xff i16
int32 ::= 0xfe i32
int64 ::= 0xfd i64

boolean: true | false
true ::= 0xfc
false ::= 0xfb

string ::= string16 | string32
string16 ::= 0xfa u16
string32 ::= 0xf9 u32

atstring ::= atstring16 | atstring32
atstring16 ::= 0xf8 u16
atstring32 ::= 0xf7 u32

float ::= float32 | float64
float32 ::= 0xf6 f32
float64 ::= 0xf5 f64

char ::= char8 | char16 | char32
char8 ::= 0xf4 u8
char16 ::= 0xf3 u16
char32 ::= 0xf2 u32

byte ::= 0xf1 u8

none ::= 0xf0
```

Note that the `byte`, `char` and `none` types are not supported by the Logical language, and are reserved for future use.

## Data tables

Relation names are stored in the string table, and referenced using an index into the string table.

```
dataopt ::= | data

data ::= tabledata | data tabledata

tabledata ::= 0x03 relation arity rowcount entities

// The relation name is an index into the string table.
relation ::= u32

arity ::= u8

rowcount ::= u32

entities ::= entity | entities entity
```

## Rules

```
rulesopt ::= | rules
rules ::= rule | rules rule

rule ::= 0x04 locals instructions end

// The number of local variables
locals ::= u8

end ::= 0x10

instructions ::= instruction | instructions instruction

instruction ::=
    assign 
|   load
|   read
|   write
|   or
|   deduplicate
|   aggregate
|   unary
|   binary
|   check
|   label
|   join

load ::= 0x20 variable entity

variable ::= u8
variable{*n} ::= variable ... variable (repeated n times)

read ::= scan | join
scan ::= 0x21 relation arity{=n} variable{*n}
join ::= 0x22 relation arity{=n} variable{*n} variable{*n}
write ::= 0x23 relation arity{=n} variable{*n}
or ::= 0x24
deduplicate ::= 0x25 u8{=n} variable{*n}
aggregate ::= count | sum | min | max | not
count ::= 0x25 variable
sum ::= 0x26 variable variable
max ::= 0x27 variable variable
not ::= 0x28 variable
check ::= 0x29 variable
min ::= 0x2a variable variable
label ::= 0x2b u8
join ::= 0x2c u8

unary ::= unaryop variable variable
binary ::= binaryop variable variable variable

unaryop ::= u8
binaryop ::= u8
```

## Instructions

Notation:
- `b` suffix indicates that the operation requires a bound (assigned) variable.
- `f` indicates that the operation requires an unbound (unassigned) variable.
- `B` a set of bound (assigned) variables
- `F` a set of unbound (unassigned) variables (that become assigned by the operation)

File sections:
- `strings`
- `atstrings`
- `data`
- `rules`
- `queries`

Relations:

- `read.F` - Reads values from a predicate.
- `write.B` - Writes the values into a predicate.
- `join.BF` - Queries a predicate, and assigns the results to the output variables.
- `exists.F` - Checks that the given values 

Comparisons:

- `eq.bb v1 v2` - Checks if `v1==v2`.
- `eq.bf v1 v2` - Assigns `v2 = v1`.
- `lt.bb v1 v2` - Checks `v1 < v2`.
- `neq.bb` - Checks that `v1 != v2`.
- `lteq.bb`
- `gt.bb`
- `gteq.bb`

Numerical computations:

- `add.bbb v1 v2 v3` - Checks if `v3=v1+v2`
- `add.bbf` - Assigns `v3=v1+v2`.
- `range.lt.lt.bbb v1 v2 v3`
- `range.lt.lteq.bbb v1 v2 v3`
- `range.lteq.lt.bbb v1 v2 v3`
- `range.lteq.lteq.bbb v1 v2 v3`
- `range.lt.lt.bbf v1 v2 v3`
- `range.lt.lteq.bbf v1 v2 v3`
- `range.lteq.lt.bbf v1 v2 v3`
- `range.lteq.lteq.bbf v1 v2 v3`

Aggregates:

- `sum.bf v1 v2` - Sums the values in `v1` and stores them in `v2`
- `strictsum.bf v1 v2`
- `count.f v1` - Counts the number of calls and stores the result in `v1`.
- `not.f v1` - Stores a valid value in `v1`.
- `strictcount.f`
- `max.bf`
- `min.bf`
- `check.b` - Checks that the variable has been assigned to. If not, fails.
- `dedupliate.B` - Ensures that the given set of variables is unique.

Control flow:
- `branch`
- `join`
- `label`

## Execution model

Instructions form a sequence, where each instruction can call the next instruction in the sequence 0, 1 or n times. Instructions communicate with each other through a set of local variables.

## Queries

```
queriesopt ::= | queries
queries ::= query | queries query

query ::= 0x05 locals{=n} column-name{*n} instructions end

column-name: entity
```

## Versioning

The version number of the file is unspecified, however files are always backwards compatible, so opcodes never change their meaning. This means for example, that if we decided that relations can have more than 255 columns, then we would need to add new op-codes for defining data, joining, or writing to the table.
