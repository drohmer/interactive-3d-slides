CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -g

.PHONY: test clean

# Unit tests of the C++ bridge, with the address and undefined-behavior sanitizers
test: build/test_slide_bridge
	./build/test_slide_bridge

build/test_slide_bridge: bridge/slide_bridge.cpp bridge/slide_bridge.hpp tests/test_slide_bridge.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined -I bridge bridge/slide_bridge.cpp tests/test_slide_bridge.cpp -o $@

clean:
	rm -rf build
