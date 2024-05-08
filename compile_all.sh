mkdir -p build
g++ test/test.cc -o build/test
g++ example/example1.cc -o build/example1
g++ example/example2.cc -o build/example2
g++ example/example_cpp11.cc -o build/example_cpp11 -std=c++11
