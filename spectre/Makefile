CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra

.PHONY: all clean

all: side_channel spectre_v1

side_channel: side_channel.cc
	$(CXX) $(CXXFLAGS) -O2 -o $@ $^

spectre_v1: spectre_v1.cc
	$(CXX) $(CXXFLAGS) -O2 -o $@ $^

clean:
	rm -f side_channel spectre_v1
