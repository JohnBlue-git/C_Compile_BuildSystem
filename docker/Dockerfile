# Toolchain image for building and running every example in this repository.
#
# The base image is pinned by digest, so a rebuild starts from the same
# Ubuntu 24.04 snapshot. To move to a newer snapshot:
#   docker pull ubuntu:24.04
#   docker image inspect ubuntu:24.04 --format '{{index .RepoDigests 0}}'
FROM ubuntu:24.04@sha256:fe81560990fae86dc4743202b2cb4d3df98d0bcf949173cecef15ce1127ef852

ARG DEBIAN_FRONTEND=noninteractive

# Compilers:  gcc/g++ 13, clang/clang++ 18
# Build:      cmake 3.28, ninja, make
# Debug:      gdb, valgrind, strace, binutils (nm, objdump, readelf)
# Libraries:  Boost.JSON, nlohmann/json (used by CXX/examples/json)
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        build-essential \
        clang \
        lld \
        cmake \
        ninja-build \
        gdb \
        valgrind \
        strace \
        file \
        less \
        libboost-json-dev \
        nlohmann-json3-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /work
CMD ["bash"]
