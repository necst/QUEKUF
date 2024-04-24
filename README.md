# QUEKUF - An accurate Union Find Decoder for the Toric Code

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.10869867.svg)](https://doi.org/10.5281/zenodo.10869867)

This repository containt the source code of the paper "An accurate Union Find Decoder for the Toric Code". 
In this paper, we present QUEKUF, an FPGA-based Union Find decoder designed for quantum error correction on the toric code. 
Our architecture attains up to 20.14X improvement in execution time over a C++ implementation while keeping a high accuracy, similar to the theoretical accuracy achieved by the Union Find algorithm (around 70%). 

## Structure of the Repository 

* `Build` contains prebuilt binaries ofr different code distance configurations;
* `Design` contains source files for our design;
* `HostCode` contains source files for the host application;
* `Scripts` contains scripts for dataset generation and utilities;
* `Testbench` contains source file for the Vitis HLS testbench.

## Default Configurations

We provide already built configurations for QUEKUF. 
These binaries are contained in `Build` within the corresponding folder (e.g., binaries build for code distance D = 3 are stored in the `D3` folder).
Each folder contains a precompiled host, a bitstream and a dataset that can be used to reproduce the results of the paper. 

## Building QUEKUF

To build the hardware and host, clone the repository:
```
git clone https://github.com/necst/QUEKUF.git
```

Comment out the line in  `Design/Controller.cpp` that says:

`#define CBUILD`

This line is used when compiling the sw_emu.

Inside the repository, run:

```
make all TARGET=hw PLATFORM=<target_platform> FREQ_MHZ=<frequency>
```
where the `<target_platform>` is the target FPGA and the recommended `<frequency>` is 300 MHz.

To choose the code length just modify the line in `Design/Defines.h` and `HostCode/host.h` that says:

`#define D <code_length>`

To whichever code lenght you prefer.

## Running the Experiments

We provide prebuilt QUEKUF (host and bitstream) for code distances from 3 to 8. 
To run the experiments, within the `Build` folder, enter the folder for the desired code distance:
- `D3` for code distance D = 3;
- `D4` for code distance D = 4;
- `D5` for code distance D = 5;
- `D6` for code distance D = 6;
- `D7` for code distance D = 7;
- `D8` for code distance D = 8.

Each folder already contains a dataset that can be used to reproduce the results.
Once in the desired folder, run:

```
./QUEKUF QUEKUF.xclbin Decoder_dataset.txt
```

## Experiment Customization 

We provide users with the possibility to run tests with custom datasets. 
The `Scripts/datasetGen.py` script will generate a dataset for a desired code length. This script can be run from terminal with:
```
python3 Scripts/datasetGen.py <Code_Length>
```
where `<Code_Length>` is the target code length. 
The script will generate a new dataset called `Decoder_dataset.txt` which can then be fed to the appropriate decoder with:
```
./Build/D<Code_Length>/QUEKUF Build/D<Code_Length>/QUEKUF.xclbin <path/to/custom/dataset>
```
NOTE: a dataset generated for a specific `<Code_Length>` must be fed to the corresponding decoder that supports the same code length

## Credits and Contributors 

Contributors: Federico Valentino, Beatrice Branchini, Davide Conficconi, Donatella Sciuto, Marco Domenico Santambrogio.

