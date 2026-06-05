CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
TARGET = program
SRC = main.c commands.c globals.c queue.c reader.c scanner.c stats.c watcher.c
BUILD_DIR = build
OBJ = $(SRC:%.c=$(BUILD_DIR)/%.o)
LEGACY_OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(TARGET) $(BUILD_DIR) $(LEGACY_OBJ)
