# Compile from Source

Acquire the source code by:

```
git clone https://github.com/samuelngsh-testing/cpen513a1
```

From the project root, the `src` directory contains the source code (where this README is also located). To compile the project, run the following from the project root:

```
mkdir src/build && cd src/build
cmake .. && make
```

By default, unit tests are performed during the compilation with the results available via standard output. You should now be in the `src/build` directory relative to the project root. From there, invoke the GUI binary by:

```
./pinrouter
```

Command line options are also available, which you can view by:

```
./pinrouter --help
```
