// input_parser.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "input_parser.h"
#include "spreadsheet.h"
#include "simple_operations.h"
#include "scrolling.h"  

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


    handleOperation(input, spreadsheet);

    return 1;
}