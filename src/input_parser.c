#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "input_parser.h"
#include "spreadsheet.h"
#include "scrolling.h"  
#include <ctype.h>
#include <time.h>

int isNumber(const char *str) {
    if (str == NULL || *str == '\0') return 0;  // Empty string is not a number
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) return 0;  // If any character is not a digit, return false
    }
    return 1;
}

clock_t end;
double cpu_time_used;

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
int parseInput(char *input, Spreadsheet *spreadsheet, clock_t start) {

    input[strcspn(input, "\n")] = '\0';

    // Exit command.
    if (strcmp(input, "q") == 0) {
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Exiting the spreadsheet. Goodbye!\n",spreadsheet->time );
        return 0;
    }
    if (strcmp(input, "disable_output") == 0) {
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Disabled output: Please Type \"enable_output\" to enable output!\n",spreadsheet->time );
        spreadsheet->display=1;
        return 1;
    }
    if (strcmp(input, "enable_output") == 0) {
        printSpreadsheet(spreadsheet);
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Output enabled!\n",spreadsheet->time );
        spreadsheet->display=0;
        return 1;
    }

    // Scroll commands.
    if (strcmp(input, "w") == 0) {
        scrollUp(spreadsheet);
        printSpreadsheet(spreadsheet);
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Scrolled Up\n",spreadsheet->time );
        return 1;
    } else if (strcmp(input, "s") == 0) {
        scrollDown(spreadsheet);
        printSpreadsheet(spreadsheet);
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Scrolled Up\n",spreadsheet->time );
        return 1;
    } else if (strcmp(input, "a") == 0) {
        scrollLeft(spreadsheet);
        printSpreadsheet(spreadsheet);
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Scrolled Left\n",spreadsheet->time );
        return 1;
    } else if (strcmp(input, "d") == 0) {
        scrollRight(spreadsheet);
        printSpreadsheet(spreadsheet);
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Scrolled Right\n",spreadsheet->time );
        return 1;
    }

if (strncmp(input, "scroll_to", 9) == 0) {
    char *token = strtok(input, " ");
    char *cellRef;

    token = strtok(NULL, " ");
    if (token == NULL) {
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Invalid format! Use: scroll_to <CellRef>\n", spreadsheet->time );
        return 1;
    }
    cellRef = token;

    token = strtok(NULL, " ");
    if (token != NULL) {
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Too many arguments! Use: scroll_to <CellRef>\n", spreadsheet->time );
        return 1;
    }

    int i = 0;
    while (isalpha(cellRef[i])) i++;
    if (i == 0 || i > 3) {
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Invalid column format! Use: scroll_to <A1 - ZZZ999>\n", spreadsheet->time );
        return 1;
    }

    char colPart[4];
    strncpy(colPart, cellRef, i);
    colPart[i] = '\0';

    char *rowPart = &cellRef[i];
    if (!isNumber(rowPart)) {
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Invalid row format! Use: scroll_to <A1 - ZZZ999>\n", spreadsheet->time );
        return 1;
    }

    int col = columnToIndex(colPart);
    int row = atoi(rowPart);

    if (row > spreadsheet->rows || row <= 0) {
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Row should be in range (1, %d)\n",spreadsheet->time , spreadsheet->rows);
        return 1;
    }
    if (col > spreadsheet->cols || col <= 0) {
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = cpu_time_used;
        printf("[%.2f] Column should be in range (A - %c)\n", spreadsheet->time , 'A' + spreadsheet->cols - 1);
        return 1;
    }

    scrollTo(spreadsheet, row, col);
    printSpreadsheet(spreadsheet);
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    spreadsheet->time = cpu_time_used;
    printf("[%.2f] Scrolled to %s", spreadsheet->time, cellRef);
    return 1;
}  

    handleOperation(input, spreadsheet, start);

    return 1;
}
