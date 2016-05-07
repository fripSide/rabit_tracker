rm main
g++ main.cpp -o main

./main -n 2 -v 1 -u 192.168.54.1  test/basic.rabit 3

./main -n 2 test/basic.rabit 2 -v 0 -u 192.168.54.1