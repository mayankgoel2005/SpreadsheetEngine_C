#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "simple_operations.h"
#include "avl_tree.h"

// Forward declaration of recalculateDependents so that it can be used in callbacks.
void recalculateDependents(Cell *cell, Spreadsheet *spreadsheet);

// Convert an Excel-style cell reference (e.g., "A1", "AA1") to zero-based indices.
void parseCellReference(const char *ref, int *row, int *col) {
    *col = 0;
    int i = 0;
    while (isalpha(ref[i])) {
        *col = *col * 26 + (toupper(ref[i]) - 'A' + 1);
        i++;
    }
    *col = *col - 1;  // Convert from 1-based to 0-based
    *row = atoi(ref + i) - 1;  // Convert from 1-based to 0-based
}

// Callback used during traversal to remove a dependent from a source cell's dependents tree.
void remove_dependent_callback(struct Cell *source, void *target_ptr) {
    struct Cell *target = (struct Cell *) target_ptr;
    source->dependents = avl_delete(source->dependents, target, avl_cell_compare);
}

// Clear the dependencies AVL tree for a cell.
// For every source in the tree, remove this cell from that source’s dependents.
void clearDependencies(Cell *cell) {
    if (cell->dependencies) {
        avl_traverse(cell->dependencies, remove_dependent_callback, cell);
        avl_free(cell->dependencies);
        cell->dependencies = NULL;
    }
}

// Insert a dependency: record that targetCell depends on source.
void addDependency(Cell *targetCell, Cell *source) {
    targetCell->dependencies = avl_insert(targetCell->dependencies, source, avl_cell_compare);
}

// Insert a dependent: record that sourceCell has dependent.
void addDependent(Cell *sourceCell, Cell *dependent) {
    sourceCell->dependents = avl_insert(sourceCell->dependents, dependent, avl_cell_compare);
}

// Helper structure to collect up to two operands from a cell's dependencies tree.
typedef struct {
    Cell *operands[2];
    int count;
} OperandCollector;

// Callback to collect operands from the dependencies AVL tree.
void collect_operands_callback(Cell *cell, void *data) {
    OperandCollector *collector = (OperandCollector *) data;
    if (collector->count < 2) {
        collector->operands[collector->count] = cell;
        collector->count++;
    }
}

// Callback for recalc traversal of dependents.
void recalc_callback(Cell *dependent, void *spreadsheet_ptr) {
    Spreadsheet *spreadsheet = (Spreadsheet *) spreadsheet_ptr;
    if (dependent->op != 0) {
        OperandCollector collector = { .operands = {NULL, NULL}, .count = 0 };
        avl_traverse(dependent->dependencies, collect_operands_callback, &collector);
        if (collector.count >= 2 && collector.operands[0] && collector.operands[1]) {
            switch (dependent->op) {
                case 1: // Addition
                    dependent->value = collector.operands[0]->value + collector.operands[1]->value;
                    break;
                case 2: // Subtraction
                    dependent->value = collector.operands[0]->value - collector.operands[1]->value;
                    break;
                case 3: // Multiplication
                    dependent->value = collector.operands[0]->value * collector.operands[1]->value;
                    break;
                case 4: // Division
                    if (collector.operands[1]->value != 0)
                        dependent->value = collector.operands[0]->value / collector.operands[1]->value;
                    else {
                        printf("Error: Division by zero while recalculating.\n");
                        return;
                    }
                    break;
                default:
                    break;
            }
            printf("Recalculated dependent cell new value: %d\n", dependent->value);
        }
        recalculateDependents(dependent, spreadsheet);
    }
}

// Recursively recalculate all cells that depend on the given cell.
void recalculateDependents(Cell *cell, Spreadsheet *spreadsheet) {
    avl_traverse(cell->dependents, recalc_callback, spreadsheet);
}

// Handle simple operations (direct assignment or formulas).
void handleSimpleOperation(const char *input, Spreadsheet *spreadsheet) {
    char targetRef[10], sourceRef1[10], sourceRef2[10], operation;
    int value;

    // Direct assignment: e.g., "C1=40"
    if (sscanf(input, "%9[^=]=%d", targetRef, &value) == 2) {
        int row, col;
        parseCellReference(targetRef, &row, &col);
        if (row < 0 || row >= spreadsheet->rows || col < 0 || col >= spreadsheet->cols) {
            printf("Error: Cell reference out of bounds (%s).\n", targetRef);
            return;
        }
        Cell *targetCell = &spreadsheet->table[row][col];
        // Clear old formula info and remove targetCell from old sources' dependents trees.
        clearDependencies(targetCell);
        targetCell->op = 0;  // Direct assignment.
        targetCell->value = value;
        printf("Set %s to %d\n", targetRef, value);
        recalculateDependents(targetCell, spreadsheet);
        printSpreadsheet(spreadsheet);
        return;
    }

    // Formula assignment: e.g., "C1=A1*B1" or later "C1=D1*E1"
    if (sscanf(input, "%9[^=]=%9[^+*/-]%c%9s", targetRef, sourceRef1, &operation, sourceRef2) == 4) {
        int targetRow, targetCol, row1, col1, row2, col2;
        parseCellReference(targetRef, &targetRow, &targetCol);
        parseCellReference(sourceRef1, &row1, &col1);
        parseCellReference(sourceRef2, &row2, &col2);
        if (targetRow < 0 || targetRow >= spreadsheet->rows ||
            targetCol < 0 || targetCol >= spreadsheet->cols ||
            row1 < 0 || row1 >= spreadsheet->rows ||
            col1 < 0 || col1 >= spreadsheet->cols ||
            row2 < 0 || row2 >= spreadsheet->rows ||
            col2 < 0 || col2 >= spreadsheet->cols) {
            printf("Error: One or more cell references are out of bounds.\n");
            return;
        }
        Cell *targetCell = &spreadsheet->table[targetRow][targetCol];
        Cell *cell1 = &spreadsheet->table[row1][col1];
        Cell *cell2 = &spreadsheet->table[row2][col2];

        // Clear any previous formula (and remove targetCell from its old dependencies' dependents).
        clearDependencies(targetCell);

        // Set the new formula operator and compute the initial value.
        switch (operation) {
            case '+':
                targetCell->value = cell1->value + cell2->value;
                targetCell->op = 1;
                break;
            case '-':
                targetCell->value = cell1->value - cell2->value;
                targetCell->op = 2;
                break;
            case '*':
                targetCell->value = cell1->value * cell2->value;
                targetCell->op = 3;
                break;
            case '/':
                if (cell2->value != 0) {
                    targetCell->value = cell1->value / cell2->value;
                    targetCell->op = 4;
                } else {
                    printf("Error: Division by zero.\n");
                    return;
                }
                break;
            default:
                printf("Error: Unsupported operation '%c'.\n", operation);
                return;
        }

        // Establish new dependency relationships:
        // 1. Record that targetCell depends on cell1 and cell2.
        addDependency(targetCell, cell1);
        addDependency(targetCell, cell2);
        // 2. Record that cell1 and cell2 have targetCell as a dependent.
        addDependent(cell1, targetCell);
        addDependent(cell2, targetCell);

        printf("Performed operation %s=%s%c%s, result: %d\n", targetRef, sourceRef1, operation, sourceRef2, targetCell->value);
        recalculateDependents(targetCell, spreadsheet);
        printSpreadsheet(spreadsheet);
        return;
    }

    printf("Error: Invalid input '%s'.\n", input);
}
