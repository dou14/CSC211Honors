 g++ -o hello_mongoc main.cpp \
    -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0 \
    -lmongoc-1.0 -lbson-1.0 -std=c++17 -pthread -Wall
    