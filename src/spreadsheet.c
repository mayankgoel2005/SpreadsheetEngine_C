#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spreadsheet.h"

// Initialize the spreadsheet with given rows and columns
Spreadsheet *initializeSpreadsheet(int rows, int cols) {
    Spreadsheet *spreadsheet = malloc(sizeof(Spreadsheet));
    spreadsheet->rows = rows;
    spreadsheet->cols = cols;
    spreadsheet->startRow = 0;
    spreadsheet->startCol = 0; // Default visible portion starts at (0, 0)

    // Allocate memory for the table
    spreadsheet->table = malloc(rows * sizeof(Cell *));
    for (int i = 0; i < rows; i++) {
        spreadsheet->table[i] = malloc(cols * sizeof(Cell));
        for (int j = 0; j < cols; j++) {
            initCell(&spreadsheet->table[i][j]);
        }
    }

    return spreadsheet;
}

// Helper function to convert a column index to Excel-style column letters
void getColumnLabel(int colIndex, char *label) {
    int i = 0;
    char temp[4]; // Temporary buffer for up to 3 letters + null terminator
    while (colIndex >= 0) {
        temp[i++] = 'A' + (colIndex % 26);
        colIndex = (colIndex / 26) - 1;
    }
    temp[i] = '\0';

    // Reverse the string to get the correct column label
    int len = strlen(temp);
    for (int j = 0; j < len; j++) {
        label[j] = temp[len - j - 1];
    }
    label[len] = '\0';
}

// Print the visible 10x10 portion of the spreadsheet
void printSpreadsheet(Spreadsheet *spreadsheet) {
    int endRow = spreadsheet->startRow + 10 < spreadsheet->rows ? spreadsheet->startRow + 10 : spreadsheet->rows;
    int endCol = spreadsheet->startCol + 10 < spreadsheet->cols ? spreadsheet->startCol + 10 : spreadsheet->cols;

    // Print column headers
    printf("   ");
    for (int col = spreadsheet->startCol; col < endCol; col++) {
        char label[4];
        getColumnLabel(col, label);
        printf("  %-4s", label);
    }
    printf("\n");

    // Print top border
    printf("   ");
    for (int col = spreadsheet->startCol; col < endCol; col++) {
        printf("+-----");
    }
    printf("+\n");

    // Print rows with values
    for (int row = spreadsheet->startRow; row < endRow; row++) {
        printf("%2d ", row + 1); // Row numbers
        for (int col = spreadsheet->startCol; col < endCol; col++) {
            printf("| %3d ", spreadsheet->table[row][col].value);
        }
        printf("|\n");

        // Print row border
        printf("   ");
        for (int col = spreadsheet->startCol; col < endCol; col++) {
            printf("+-----");
        }
        printf("+\n");
    }
}

// Free the memory allocated for the spreadsheet
void freeSpreadsheet(Spreadsheet *spreadsheet) {
    for (int i = 0; i < spreadsheet->rows; i++) {
        for (int j = 0; j < spreadsheet->cols; j++) {
            freeCell(&spreadsheet->table[i][j]);
        }
        free(spreadsheet->table[i]);
    }
    free(spreadsheet->table);
    free(spreadsheet);
}
