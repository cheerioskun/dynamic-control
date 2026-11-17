.PHONY: all configure test setup compiler check-patches test-tooling test-compiler bench

all: configure

configure:
	cmake -S . -B build/runtime -DCMAKE_BUILD_TYPE=Release
	cmake --build build/runtime --parallel 2

test: configure
	ctest --test-dir build/runtime --output-on-failure

setup:
	python3 scripts/llvm.py setup

compiler:
	./scripts/build-llvm.sh

check-patches:
	python3 scripts/llvm.py check

test-tooling:
	python3 -m unittest discover -s tests/tooling -v

test-compiler:
	python3 tests/compiler.py

bench:
	cmake -S . -B build/bench -G Ninja -DCMAKE_BUILD_TYPE=Release \
		-DDC_BUILD_BENCH=ON -DBUILD_TESTING=OFF
	cmake --build build/bench --parallel 2
	python3 bench/run.py
