#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H

#include "spreadsheet.h"
#include "scrolling.h"
#include "simple_operations.h"

// Parse user input and handle commands
// Returns 0 if the program should exit, 1 otherwise
int parseInput(char *input, Spreadsheet *spreadsheet);

#endif
