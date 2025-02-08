// input_parser.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "input_parser.h"
#include "spreadsheet.h"
#include "simple_operations.h"
#include "scrolling.h"  // if you have scrolling functions defined elsewhere

// Function to parse and handle user input.
int parseInput(char *input, Spreadsheet *spreadsheet) {
    // Remove trailing newline.
    input[strcspn(input, "\n")] = '\0';

    // Exit command.
    if (strcmp(input, "q") == 0) {
        printf("Exiting the spreadsheet. Goodbye!\n");
        return 0;
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

    // Otherwise, assume it's a cell operation.
    handleSimpleOperation(input, spreadsheet);

    return 1;
}