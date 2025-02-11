// input_parser.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "input_parser.h"
#include "spreadsheet.h"
#include "simple_operations.h"
#include "scrolling.h"  
#include <ctype.h>

int isNumber(const char *str) {
    if (str == NULL || *str == '\0') return 0;  // Empty string is not a number
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) return 0;  // If any character is not a digit, return false
    }
    return 1;
}


int columnToIndex(const char *col) {
    int index = 0;
    while (*col) {
        if (!isalpha(*col)) return -1;
        index = index * 26 + (toupper(*col) - 'A' + 1);
        col++;
    }
    return index;
}


// function to parse and handle user input.
int parseInput(char *input, Spreadsheet *spreadsheet) {

    input[strcspn(input, "\n")] = '\0';

    // Exit command.
    if (strcmp(input, "q") == 0) {
        printf("Exiting the spreadsheet. Goodbye!\n");
        return 0;
    }
    if (strcmp(input, "disable_output") == 0) {
        printf("Disabled output: Please Type \"enable_output\" to enable output!\n");
        spreadsheet->display=1;
        return 1;
    }
    if (strcmp(input, "enable_output") == 0) {
        printf("Output enabled!\n");
        spreadsheet->display=0;
        printSpreadsheet(spreadsheet);
        return 1;
    }

    // Scroll commands.
    if (strcmp(input, "w") == 0) {
        scrollUp(spreadsheet);
        printSpreadsheet(spreadsheet);
        return 1;
    } else if (strcmp(input, "s") == 0) {
        scrollDown(spreadsheet);
        printSpreadsheet(spreadsheet);
        return 1;
    } else if (strcmp(input, "a") == 0) {
        scrollLeft(spreadsheet);
        printSpreadsheet(spreadsheet);
        return 1;
    } else if (strcmp(input, "d") == 0) {
        scrollRight(spreadsheet);
        printSpreadsheet(spreadsheet);
        return 1;
    }

if (strncmp(input, "scroll_to", 9) == 0) {
    char *token = strtok(input, " ");
    char *cellRef;

    token = strtok(NULL, " ");
    if (token == NULL) {
        printf("Invalid format! Use: scroll_to <CellRef>\n");
        return 1;
    }
    cellRef = token;

    token = strtok(NULL, " ");
    if (token != NULL) {
        printf("Too many arguments! Use: scroll_to <CellRef>\n");
        return 1;
    }

    int i = 0;
    while (isalpha(cellRef[i])) i++;
    if (i == 0 || i > 3) {
        printf("Invalid column format! Use: scroll_to <A1 - ZZZ999>\n");
        return 1;
    }

    char colPart[4];
    strncpy(colPart, cellRef, i);
    colPart[i] = '\0';

    char *rowPart = &cellRef[i];
    if (!isNumber(rowPart)) {
        printf("Invalid row format! Use: scroll_to <A1 - ZZZ999>\n");
        return 1;
    }

    int col = columnToIndex(colPart);
    int row = atoi(rowPart);

    if (row > spreadsheet->rows || row <= 0) {
        printf("Row should be in range (1, %d)\n", spreadsheet->rows);
        return 1;
    }
    if (col > spreadsheet->cols || col <= 0) {
        printf("Column should be in range (A - %c)\n", 'A' + spreadsheet->cols - 1);
        return 1;
    }

    scrollTo(spreadsheet, row, col);
    printSpreadsheet(spreadsheet);
    return 1;
}  


    handleOperation(input, spreadsheet);

    return 1;
}