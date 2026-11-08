# Toolchain image for building and running every example in this repository.
#
# The base image is pinned by digest, and apt installs packages from a fixed
# Ubuntu archive snapshot (snapshot.ubuntu.com). A rebuild therefore gets the
# same OS layer and the same package versions. To move to newer versions:
#   docker pull ubuntu:24.04
#   docker image inspect ubuntu:24.04 --format '{{index .RepoDigests 0}}'
# then update the digest below and set UBUNTU_SNAPSHOT to a newer date.
FROM ubuntu:24.04@sha256:fe81560990fae86dc4743202b2cb4d3df98d0bcf949173cecef15ce1127ef852

ARG DEBIAN_FRONTEND=noninteractive
ARG UBUNTU_SNAPSHOT=20261102T000000Z

# Compilers:  gcc/g++ 13, clang/clang++ 18
# Build:      cmake 3.28, meson, ninja, make
# Debug:      gdb, valgrind, strace, binutils (nm, objdump, readelf)
# Libraries:  Boost.JSON, nlohmann/json (used by CXX/examples/library/json)
# snapshot.ubuntu.com is HTTPS only, and the base image has no CA certificates.
# Install them from the normal archive first, then switch apt to the snapshot.
RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates \
 && echo "APT::Snapshot \"${UBUNTU_SNAPSHOT}\";" > /etc/apt/apt.conf.d/50snapshot \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
        build-essential \
        clang \
        lld \
        cmake \
        meson \
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
