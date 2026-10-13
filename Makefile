SHELL := /bin/bash
.DEFAULT_GOAL := help

BUILD_DIR  ?= build
BUILD_TYPE ?= Release
PREFIX     ?= /usr/local
CMAKE      ?= cmake
JOBS       ?= $(shell nproc 2>/dev/null || echo 4)

.PHONY: help configure build debug asan coverage install uninstall \
        format format-check lint tidy clean distclean docker run run-json

help: ## Show this help
	@grep -hE '^[a-zA-Z_-]+:.*?## ' $(MAKEFILE_LIST) \
		| awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-14s\033[0m %s\n", $$1, $$2}'

configure: ## Configure the release build tree
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX)

build: configure ## Build (parallel)
	$(CMAKE) --build $(BUILD_DIR) -j $(JOBS)

debug: ## Configure + build a Debug tree
	$(CMAKE) -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
	$(CMAKE) --build build/debug -j $(JOBS)

asan: ## Configure + build with ASan/UBSan
	$(CMAKE) -S . -B build/asan -DCMAKE_BUILD_TYPE=Debug -DGRIDOT_ENABLE_SANITIZERS=ON
	$(CMAKE) --build build/asan -j $(JOBS)

coverage: ## Configure + build with coverage instrumentation
	$(CMAKE) -S . -B build/coverage -DCMAKE_BUILD_TYPE=Debug -DGRIDOT_ENABLE_COVERAGE=ON
	$(CMAKE) --build build/coverage -j $(JOBS)
	@$(CMAKE) --build build/coverage --target coverage || true

install: build ## Install to $(PREFIX)
	$(CMAKE) --install $(BUILD_DIR)

uninstall: ## Remove files installed by 'make install'
	@xargs -a $(BUILD_DIR)/install_manifest.txt rm -f 2>/dev/null || \
		echo "no install manifest found"

format: ## Apply clang-format / cmake-format
	@if command -v clang-format >/dev/null; then \
		find src \( -name '*.cpp' -o -name '*.h' \) | xargs -r clang-format -i; \
	else echo "clang-format not installed"; fi
	@if command -v cmake-format >/dev/null; then \
		cmake-format -i CMakeLists.txt cmake/*.cmake; \
	else echo "cmake-format not installed"; fi

format-check: ## Verify formatting without writing files
	@if command -v clang-format >/dev/null; then \
		find src \( -name '*.cpp' -o -name '*.h' \) | xargs -r clang-format --dry-run --Werror; \
	else echo "clang-format not installed"; fi
	@if command -v cmake-format >/dev/null; then \
		cmake-format --check CMakeLists.txt cmake/*.cmake; \
	else echo "cmake-format not installed"; fi

lint: configure ## Run clang-tidy over the sources (warnings are errors)
	@if ! command -v clang-tidy >/dev/null; then echo "clang-tidy not installed"; exit 0; fi; \
	for f in $$(find src -name '*.cpp'); do \
		clang-tidy "$$f" -p $(BUILD_DIR) --quiet --warnings-as-errors='*' || exit 1; \
	done

tidy: configure ## Run clang-tidy without promoting warnings to errors
	@if ! command -v clang-tidy >/dev/null; then echo "clang-tidy not installed"; exit 0; fi; \
	for f in $$(find src -name '*.cpp'); do \
		clang-tidy "$$f" -p $(BUILD_DIR) --quiet || exit 1; \
	done

run: build ## Run a scan from the build tree
	$(BUILD_DIR)/gridot scan

run-json: build ## Run a scan and emit JSON
	$(BUILD_DIR)/gridot scan --json

docker: ## Build the container image
	docker build -t gridot:latest .

clean: ## Remove build artifacts from the current tree
	$(CMAKE) --build $(BUILD_DIR) --target clean || true

distclean: ## Remove all build trees
	rm -rf build
