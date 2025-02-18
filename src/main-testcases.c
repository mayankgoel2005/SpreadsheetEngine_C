#include <stdio.h>
#include <stdlib.h>
#include "spreadsheet.h"
#include "input_parser.h"
#include <time.h>
#include <string.h>

#define MAX_INPUT_SIZE 100

void compareFiles(const char *outputFile, const char *expFile) {
    FILE *outFile = fopen(outputFile, "r");
    FILE *expFilePtr = fopen(expFile, "r");

    if (!outFile || !expFilePtr) {
        printf("Error: Unable to open one of the files for comparison.\n");
        return;
    }

    char outLine[MAX_INPUT_SIZE], expLine[MAX_INPUT_SIZE];
    int lineNum = 1;

    while (fgets(outLine, sizeof(outLine), outFile) && fgets(expLine, sizeof(expLine), expFilePtr)) {
        if (strcmp(outLine, expLine) != 0) {
            printf("Difference at line %d:\n", lineNum);
            printf("Output: %s", outLine);
            printf("Expected: %s", expLine);
        }
        lineNum++;
    }

    fclose(outFile);
    fclose(expFilePtr);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <rows> <cols>\n", argv[0]);
        return 1;
    }
    
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

    fclose(file);
    fclose(outFile);

    // Restore the stdout to original (console)
    stdout = original_stdout;

    // Compare output.txt with exp.txt
    compareFiles("output.txt", "exp.txt");

    freeSpreadsheet(spreadsheet);
    return 0;
}
