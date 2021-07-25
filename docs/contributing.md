# Directory layout

# Building the source code

The following commands will build the source code and test it.
```bash
mkdir build
cd build
cmake ../src
make
make test
```

## Building on MacOS

Prerequisites:
- Xcode (for MacOS build tools)
- Homebrew (for `flex`, `bison` and `boost`)
- CMake

```
brew install flex bison boost antlr antlr4-cpp-runtime mysql
cmake .. -DBISON_EXECUTABLE=/opt/homebrew/opt/bison/bin/bison -DFLEX_INCLUDE_DIR=/opt/homebrew/opt/flex/include -DFL_LIBRARY=/opt/homebrew/opt/flex/lib/libfl.a -DMYSQL_INCLUDE_DIR=/opt/homebrew/opt/mysql/include
make
make test
```

As you can see, the versions of `flex` and `bison` that ship with the MacOS build tools are out of date and you will need the latest version, that can be supplied using Homebrew.

The default CMake generator is to produce a Makefile, which works perfectly well.  Additional options to CMake include

- `-DCMAKE_BUILD_TYPE=Debug` for a debug build
- `-GXcode` for development in Xcode. Open the generated file `logical.xcodeproj` in Xcode.

# Running tests

In the build directory, run

```bash
make test
```

or run `ctest`.

# Adding tests

New tests are added by adding a `.dl` file to the relevant `tests` directory, and adding an entry for the test in `CMakeLists.txt`.

