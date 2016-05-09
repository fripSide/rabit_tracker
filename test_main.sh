rm main
g++ main.cpp -o main

./main -n 2 -v 0 -r demo -u 10.11.53.64  test/basic.rabit 3

./main -n 2 test/basic.rabit 4 -v 0 -u 10.11.53.64