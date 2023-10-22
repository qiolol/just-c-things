See the header `Vec.h` for information about the vector and its API.

See `example.c` for demo code that uses the vector.

# Compiling
```
make
```

will compile the vector code in both debug mode (with debug info) and release
mode (with optimizations) and put the artifacts in separate directories.

To compile just in debug mode, use:

```
make debug
```

Ditto with release mode:

```
make release
```

The executables include:

1. A program that runs the unit tests
2. A program that demos example usage of a vector

The vector object files generated include:

1. A "test" object file with assertions disabled (used by the unit test
executable to test whether the code does the right thing when there are no
assert crashes)
2. A normal object file with assertions enabled (causing vector code to crash
when serious invariants are violated)

# Testing

`tests.c` contains the unit tests. To compile and run them:

```
make debug && ./debug/tests.exe
```

# Benchmarking

The `benchmark/` subdirectory has a C++ program that benchmarks the vector
against a `std::vector`. To compile and run it:

```
cd benchmark/ ; make && ./benchmark.exe 2>/dev/null
```

(Muting stderr with `2>/dev/null` is recommended because the results would
otherwise be flooded by debugging output.)
