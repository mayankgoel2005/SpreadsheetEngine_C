#ifndef CELL_H
#define CELL_H

typedef struct Cell {
    int op;            // Operation code
    int row1;          // First row for operations
    int col1;          // First column for operations
    int row2;          // Second row for operations
    int col2;          // Second column for operations
    int value;         // Current value of the cell
    struct Cell **dependencies; // Array of pointers to dependent cells
    int dep_count;     // Number of dependencies
    struct Cell **dependents;   // Array of pointers to cells dependent on this cell
    int dependents_count;       // Number of dependents
} Cell;

void initCell(Cell *cell);
void freeCell(Cell *cell);

#endif
