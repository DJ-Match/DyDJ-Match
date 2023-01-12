# DyDJ Match

**DyDJ Match** implements a number of different algorithms to solve the **k-Disjoint Matchings**
problem in the fully dynamic setting: Maintain a set of k pairwise edge-disjoint matchings
such that the total weight of the edges contained in one of the k matchings is maximized
as the input graph undergoes a series of edge weights updates.
This problem appears in the context of reconfigurable optical technologies for
data centers.

This repository contains the source code to our INFOCOM'23 paper
**Dynamic Demand-Aware Link Scheduling for Reconfigurable Datacenters**.


## Building

This program is written in C++17 and built on top of the modular algorithm framework
[Algora](https://libalgora.gitlab.io).
The build process uses CMake (at least 3.9).

On Debian/Ubuntu, all dependencies can be installed by running: `# apt install
cmake libboost-dev`.
On Fedora, run `# dnf install cmake boost-devel`.
On FreeBSD, dependencies can be installed with  `# pkg install cmake boost-libs`.

Before you can compile **DyDJ Match**, you need to obtain and compile the Algora
libraries
[Algora|Core](https://gitlab.com/libalgora/AlgoraCore), version 1.3 or greater,
and
[Algora|Dyn](https://gitlab.com/libalgora/AlgoraDyn), version 1.1.1 or greater.
The build configuration expects to find them in `Algora/AlgoraCore` and `Algora/AlgoraDyn`.
You can simply run `$ ./downloadAndCompileAlgora` to clone and compile both libraries.
Alternatively, you can put these libraries also elsewhere and manually
create the corresponding symlinks, for example.
Do not forget to compile Algora|Core and Algora|Dyn before you try to build
**DyDJ Match**!
The respective READMEs provide for further instructions.

**DyDJ Match** comes with an
`easyCompile` script that creates the necessary build directories and
compiles the app on Linux.
If you have compiled **Algora|Core** and **Algora|Dyn**, all you need to do
now is to call ` ./easyCompile`.
The compiled binaries can then be found in the `build/Debug` and `build/Release`
subdirectories.

Alternatively, run
```
mkdir build
cd build
cmake ..
make
```
To specify custom locations for **Algora|Core** and **Algora|Dyn** define the variables `ALGORA_CORE_PATH` and `ALGORA_DYN_PATH` when running CMake.

## Running

**DyDJ Match** needs a graph file as command line argument and reads a configuration file from standard input:
```
$ DyDjMatch input-file < configfile
```

The expected format of the input files is one line per edge, where each line has the format
```
<node id> <node id> <weight/demand> <timestamp>
```
Two example input files can be found in `examples/examples.zip`.

For information on the structure of the configuration file,
see [the configuration documentation](docs/Configuration.md). The configuration can also be specified interactively when calling `DyDjMatch input-file`.



## External Projects

Besides Algora, we use or adapted code from the following projects:
- [DJ Match](https://github.com/DJ-Match/DJ-Match)
- [argtable3](https://github.com/argtable/argtable3), shipped in `src/extern/`
- [Boost 1.74](https://www.boost.org/users/history/version_1_74_0.html)

## License

**Algora** and **DyDJ Match** are free software and licensed under the GNU General Public License
version 3.  See also the file `COPYING`.

The licenses of files (derived) from external projects (DJ Match, argtable3, Boost) may deviate and
can be found in the respective head sections.

If you publish results using our algorithms, please acknowledge our work by
citing the corresponding paper:

```
@inproceedings{dydjmatch,
  author    = {Hanauer, Kathrin and Henzinger, Monika and Ost, Lara and Schmid, Stefan},
  title     = {Dynamic Demand-Aware Link Scheduling for Reconfigurable Datacenters},
  booktitle = {{IEEE} Conference on Computer Communications, {INFOCOM} 2023, May 17-20, 2023},
  publisher = {{IEEE}},
  year      = {2023},
  note      = {to appear}
}
```

## Code Contributors (in alphabetic order by last name)

- Kathrin Hanauer
- Lara Ost
