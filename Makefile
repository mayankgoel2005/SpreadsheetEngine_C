CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -pedantic -Iinclude
TARGET = target/release/spreadsheet  # Correct binary name & path
TEST_TARGET = test_sheet
LDFLAGS = -lm

SRC = src/main.c src/spreadsheet.c src/cell.c src/input_parser.c src/scrolling.c src/avl_tree.c
OBJ = $(SRC:.c=.o)

TEST_SRC = src/main-testcases.c src/spreadsheet.c src/cell.c src/input_parser.c src/scrolling.c src/avl_tree.c
TEST_OBJ = $(TEST_SRC:.c=.o)

all: $(TARGET)

test: $(TEST_TARGET)

# Ensure target directory exists before compiling
$(TARGET): $(OBJ)
	mkdir -p target/release   # Create required directories
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ) $(TARGET) $(TEST_OBJ) $(TEST_TARGET) target/release/


