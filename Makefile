.PHONY: all configure test

all: configure

configure:
	cmake -S . -B build/runtime -DCMAKE_BUILD_TYPE=Release
	cmake --build build/runtime --parallel 2

test: configure
	ctest --test-dir build/runtime --output-on-failure
