CC = gcc
CFLAGS = -g -O2 -Wall -Wextra -pedantic -Iinclude -I/opt/homebrew/opt/libxlsxwriter/include
TARGET = ./target/release/spreadsheet
#TEST_TARGET = test_sheet
#LDFLAGS = -L/opt/homebrew/opt/libxlsxwriter/lib -lxlsxwriter -lm
REPORT_SRC = report.tex
REPORT_PDF = report.pdf
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

report: $(REPORT_PDF)
$(REPORT_PDF): $(REPORT_SRC)
	@pdflatex $(REPORT_SRC)
	@pdflatex $(REPORT_SRC) # Run twice for proper cross-referencing
	@rm -f *.aux *.log *.out


clean:
	rm -f $(OBJ) $(TARGET) $(REPORT_PDF) *.aux *.log *.out
	#rm -f $(OBJ) $(TARGET) $(TEST_OBJ) $(TEST_TARGET)

