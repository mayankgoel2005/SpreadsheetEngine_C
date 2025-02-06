#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simple_operations.h"

// Parse a cell reference like A1 and get row/column
#include <ctype.h>

// Convert an Excel-style column reference (e.g., A, Z, AA, AB, ZZZ) to a zero-based index
void parseCellReference(const char *ref, int *row, int *col) {
    *col = 0;
    int i = 0;

    // Extract column letters (e.g., AA in AA1)
    while (isalpha(ref[i])) {
        *col = *col * 26 + (toupper(ref[i]) - 'A' + 1);
        i++;
    }
    *col = *col - 1; // Convert from 1-based to 0-based index

    // Extract row number (e.g., 1 in AA1)
    *row = atoi(ref + i) - 1; // Convert from 1-based to 0-based index
}


// Clear the dependents list for a cell
void clearDependents(Cell *cell) {
    if (cell->dependents) {
        free(cell->dependents);
        cell->dependents = NULL;
    }
    cell->dependents_count = 0;
}

// Add a cell to the dependencies list
void addDependency(Cell *dependentCell, Cell *dependencyCell) {
    dependencyCell->dependencies = realloc(dependencyCell->dependencies, sizeof(Cell *) * (dependencyCell->dep_count + 1));
    if (!dependencyCell->dependencies) {
        perror("Failed to allocate memory for dependencies");
        exit(EXIT_FAILURE);
    }
    dependencyCell->dependencies[dependencyCell->dep_count++] = dependentCell;
}

// Add a cell to the dependents list
void addDependent(Cell *cell, Cell *dependent) {
    cell->dependents = realloc(cell->dependents, sizeof(Cell *) * (cell->dependents_count + 1));
    if (!cell->dependents) {
        perror("Failed to allocate memory for dependents");
        exit(EXIT_FAILURE);
    }
    cell->dependents[cell->dependents_count++] = dependent;
}

// Recursive function to recalculate dependencies
void recalculateDependencies(Cell *cell, Spreadsheet *spreadsheet) {
    for (int i = 0; i < cell->dep_count; i++) {
        Cell *dependentCell = cell->dependencies[i];

        // Recalculate the value of the dependent cell
        Cell *cell1 = &spreadsheet->table[dependentCell->row1][dependentCell->col1];
        Cell *cell2 = &spreadsheet->table[dependentCell->row2][dependentCell->col2];

        switch (dependentCell->op) {
            case 1: // Addition
                dependentCell->value = cell1->value + cell2->value;
                break;
            case 2: // Subtraction
                dependentCell->value = cell1->value - cell2->value;
                break;
            case 3: // Multiplication
                dependentCell->value = cell1->value * cell2->value;
                break;
            case 4: // Division
                if (cell2->value != 0) {
                    dependentCell->value = cell1->value / cell2->value;
                } else {
                    printf("Error: Division by zero while recalculating.\n");
                    return;
                }
                break;
            default:
                break; // No operation
        }

        printf("Recalculated %c%d: %d\n", dependentCell->col1 + 'A', dependentCell->row1 + 1, dependentCell->value);

        // Recursively update the dependencies of the dependent cell
        recalculateDependencies(dependentCell, spreadsheet);
    }
}

// Handle simple operations
void handleSimpleOperation(const char *input, Spreadsheet *spreadsheet) {
    char target[10], source1[10], source2[10], operation;
    int value;

    // Check if the input is a direct assignment like AA1=40
    if (sscanf(input, "%9[^=]=%d", target, &value) == 2) {
        int row, col;
        parseCellReference(target, &row, &col);

        if (row < 0 || row >= spreadsheet->rows || col < 0 || col >= spreadsheet->cols) {
            printf("Error: Cell reference out of bounds (%s).\n", target);
            return;
        }

        Cell *targetCell = &spreadsheet->table[row][col];

        // Set the value and reset operation fields
        targetCell->value = value;
        targetCell->op = 0;
        targetCell->row1 = -1;
        targetCell->col1 = -1;
        targetCell->row2 = -1;
        targetCell->col2 = -1;

        // Clear dependents
        clearDependents(targetCell);

        printf("Set %s to %d\n", target, value);

        // Recalculate dependencies
        recalculateDependencies(targetCell, spreadsheet);

        printSpreadsheet(spreadsheet);
        return;
    }

    // Check if the input is a formula like AA1=B1*C1
    if (sscanf(input, "%9[^=]=%9[^+*/-]%c%9s", target, source1, &operation, source2) == 4) {
        int targetRow, targetCol, row1, col1, row2, col2;
        parseCellReference(target, &targetRow, &targetCol);
        parseCellReference(source1, &row1, &col1);
        parseCellReference(source2, &row2, &col2);

        if (targetRow < 0 || targetRow >= spreadsheet->rows ||
            targetCol < 0 || targetCol >= spreadsheet->cols ||
            row1 < 0 || row1 >= spreadsheet->rows ||
            col1 < 0 || col1 >= spreadsheet->cols ||
            row2 < 0 || row2 >= spreadsheet->rows ||
            col2 < 0 || col2 >= spreadsheet->cols) {
            printf("Error: One or more cell references are out of bounds.\n");
            return;
        }

        Cell *cell1 = &spreadsheet->table[row1][col1];
        Cell *cell2 = &spreadsheet->table[row2][col2];
        Cell *targetCell = &spreadsheet->table[targetRow][targetCol];

        // Clear existing dependents of the target cell
        clearDependents(targetCell);

        // Perform the operation
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

        // Set operation metadata
        targetCell->row1 = row1;
        targetCell->col1 = col1;
        targetCell->row2 = row2;
        targetCell->col2 = col2;

        // Add dependencies and dependents
        addDependency(targetCell, cell1);
        addDependency(targetCell, cell2);
        addDependent(cell1, targetCell);
        addDependent(cell2, targetCell);

        printf("Performed operation %s=%s%c%s, result: %d\n", target, source1, operation, source2, targetCell->value);

        // Recalculate dependencies
        recalculateDependencies(targetCell, spreadsheet);

        printSpreadsheet(spreadsheet);
        return;
    }

    printf("Error: Invalid input '%s'.\n", input);
}

