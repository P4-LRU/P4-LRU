# CPU Simulation Code for P4LRU

This directory is for P4LRU CPU simulation. You can run this code to reproduce the CPU-simulation results in the paper.



## Prerequisite

A linux server with `clang`(at least version 10) environment.

## Build

Type `make` in the shell to compile the program.

## Run

The program requires a datafile with the path `dataset\data.dat`. `data.dat` is a binary file generated from CAIDA_2018, which use 23 bytes representing a packet. 23-byte unit can be shown in C style:

```c
struct packet_info{
    uint32_t srcip;
    uint32_t destip;
    uint16_t srcport;
    uint16_t destport;
    u_char protocal;
    int size;
    double timestamp;
};
```



Once compiled, the executable program will be located within the `exec` subdirectory.

The full test is segmented into six components, designated as App1 through App6, which can be found in `src/experiment.cpp`.

- Apps 1, 2, and 3 correspond respectively to Figures 15, 16, and 17 in the paper.
- Apps 4, 5, and 6 correlate respectively to Figures 12, 13, and 14 in the paper.

Running the corresponding part can generate the results shown in the paper.

