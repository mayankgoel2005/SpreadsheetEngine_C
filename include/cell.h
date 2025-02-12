#ifndef CELL_H
#define CELL_H

#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

typedef struct AVLNode AVLNode;

// Operation codes.
#define OP_NONE       0
#define OP_ADD        1
#define OP_SUB        2
#define OP_MUL        3
#define OP_DIV        4
// Advanced operations.
#define OP_ADV_SUM    5
#define OP_ADV_MIN    6
#define OP_ADV_MAX    7
#define OP_ADV_AVG    8
#define OP_ADV_STDEV  9
#define OP_SLEEP      10

typedef struct Cell {
    int value;
    int op;
    // For advanced formulas: store the range.
    int row1, col1, row2, col2;
    
    // For simple formulas.
    int operand1IsLiteral;
    int operand1Literal;
    struct Cell *operand1;
    int operand2IsLiteral;
    int operand2Literal;
    struct Cell *operand2;
    
    // Dependency tracking.
    AVLNode *dependencies;  // cells this cell depends on.
    AVLNode *dependents;    // cells that depend on this cell.
} Cell;

void initCell(Cell *cell);
void freeCell(Cell *cell);
void parseCellReference(const char *ref, int *row, int *col);

#endif  // CELL_H
