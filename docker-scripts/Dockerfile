FROM ubuntu:latest

# Update and install software
RUN apt-get update
RUN apt-get -y install build-essential\
                       cmake\
                       git\
                       libboost-all-dev

# Get lseb and compile it
RUN git clone https://github.com/manzali/lseb.git;\
    mkdir lseb/build;\
    cd lseb/build;\
    cmake -DCMAKE_BUILD_TYPE=Release -DTRANSPORT=TCP ..;\
    make -j

