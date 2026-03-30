CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

SRC = main.cpp $(shell find server connection protocol command store persistence -name "*.cpp")
OUT = runredis

all:
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)