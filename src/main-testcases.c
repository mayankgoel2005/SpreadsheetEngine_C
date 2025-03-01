#include <stdio.h>
#include <stdlib.h>
#include "spreadsheet.h"
#include "input_parser.h"
#include <time.h>
#include <string.h>
#include <xlsxwriter.h>





#define MAX_INPUT_SIZE 100
#define MAX_COLUMN 16384  // Columns A to ZZZ
#define MAX_ROW 999       // Rows 1 to 999
#define MIN_VALUE -1000
#define MAX_VALUE 1000

int n = 20;  // Global variable for the total number of operations

// Function to get Excel-style column name (e.g., A, AB, ZZZ)
void get_column_letter(int col_num, char *col_letter) {
    int index = 0;
    while (col_num > 0) {
        col_num--;
        col_letter[index++] = 'A' + (col_num % 26);
        col_num /= 26;
    }
    col_letter[index] = '\0';

    // Reverse string for correct column name
    for (int i = 0, j = strlen(col_letter) - 1; i < j; i++, j--) {
        char temp = col_letter[i];
        col_letter[i] = col_letter[j];
        col_letter[j] = temp;
    }
}

// Function to convert Excel-style cell (A1) into row and column indexes
void get_cell_coordinates(const char *cell, int *row, int *col) {
    char col_str[10];
    int col_index = 0, row_index = 0;

    // Extract letters (column part)
    while (*cell >= 'A' && *cell <= 'Z') {
        col_str[col_index++] = *cell++;
    }
    col_str[col_index] = '\0';

    // Extract numbers (row part)
    sscanf(cell, "%d", &row_index);

    // Convert column name to number
    int col_num = 0;
    for (int i = 0; col_str[i] != '\0'; i++) {
        col_num = col_num * 26 + (col_str[i] - 'A' + 1);
    }

    *col = col_num - 1;  // Excel columns are 0-based in libxlsxwriter
    *row = row_index - 1; // Excel rows are 0-based in libxlsxwriter
}

// Function to generate a random Excel cell reference
void generate_random_cell(char *cell) {
    int column = rand() % MAX_COLUMN + 1;
    int row = rand() % MAX_ROW + 1;

    char col_letter[10];
    get_column_letter(column, col_letter);
    sprintf(cell, "%s%d", col_letter, row);
}

// // Ensure C3 is bottom-right of C2
int is_bottom_right(int row_c2, int col_c2, int row_c3, int col_c3) {
    return ((row_c3 >= row_c2 && col_c3 >= col_c2 && !(col_c3 == col_c2 && row_c3 == row_c2)));
}


// Function to generate and correctly place formulas in Excel
void generate_excel_formulas(void) {
    // Create an Excel workbook
    lxw_workbook *workbook = workbook_new("expected.xlsx");
    lxw_worksheet *worksheet = workbook_add_worksheet(workbook, NULL);

    // Open input.txt for writing
    FILE *file = fopen("input.txt", "w");
    if (!file) {
        perror("Error opening input.txt");
        return;
    }

    // Seed randomness
    srand(time(NULL));

    // **Initialize all cells (A1 to ZZZ999) to 0**
    for (int col = 0; col < MAX_COLUMN; col++) {
        for (int row = 0; row < MAX_ROW; row++) {
            worksheet_write_number(worksheet, row, col, 0, NULL);
        }
    }

    // **Generate `n` random operations**
    for (int i = 0; i < n; i++) {
        char c1[10], c2[10], c3[10];
        generate_random_cell(c1);

        int row, col;
        get_cell_coordinates(c1, &row, &col);

        int type = (rand() % 2) + 1;

        if (type == 1) {
            // Assign a random numeric value to the correct cell
            int value = (rand() % (MAX_VALUE - MIN_VALUE + 1)) + MIN_VALUE;
            worksheet_write_number(worksheet, row, col, value, NULL);
            fprintf(file, "%s=%d\n", c1, value);
        } else {
            // Generate valid random cells for formulas
               int row_c2, col_c2, row_c3, col_c3;
            do {
             
               generate_random_cell(c2); // Generate C2
                generate_random_cell(c3); // Generate C3
             
                get_cell_coordinates(c2, &row_c2, &col_c2);
                get_cell_coordinates(c3, &row_c3, &col_c3);
            } while (!is_bottom_right(row_c2, col_c2, row_c3, col_c3));

            int operation = (rand() % 10) + 1;
            char formula[50];
            char formula1[50];
            int sleep_time = (rand() % 3) + 1;
            
            switch (operation) {
                case 1: sprintf(formula, "=FLOOR(%s+%s, 1)", c2, c3); break;
                case 2: sprintf(formula, "=FLOOR(%s-%s, 1)", c2, c3); break;
                case 3: sprintf(formula, "=FLOOR(%s*%s, 1)", c2, c3); break;
                case 4: sprintf(formula, "=FLOOR(%s/%s, 1)", c2, c3); break;
                case 5: sprintf(formula, "=FLOOR(MIN(%s:%s), 1)", c2, c3); break;
                case 6: sprintf(formula, "=FLOOR(MAX(%s:%s), 1)", c2, c3); break;
                case 7: sprintf(formula, "=FLOOR(AVERAGE(%s:%s), 1)", c2, c3); break;
                case 8: sprintf(formula, "=FLOOR(SUM(%s:%s), 1)", c2, c3); break;
                case 9: sprintf(formula, "=FLOOR(STDEV(%s:%s), 1)", c2, c3); break;
                case 10: {
                    
                    sprintf(formula, "=%d", sleep_time); // Placeholder (Excel does not support sleep)
                    break;
                }
            }
            switch (operation) {
                case 1: sprintf(formula1, "=%s+%s", c2, c3); break;
                case 2: sprintf(formula1, "=%s-%s", c2, c3); break;
                case 3: sprintf(formula1, "=%s*%s", c2, c3); break;
                case 4: sprintf(formula1, "=%s/%s", c2, c3); break;
                case 5: sprintf(formula1, "=MIN(%s:%s)", c2, c3); break;
                case 6: sprintf(formula1, "=MAX(%s:%s)", c2, c3); break;
                case 7: sprintf(formula1, "=AVG(%s:%s)", c2, c3); break;
                case 8: sprintf(formula1, "=SUM(%s:%s)", c2, c3); break;
                case 9: sprintf(formula1, "=STDEV(%s:%s)", c2, c3); break;
                case 10: {

                    sprintf(formula1, "=SLEEP(%d)", sleep_time); 
                    break;
                }
            }

            // Write formula in correct Excel cell
            worksheet_write_formula(worksheet, row, col, formula, NULL);
            fprintf(file, "%s%s\n", c1, formula1);
        }
    }

    // Close workbook and input.txt
    workbook_close(workbook);
    fclose(file);
}




