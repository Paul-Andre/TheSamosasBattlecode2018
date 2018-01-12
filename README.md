Battlecode 2018
===============
This is our submission to MIT's 2018 [Battlecode](http://battlecode.org/)
programming competition.

Running
-------
Simply clone this repository into the
[bc18-scaffold](https://github.com/battlecode/bc18-scaffold) repository, and
run the `scaffold`'s `run.sh` or `run_nodocker.sh`.

*Note*: You should `make clean` when switching between the docker and
docker-less environments.

Linting
-------
We use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to format
all of our C++ code. You must auto-format your code before pushing using the
`format.sh` script as follows:

```bash
./format.sh
```

For more options, simply run:

```bash
./format.sh --help
```
