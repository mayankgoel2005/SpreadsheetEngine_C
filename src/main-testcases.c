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
        printf("Error: Rows should be in the range 1 to 1000 inclusive\n");
        return 1;
    }
    if (cols >= 18279 || cols <= 0)
    {
        printf("Error: Cols should be in the range 1 to 18278 inclusive\n");
        return 1;
    }
    
    Spreadsheet *spreadsheet = initializeSpreadsheet(rows, cols);
    spreadsheet->display = 1;
    printf("Spreadsheet initialized. Reading commands from input.txt\n");
    

    FILE *file = fopen("input.txt", "r");
    if (!file) {
        printf("Error: Unable to open input.txt\n");
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
    spreadsheet->display=0;
    printf("\n");
    printSpreadsheet(spreadsheet);
    fclose(file);
    freeSpreadsheet(spreadsheet);
    return 0;
}
