CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -g

.PHONY: test examples clean

# Unit tests of the C++ bridge, with the address and undefined-behavior sanitizers
test: build/test_slide_bridge
	./build/test_slide_bridge

build/test_slide_bridge: bridge/slide_bridge.cpp bridge/slide_bridge.hpp tests/test_slide_bridge.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined -I bridge bridge/slide_bridge.cpp tests/test_slide_bridge.cpp -o $@

clean:
	rm -rf build

# Web build, browser test and poster of each example (the Emscripten SDK active, and
# CGP_DIR=<a clone of https://github.com/drohmer/cgp-2027> for the CGP examples)
examples:
	python3 scripts/build_examples.py
