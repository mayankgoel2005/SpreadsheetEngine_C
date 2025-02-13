#include <stdio.h>
#include <stdlib.h>
#include "spreadsheet.h"
#include "input_parser.h"
#include <time.h>

#define MAX_INPUT_SIZE 100

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <rows> <cols>\n", argv[0]);
        return 1;
    }
    
    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);
    if (rows >= 1000 || rows <= 0)
    {
        printf("Error: Rows should be in the range 1 to 1000 inclusive");
        return 1;
    }
    if (cols >= 18279 || cols <= 0)
    {
        printf("Error: Cols should be in the range 1 to 18278 inclusive");
        return 1;
    }
    

    Spreadsheet *spreadsheet = initializeSpreadsheet(rows, cols);

    printf("Spreadsheet initialized. Type 'q' to quit.\n");
    printSpreadsheet(spreadsheet);

    char input[MAX_INPUT_SIZE];
    while (1) {
        clock_t start;
        start = clock();
        
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("( unrecognized cmd ) ");
            break;
        }

        // Parse the input and handle commands
        if (!parseInput(input, spreadsheet, start)) {
            break;
        }
    }

    freeSpreadsheet(spreadsheet);
    return 0;
}
