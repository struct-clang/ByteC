CC := clang
CFLAGS := -Wall -Wextra -O2
LDFLAGS := $(shell)
SRC_DIR := src
BUILD_DIR := build

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.bpl,$(SRC_FILES))

all: $(OBJ_FILES)

$(BUILD_DIR)/%.bpl: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -dynamiclib $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
