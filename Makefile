CMAKE_DIR := build

.PHONY: setup lint format test build clean

setup:
	sudo apt update
	sudo apt install -y cmake clang-format clang-tidy build-essential cmake-format

build:
	cmake -S . -B $(CMAKE_DIR) -DXCONN_BUILD_TESTS=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(CMAKE_DIR) --parallel

format:
	cmake -S . -B $(CMAKE_DIR) -DCMAKE_BUILD_TYPE=XFormat -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(CMAKE_DIR) --target xconn_format --parallel
	cmake-format -i CMakeLists.txt

lint:
	cmake -S . -B $(CMAKE_DIR) -DCMAKE_BUILD_TYPE=XLint -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(CMAKE_DIR) --target xconn_lint --parallel
	cmake-lint CMakeLists.txt

test:
	cmake -S . -B $(CMAKE_DIR) -DXCONN_BUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(CMAKE_DIR) --parallel
	ctest --test-dir $(CMAKE_DIR) --output-on-failure -V

clean:
	rm -rf $(CMAKE_DIR)

run-xconn:
	git clone https://github.com/xconnio/xconn-aat-setup.git
	cd xconn-aat-setup/nxt && make run
