rm main
g++ main.cpp -o main

./main -n 2 -v 0 -r mpi -u 192.168.57.1  test/basic.rabit 3

./main -n 2 test/basic.rabit 4 -v 0 -u 192.168.57.1