CC  = g++ -pthread
CXX = g++ -pthread

INCLUDES = 
CFLAGS   = -std=c++17 -g -Wall $(INCLUDES)
CXXFLAGS = -std=c++17 -g -Wall $(INCLUDES)
default: main
main: main.o 
# header dependency
main.o: main.cpp 