# Viewing Class Documentations

If you are viewing the Doxygen-generated version of this page, you should see the "Classes" and "Files" tabs above which lead you to code documentations.

If you wish to compile the docs, run `doxygen Doxygen` in the `src` directory (where this README is normally located).

# Running the Routing Program

## macOS

A bundled binary is included with the assignment submission with the name `pinrouter.app`. It should be supported by macOS 14 and above (tested on macOS 14 and 15).

If compiling from source is desired, CMake and Qt5 are required. The simplest way to acquire `qt` seems to be from the [Homebrew package manager](https://brew.sh). Once you have brew installed, type:

```
brew install cmake qt
```

With the prerequisites installed, the rest of the steps are identical to the Ubuntu compilation steps starting from the `git clone` line which you can find below.

## Ubuntu

### Install and Run the Packaged Snap

Running the packaged Snap version of the program is possible if you're running on a modern Ubuntu host. A packaged Snap is included in the root directory of the assignment archive after extraction. In a terminal, change to the extracted root, `pinrouter_0.0.1_amd64.snap` should be present. Install the snap:
```
sudo snap install --devmode pinrouter_0.0.1_amd64.snap
```

Invoke the GUI (add `--help` for additional command line options):
```
pinrouter
```

Uninstall the snap:
```
sudo snap remove pinrouter
```


### Compile from Source

Note: these instructions should also work on a WSL installation of Ubuntu 18.04 LTS or 20.04 LTS on Windows 10, assuming that you have a viable X11 server such as Xming.

Install prerequisites:
```
sudo apt install make gcc g++ qtchooser qt5-default libqt5svg5-dev qttools5-dev qttools5-dev-tools
```

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
