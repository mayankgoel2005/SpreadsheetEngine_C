CC = gcc
CFLAGS = -g -O2 -Wall -Wextra -pedantic -Iinclude -I/opt/homebrew/opt/libxlsxwriter/include
TARGET = ./target/release/spreadsheet
#TEST_TARGET = test_sheet
#LDFLAGS = -L/opt/homebrew/opt/libxlsxwriter/lib -lxlsxwriter -lm

SRC = src/main.c src/spreadsheet.c src/cell.c src/input_parser.c src/scrolling.c src/avl_tree.c
OBJ = $(SRC:.c=.o)

#TEST_SRC = src/main-testcases.c src/spreadsheet.c src/cell.c src/input_parser.c src/scrolling.c src/avl_tree.c
#TEST_OBJ = $(TEST_SRC:.c=.o)

all: $(TARGET)

#test: $(TEST_TARGET)
#	./$(TEST_TARGET) 999 16384

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)
	cp $(TARGET) ./sheet

#$(TEST_TARGET): $(TEST_OBJ)
#	$(CC) $(CFLAGS) -o $@ $(TEST_OBJ) $(LDFLAGS)
#	cp $(TEST_TARGET) ./sheet_test

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
	#rm -f $(OBJ) $(TARGET) $(TEST_OBJ) $(TEST_TARGET)

# Added report target to compile LaTeX file and display the report
report: report.pdf
	# On macOS, use 'open'; on Linux, you might use 'xdg-open'
	open report.pdf

report.pdf: report.tex
	pdflatex report.tex
	# Running pdflatex a second time for proper reference resolution (optional)
	pdflatex report.tex
