CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -pedantic -Iinclude
TARGET = sheet
LDFLAGS = -lm

SRC = src/main.c src/spreadsheet.c src/cell.c src/input_parser.c src/scrolling.c src/avl_tree.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)  

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
