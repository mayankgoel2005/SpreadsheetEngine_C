// cell.h
#ifndef CELL_H
#define CELL_H

typedef struct Cell {
    int row, col;       // Cell location
    int value;          // Current value
    int op;             // 0 = constant; 1 = +; 2 = -; 3 = *; 4 = /
    // Formula fields:
    int row1, col1;     // Coordinates for operand1 (if reference)
    struct Cell* cell1; // Pointer to operand1 (if used)
    int row2, col2;     // Coordinates for operand2 (if reference)
    struct Cell* cell2; // Pointer to operand2 (if used)
    int isLiteral1;     // 1 if operand1 is literal; 0 if reference
    int literal1;       // Literal value for operand1
    int isLiteral2;     // 1 if operand2 is literal; 0 if reference
    int literal2;       // Literal value for operand2

    // Dependency tracking for formulas that depend on this cell:
    struct Cell* avlroot; // Root pointer for the AVL tree of dependent cells
    struct Cell* left;    // Left child (AVL tree)
    struct Cell* right;   // Right child (AVL tree)
    int height;           // AVL tree node height
} Cell;

void initCell(Cell *cell);
void freeCell(Cell *cell);

#endif // CELL_H