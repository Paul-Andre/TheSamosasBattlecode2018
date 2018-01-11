Battlecode 2018
===============
This is our submission to MIT's 2018 [Battlecode](http://battlecode.org/) programming
competition.

Building
--------

```bash
mkdir build
cd build
cmake ..
make
```

Running
-------
After building simply run:

```bash
./build/earth-agent
```

and

```bash
./build/mars-agent
```

Linting
-------
We use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to format all of
our C++ code. You must auto-format your code before pushing using the `format.sh`
script as follows:

```bash
./format.sh
```

For more options, simply run:

```bash
./format.sh --help
```
