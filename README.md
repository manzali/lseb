[![Build Status](https://travis-ci.org/manzali/lseb.svg?branch=master)](https://travis-ci.org/manzali/lseb)
[![Coverage Status](https://coveralls.io/repos/manzali/lseb/badge.svg?branch=master&service=github)](https://coveralls.io/github/manzali/lseb?branch=master)

# LSEB (Large Scale Event Building)

LSEB is a DAQ (data acquisition system) software that aims to perform the event building of the LHCb physic experiment after its next major upgrade (2018-2019). This software is designed to scale up to hundreds of nodes and to use different transport layers (at the moment only TCP (using libfabric) and Infiniband verbs are implemented).

## Generic Design

Each node of the system receives data from a part of the detector, collecting just a fragment of each event. Each node can also build an event receiving fragments from the other nodes of the system.

This software is mainly composed by three components:
* **Controller** - The controller takes care of collecting fragments from a generator that simulate the acquisition from the detector.
* **Readout Unit** - Once that enough fragments have been collected, the Readout Unit sends them to a specific node.
* **Builder Unit** - The Builder Unit receives from the other nodes the fragments of a specific event, checking the correctness.

LSEB runs as a single process in each node and spawn two threads: one for the ReadUnit and one for the Builder Unit.

## Install

First of all you need to install the boost libraries (at least with system and program_options components). You need also the infiniband libraries (available installing the OFED Package) if you want to use infiniband as transport layer.

You also optionnaly need to install the hydra launcher fropm mpich (https://www.mpich.org/downloads/).

In order to install LSEB run:

```Bash
    git clone https://github.com/manzali/lseb.git
    cd lseb
    mkdir build
    cd build
    cmake -DTRANSPORT=<FI_TCP | FI_VERBS | VERBS> ..
    #or
    cmake -DTRANSPORT=<FI_TCP | FI_VERBS | VERBS> -DENABLE_HYDRA=ON -DWITH_HYDRA=<PATH_TO_HYDRA_PREFIX> ..
```

## Getting Started

You can start from configuration.json in the root directory in order to create your own configuration file.

```JSON
    "<NODE_NAME1>":{"HOST": "<HOSTNAME1>", "PORT": "<PORT1>"},
    "<NODE_NAME2>":{"HOST": "<HOSTNAME2>", "PORT": "<PORT2>"}
```
 
Once that the configuration file is ready you can LSEB typing:

```Bash
    ./lseb -c configuration.json -i ID
```

## Running with Hydra

You can start from configuration.json in the root directory in order to create your own configuration file. Select the net interface you want to use. Setup an `hostfile` listing the hosts you want to run on.

Once that the configuration file is ready you can LSEB typing:

```Bash
    mpiexec --hostfile ./hosts -np 16 -ppn 1 ./lseb -c configuration.json
```

It says to use `./hosts` to get list of hosts to run on, `-ppn 1` force usage of only 1 process per node and `-np 16` launche 16 processes.

If you are running into slurm (http://slurm.schedmd.com/) or requivalent job scheduler you might use :

```Bash
    salloc -N 16 -n 16 --exclusive mpiexec ./lseb -c configuration.json
```

## License

Coming soon.
