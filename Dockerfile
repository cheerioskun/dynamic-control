FROM ubuntu:24.04 AS microbench

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

# Optional x86-64 image; the named context keeps the host recipe in its own repo.
FROM microbench AS shenango
COPY --from=cloudlab setup.sh compatibility.patch /opt/shenango-recipe/
RUN apt-get update && apt-get install -y --no-install-recommends kmod procps iproute2 \
    && bash /opt/shenango-recipe/setup.sh --userspace-only \
    && git -C /local/shenango apply /opt/dynamic-control/results/netbench-shenango/netbench.patch \
    && make -C /local/shenango/breakwater/apps/netbench/src -f Makefile.parity \
        MODE=hardcoded DC_ROOT=/opt/dynamic-control DC_ARCH=x86-64-v2 -j16 \
    && make -C /local/shenango/breakwater/apps/netbench/src -f Makefile.parity \
        MODE=auto DC_ROOT=/opt/dynamic-control DC_ARCH=x86-64-v2 -j16 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=cloudlab node.py container.py /opt/shenango-recipe/
ENTRYPOINT ["python3", "/opt/shenango-recipe/container.py"]

# Keep the ordinary, multiarchitecture build as the default.
FROM microbench AS default
