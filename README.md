# Maximum-k-Cut Preprocessing
TODO: Make this a nicer ReadMe

## External Dependencies
### GUROBI
The code relies on Gurobi Optimizer being installed. For a tutorial to install Gurobi see e.g. https://ca.cs.uni-bonn.de/doku.php?id=tutorial:gurobi-install

### Further Dependencies
- The project uses GoogleTest and NetworKit via git submodules. To initialise them call ```git submodule update --init --recursive```.
- Some of the python scripts also have dependencies. Create a virtual enviroment and run ```pip install -r requirements.txt``` to install the python requirements.

### Licensing
- This project uses cxxopts by Jarryd Beck licensed under the MIT licence. See ```include/cxxopts.hpp```
- 

### Installing instances
The instances can be installed by running
```bash
python3 scripts/installInstances.py
```

## Building
The project is build via CMake with 
```bash
cmake -B build -D CMAKE_BUILD_TYPE=<Debug/Release>
cmake --build build -j 8
```
We also have a compile scripts that you can call using
```bash
./scripts/compile_debug # builds in build_debug
```
and
```bash
./scripts/compile_release # builds in build
```

## Running the benchmark
### Stack problems on large instances
The code to compute SPQR-trees does a lot of recursive calls, which on large instances may lead to the stack being full. This causes a segfault.
To avoid this segfault we set the stack size to a larger value e.g. with 
```bash 
ulimit -s 32768
```

### Run benchmark executable once
```bash
./build/benchmark -i instances/instances_clean/ca-netscience.snap -o output/test.txt -c -d 1 -k 3
```

### Run full benchmark
The slurm folder contains slurm batch scripts that can be used for compiling and running different parts of the benchmark.
If you are not on a cluster using slurm you may have to modify them a bit, as they use the slurm array argument for seeds.

To collect all results once the benchmark is completed, call
```bash
python3 scripts/eval.py
```
This writes the results into experiments_[type].csv files in the experimental_results folder.

For plotting and generating tables run
```bash
python3 scripts/plot.py
```
Note that most of the table are just the raw table contends for LaTeX and lack the preamble.