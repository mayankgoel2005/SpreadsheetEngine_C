#ifndef CELL_H
#define CELL_H

typedef struct Cell {
    int op;            // Operation code: 0 = no formula; 1 = addition; 2 = subtraction; 3 = multiplication; 4 = division.
    int value;         // Current value of the cell.
    // For a formula cell (op != 0):
    //   - dependencies holds pointers to the source cells (e.g., for C1 = A1 * B1, dependencies[0] points to A1, dependencies[1] points to B1).
    //   - dep_count is the number of source cells.
    struct Cell **dependencies;
    int dep_count;
    // For every cell:
    //   - dependents holds pointers to cells that use this cell’s value in a formula.
    //     (e.g., if C1 = A1 * B1, then A1.dependents and B1.dependents each include a pointer to C1)
    //   - dependents_count is the number of such cells.
    struct Cell **dependents;
    int dependents_count;
} Cell;

void initCell(Cell *cell);
void freeCell(Cell *cell);

#endif