void exportSpreadsheetToExcel(Spreadsheet *spreadsheet, const char *filename) {
    // Create a new Excel workbook and worksheet
    lxw_workbook *workbook = workbook_new(filename);
    lxw_worksheet *worksheet = workbook_add_worksheet(workbook, NULL);

    // Iterate over spreadsheet rows and columns to write data into the worksheet
    for (int r = 0; r < spreadsheet->rows; r++) {
        for (int c = 0; c < spreadsheet->cols; c++) {
            // Assuming each cell contains a numeric or string value
            Cell *cell = &spreadsheet->table[r][c];
            worksheet_write_number(worksheet, r, c, cell->value, NULL);
        }
    }

    // Close the workbook (saves the file)
    workbook_close(workbook);
}


// Function to compare two Excel files by calling a Python script
int compareExcelFiles(const char *file1, const char *file2) {
    char command[512];
    snprintf(command, sizeof(command), "python3 compare_excel.py \"%s\" \"%s\"", file1, file2);
    int result = system(command);
    
    if (result == 0) {
        printf("Excel files are identical.\n");
        return 1;
    } else {
        printf("Excel files have differences.\n");
        return 0;
    }
}



int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <rows> <cols>\n", argv[0]);
        return 1;
    }
    generate_excel_formulas();
    printf("Excel file 'expected.xlsx' and text file 'input.txt' created successfully.\n");
    
    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);
    if (rows >= 1000 || rows <= 0) {
        printf("Error: Rows should be in the range 1 to 1000 inclusive\n");
        return 1;
    }
    if (cols >= 18279 || cols <= 0) {
        printf("Error: Cols should be in the range 1 to 18278 inclusive\n");
        return 1;
    }
    
    Spreadsheet *spreadsheet = initializeSpreadsheet(rows, cols);
    spreadsheet->display = 1;
    printf("Spreadsheet initialized. Reading commands from input.txt\n");

    // Open output.txt for writing the output
    FILE *outFile = fopen("output.txt", "w");
    if (!outFile) {
        printf("Error: Unable to open output.txt for writing\n");
        freeSpreadsheet(spreadsheet);
        return 1;
    }

    // Redirect stdout to output.txt
    FILE *original_stdout = stdout;
    stdout = outFile;

    FILE *file = fopen("input.txt", "r");
    if (!file) {
        printf("Error: Unable to open input.txt\n");
        fclose(outFile);
        freeSpreadsheet(spreadsheet);
        return 1;
    }

    char input[MAX_INPUT_SIZE];
    while (fgets(input, sizeof(input), file)) {
        clock_t start = clock();
        
        if (!parseInput(input, spreadsheet, start)) {
            break;
        }
    }

    spreadsheet->display = 0;
    printf("\n");
    printSpreadsheet(spreadsheet);
    exportSpreadsheetToExcel(spreadsheet, "output.xlsx");
    fclose(file);
    fclose(outFile);

    // Restore the stdout to original (console)
    stdout = original_stdout;

    // Compare output.txt with exp.txt
    if (compareExcelFiles("output.xlsx", "expected.xlsx")) {
    printf("Test passed: output.xlsx matches expected.xlsx\n");
} else {
    printf("Test failed: output.xlsx differs from expected.xlsx\n");
}

    freeSpreadsheet(spreadsheet);
    return 0;
}
