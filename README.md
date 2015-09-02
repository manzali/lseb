[![Build Status](https://travis-ci.org/manzali/lseb.svg?branch=master)](https://travis-ci.org/manzali/lseb)
[![Coverage Status](https://coveralls.io/repos/manzali/lseb/badge.svg?branch=master&service=github)](https://coveralls.io/github/manzali/lseb?branch=master)

# LSEB (Large Scale Event Building)

LSEB is a DAQ (data acquisition system) software that aims to perform the event building of the LHCb physic experiment after its next major upgrade (2018-2019). This software is designed to scale up to hundreds of nodes and to use different transport layers (at the moment only TCP and Infiniband verbs are implemented).

## Generic Design

Each node of the system receives data from a part of the detector, collecting just a fragment of each event. Each node can also build an event receiving fragments from the other nodes of the system.

This software is mainly composed by three components:
* **Controller** - The controller takes care of collecting fragments from a generator that simulate the acquisition from the detector.
* **Readout Unit** - Once that enough fragments have been collected, the Readout Unit sends them to a specific node.
* **Builder Unit** - The Builder Unit receives from the other nodes the fragments of a specific event, checking the correctness.

LSEB runs as a single process in each node and spawn two threads: one for the ReadUnit and one for the Builder Unit.

## Install

First of all you need to install the boost libraries (at least with system and program_options components). You need also the infiniband libraries (available installing the OFED Package) if you want to use infiniband as transport layer.

In order to install LSEB run:

```Bash
    git clone https://github.com/manzali/lseb.git
    cd lseb
    mkdir build
    cd build
    cmake -DTRANSPORT=<TCP | VERBS> ..
```

## Getting Started

You can start from configuration.json in the root directory in order to create your own configuration file.

The placeholder \_\_ENDPOINTS\_\_ has to be replaced with the json describing the hostnames (or ips) and ports used, for example for a 2 nodes configuration:

```JSON
    {"HOST":"hostname1", "PORT":"7000"},
    {"HOST":"hostname2", "PORT":"7000"}
```

Once that the configuration file is ready you can LSEB typing:

```Bash
    ./lseb -c configuration.json -i ID
```
where ID is the id of the node where LSEB will be executed (using the 2 nodes configuration above, the id can be 0 or 1).

## License

Coming soon.
