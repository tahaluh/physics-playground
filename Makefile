.PHONY: configure build run clean rebuild

BUILD_DIR := build
TARGET := $(BUILD_DIR)/app

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) --parallel

run: build
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
rebuild:
	$(MAKE) clean
	$(MAKE) run
