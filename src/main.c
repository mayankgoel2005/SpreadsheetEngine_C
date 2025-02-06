#include <stdio.h>
#include <stdlib.h>
#include "spreadsheet.h"
#include "input_parser.h"

#define MAX_INPUT_SIZE 100

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <rows> <cols>\n", argv[0]);
        return 1;
    }

    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);

    if (rows <= 0 || cols <= 0) {
        printf("Error: Rows and columns must be positive integers.\n");
        return 1;
    }

    Spreadsheet *spreadsheet = initializeSpreadsheet(rows, cols);

    printf("Spreadsheet initialized. Type 'q' to quit.\n");
    printSpreadsheet(spreadsheet);

    char input[MAX_INPUT_SIZE];
    while (1) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\nError reading input.\n");
            break;
        }

        // Parse the input and handle commands
        if (!parseInput(input, spreadsheet)) {
            break;
        }
    }

    freeSpreadsheet(spreadsheet);
    return 0;
}
