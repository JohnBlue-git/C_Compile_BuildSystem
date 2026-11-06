# Thin wrapper: every target runs inside the Docker toolchain image.
#   make image                 build the image (once)
#   make shell                 interactive shell in the container
#   make build [PRESET=asan]   configure + build all examples
#   make test  [PRESET=asan]   build, then run every example as a test
#   make run EX=cxx_move       build and run one example
#   make clean                 remove build/

IMAGE  ?= cpp-playground:latest
PRESET ?= debug
EX     ?=

BUILD = cmake --preset $(PRESET) && cmake --build --preset $(PRESET)

TTY := $(shell [ -t 0 ] && echo -it)
DOCKER_RUN = docker run --rm $(TTY) \
	-v "$(CURDIR)":/work -w /work \
	--user $(shell id -u):$(shell id -g) -e HOME=/tmp \
	--cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
	$(IMAGE)

.PHONY: image shell build test run clean

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

clean:
	rm -rf build
