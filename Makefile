# Thin wrapper: every target runs inside the Docker toolchain image.
#   make image                 build the image (once)
#   make shell                 interactive shell in the container
#   make build [PRESET=asan]   configure + build all CMake examples
#   make test  [PRESET=asan]   build, then run CMake tests
#   make run EX=cxx_move       build and run one CMake example
#   make meson-build|meson-test|meson-run  use Meson/Ninja instead
#   make clean                 remove build/

IMAGE  ?= cpp-playground:latest
PRESET ?= debug
EX     ?=
MESON_BUILD_DIR ?= build/meson/$(PRESET)

BUILD = cmake --preset $(PRESET) && cmake --build --preset $(PRESET)

MESON_ARGS_debug = --buildtype debug
MESON_ARGS_release = --buildtype release -Doptimization=2
MESON_ARGS_asan = --buildtype debug -Db_sanitize=address,undefined
MESON_ARGS_clang = --buildtype debug
MESON_COMPILER_clang = CC=clang CXX=clang++
MESON_SETUP = $(MESON_COMPILER_$(PRESET)) meson setup $(MESON_ARGS_$(PRESET)) $(MESON_BUILD_DIR)
MESON_CONFIGURE = if test -f $(MESON_BUILD_DIR)/meson-private/coredata.dat; then $(MESON_COMPILER_$(PRESET)) meson setup --reconfigure $(MESON_ARGS_$(PRESET)) $(MESON_BUILD_DIR); else $(MESON_SETUP); fi

TTY := $(shell [ -t 0 ] && echo -it)
DOCKER_RUN = docker run --rm $(TTY) \
	-v "$(CURDIR)":/work -w /work \
	--user $(shell id -u):$(shell id -g) -e HOME=/tmp \
	--cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
	$(IMAGE)

.PHONY: image shell build test run meson-build meson-test meson-run clean

image:
	docker build -t $(IMAGE) docker

shell:
	$(DOCKER_RUN) bash

build:
	$(DOCKER_RUN) bash -c '$(BUILD)'

test:
	$(DOCKER_RUN) bash -c '$(BUILD) && ctest --preset $(PRESET)'

run:
	@test -n "$(EX)" || { echo "usage: make run EX=<target name>"; exit 1; }
	$(DOCKER_RUN) bash -c 'cmake --preset $(PRESET) >/dev/null && cmake --build --preset $(PRESET) --target $(EX) && cd "$$(dirname "$$(find build/$(PRESET) -type f -name $(EX) -perm -u+x | head -1)")" && ./$(EX)'

meson-build:
	$(DOCKER_RUN) bash -c '$(MESON_CONFIGURE) && meson compile -C $(MESON_BUILD_DIR)'

meson-test:
	$(DOCKER_RUN) bash -c '$(MESON_CONFIGURE) && meson compile -C $(MESON_BUILD_DIR) && meson test -C $(MESON_BUILD_DIR) --print-errorlogs'

meson-run:
	@test -n "$(EX)" || { echo "usage: make meson-run EX=<target name>"; exit 1; }
	$(DOCKER_RUN) bash -c '$(MESON_CONFIGURE) && meson compile -C $(MESON_BUILD_DIR) $(EX) && binary=$$(find $(MESON_BUILD_DIR) -type f -name "$(EX)" -perm -u+x -print -quit) && test -n "$$binary" && "$$binary"'

clean:
	rm -rf build
