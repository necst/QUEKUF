# QUEKUF, an accurate Union Find Decoder for the Toric Code


[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.10869867.svg)](https://doi.org/10.5281/zenodo.10869867)



QUEKUF is an accurate Union Find Decoder designed for quantum error correction on the Toric Code.
Built and compiled with Vitis HLS 2023.1

Source files contained in the Design/ folder.
The Build/ folder contains prebuilt binaries for the decoder from D3 up to D8 aswell as various datasets.

# Build process
To build the hardware and host, clone the repository:

`git clone https://github.com/necst/QUEKUF.git`

Comment out the line in  `Design/Controller.cpp` that says:

`#define CBUILD`

This line is used when compiling the sw_emu.

Then navigate to the cloned folder and run:

`make all TARGET=hw PLATFORM=<target_platform> FREQ_MHZ=<frequency>`

To choose the code length just modify the line in `Design/Defines.h` and `HostCode/host.h` that says:

`#define D <code_length>`

To whichever code lenght you prefer.
