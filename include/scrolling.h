#ifndef SCROLLING_H
#define SCROLLING_H

#include "spreadsheet.h"

// Handle scrolling commands
void scrollUp(Spreadsheet *spreadsheet);
void scrollDown(Spreadsheet *spreadsheet);
void scrollLeft(Spreadsheet *spreadsheet);
void scrollRight(Spreadsheet *spreadsheet);
void scrollTo(Spreadsheet *spreadsheet, int row, int col);

#endif
