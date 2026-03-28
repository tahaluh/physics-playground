.PHONY: configure build shaders run clean rebuild

BUILD_DIR := build
TARGET := $(BUILD_DIR)/app

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) --parallel

shaders: configure
	cmake --build $(BUILD_DIR) --target compile_shaders

run: build
	./$(TARGET)

clean:
	mkdir -p /tmp/phys-shaders-preserve
	if [ -d $(BUILD_DIR)/shaders ]; then cp -a $(BUILD_DIR)/shaders /tmp/phys-shaders-preserve/; fi
	rm -rf $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)
	if [ -d /tmp/phys-shaders-preserve/shaders ]; then mv /tmp/phys-shaders-preserve/shaders $(BUILD_DIR)/; fi
	rm -rf /tmp/phys-shaders-preserve
rebuild:
	$(MAKE) clean
	$(MAKE) run
