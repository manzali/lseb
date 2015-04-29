# COMPILE AND RUN #

Create a build directory and build from it:

mkdir build

cd build

#transport can be TCP,VERBS,RSOCKETS
cmake -DTRANSPORT=<trasport> -DCMAKE_BUILD_TYPE=Debug ..

From build directory run the ru executable:

./ru/ru ../test/configuration.ini
