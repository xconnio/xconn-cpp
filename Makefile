CMAKE_DIR := build
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)

.PHONY: setup lint format test build clean

setup:
	sudo apt update
	sudo apt install -y cmake clang-format clang-tidy build-essential libmsgpack-dev libcjson-dev cmake-format libsodium-dev uthash-dev libmbedtls-dev libasio-dev

build:
	cmake -S . -B $(CMAKE_DIR) -DXCONN_BUILD_TESTS=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(CMAKE_DIR) -j$(NPROC)

format: build
	cmake --build $(CMAKE_DIR) --target xconn_format
	cmake-format -i CMakeLists.txt

lint: build
	cmake --build $(CMAKE_DIR) --target xconn_lint
	cmake-lint CMakeLists.txt

test:
	cmake -S . -B $(CMAKE_DIR) -DXCONN_BUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(CMAKE_DIR) -j$(NPROC)
	ctest --test-dir $(CMAKE_DIR) --output-on-failure -V

clean:
	rm -rf $(CMAKE_DIR)

run-xconn:
	git clone https://github.com/xconnio/xconn-aat-setup.git
	cd xconn-aat-setup/nxt && make run
