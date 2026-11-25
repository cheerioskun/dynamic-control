FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential ca-certificates cmake git lld ninja-build python3 \
    zlib1g-dev libzstd-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/dynamic-control

# Cache the compiler build independently of benchmark and documentation edits.
COPY toolchain.json ./
COPY patches/llvm/ patches/llvm/
COPY scripts/llvm.py scripts/build-llvm.sh scripts/
ARG DC_BUILD_JOBS=2
RUN DC_BUILD_JOBS=${DC_BUILD_JOBS} ./scripts/build-llvm.sh -DLLVM_USE_LINKER=lld

COPY . .
ENV PATH="/opt/dynamic-control/build/llvm/bin:${PATH}"
RUN make test test-compiler bench
CMD ["/bin/bash"]
