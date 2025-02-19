CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -pedantic -Iinclude -I/opt/homebrew/opt/libxlsxwriter/include
TARGET = sheet
TEST_TARGET = test_sheet
LDFLAGS = -L/opt/homebrew/opt/libxlsxwriter/lib -lxlsxwriter -lm

SRC = src/main.c src/spreadsheet.c src/cell.c src/input_parser.c src/scrolling.c src/avl_tree.c
OBJ = $(SRC:.c=.o)

TEST_SRC = src/main-testcases.c src/spreadsheet.c src/cell.c src/input_parser.c src/scrolling.c src/avl_tree.c
TEST_OBJ = $(TEST_SRC:.c=.o)

all: $(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET) 999 16384

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_OBJ) $(TEST_TARGET)
