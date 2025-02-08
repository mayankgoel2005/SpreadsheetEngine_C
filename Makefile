CC = gcc
CFLAGS = -Wall -g -Iinclude
TARGET = sheet

SRC = src/main.c src/spreadsheet.c src/cell.c src/input_parser.c src/scrolling.c src/simple_operations.c src/avl_tree.c 
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
