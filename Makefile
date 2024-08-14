# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c99 -Iinclude $(shell pkg-config --cflags json-c)

# Linker flags
LDFLAGS = -lsqlite3 $(shell pkg-config --libs json-c)

# Source files
SRC = src/main.c src/logger.c src/database.c src/repo.c src/utils.c src/docker.c

# Object files
OBJ = $(SRC:.c=.o)

# Output executable
TARGET = dployer

# Default target
all: $(TARGET)

# Rule to build the target
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJ) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean run