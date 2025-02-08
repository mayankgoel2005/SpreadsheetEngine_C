#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "simple_operations.h"

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

// Remove a specific dependent from a source cell's dependents list.
// Searches sourceCell->dependents for the pointer 'dependent' and removes it.
void removeDependentFromSource(Cell *sourceCell, Cell *dependent) {
    if (sourceCell->dependents == NULL || sourceCell->dependents_count == 0)
        return;

    int i;
    for (i = 0; i < sourceCell->dependents_count; i++) {
        if (sourceCell->dependents[i] == dependent) {
            // Shift all later entries one position to the left.
            int j;
            for (j = i; j < sourceCell->dependents_count - 1; j++) {
                sourceCell->dependents[j] = sourceCell->dependents[j + 1];
            }
            sourceCell->dependents_count--;
            // Optionally, shrink the memory allocation.
            if (sourceCell->dependents_count > 0) {
                sourceCell->dependents = realloc(sourceCell->dependents, sizeof(Cell *) * sourceCell->dependents_count);
            } else {
                free(sourceCell->dependents);
                sourceCell->dependents = NULL;
            }
            break;  // We found and removed the dependent; exit the loop.
        }
    }
}

// Clear the dependencies list for a cell.
// This function removes the cell (the target cell) from the dependents lists of each source cell
// that it previously depended on, then frees its own dependencies list.
void clearDependencies(Cell *cell) {
    if (cell->dependencies) {
        for (int i = 0; i < cell->dep_count; i++) {
            Cell *source = cell->dependencies[i];
            removeDependentFromSource(source, cell);
        }
        free(cell->dependencies);
        cell->dependencies = NULL;
    }
    cell->dep_count = 0;
}

// Add a dependency to the target cell: record that the target cell depends on 'source'.
void addDependency(Cell *targetCell, Cell *source) {
    targetCell->dependencies = realloc(targetCell->dependencies, sizeof(Cell *) * (targetCell->dep_count + 1));
    if (!targetCell->dependencies) {
        perror("Failed to allocate memory for dependencies");
        exit(EXIT_FAILURE);
    }
    targetCell->dependencies[targetCell->dep_count++] = source;
}

// Add a dependent to a source cell: record that 'dependent' depends on this source cell.
void addDependent(Cell *sourceCell, Cell *dependent) {
    sourceCell->dependents = realloc(sourceCell->dependents, sizeof(Cell *) * (sourceCell->dependents_count + 1));
    if (!sourceCell->dependents) {
        perror("Failed to allocate memory for dependents");
        exit(EXIT_FAILURE);
    }
    sourceCell->dependents[sourceCell->dependents_count++] = dependent;
}

// Recursively recalculate all cells that depend on the given cell.
void recalculateDependents(Cell *cell, Spreadsheet *spreadsheet) {
    for (int i = 0; i < cell->dependents_count; i++) {
        Cell *dependentCell = cell->dependents[i];
        if (dependentCell->op != 0 && dependentCell->dep_count >= 2) {  // Ensure it's a formula cell.
            // For a binary operation, assume the first two dependencies are the operands.
            Cell *operand1 = dependentCell->dependencies[0];
            Cell *operand2 = dependentCell->dependencies[1];
            switch (dependentCell->op) {
                case 1: // Addition
                    dependentCell->value = operand1->value + operand2->value;
                    break;
                case 2: // Subtraction
                    dependentCell->value = operand1->value - operand2->value;
                    break;
                case 3: // Multiplication
                    dependentCell->value = operand1->value * operand2->value;
                    break;
                case 4: // Division
                    if (operand2->value != 0)
                        dependentCell->value = operand1->value / operand2->value;
                    else {
                        printf("Error: Division by zero while recalculating.\n");
                        continue;
                    }
                    break;
                default:
                    break;
            }
            printf("Recalculated dependent cell new value: %d\n", dependentCell->value);
            recalculateDependents(dependentCell, spreadsheet);
        }
    }
}

// Handle simple operations (direct assignment or formulas).
// When a cell is updated, all cells in its dependents list are recalculated recursively.
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
        // Clear any old formula information (and remove C1 from old sources' dependents).
        clearDependencies(targetCell);
        targetCell->op = 0;  // No formula; it's a direct value.
        targetCell->value = value;
        printf("Set %s to %d\n", targetRef, value);
        // Propagate changes: recalc all cells that depend on this cell.
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

        // Clear any previous formula from the target cell.
        // This will also remove targetCell (C1) from the dependents lists of its old dependencies (e.g., A1 and B1).
        clearDependencies(targetCell);

        // Set the new formula's operator and compute the initial value.
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

        // Establish the new dependency relationship:
        // 1. Record that targetCell (e.g., C1) depends on cell1 and cell2.
        addDependency(targetCell, cell1);
        addDependency(targetCell, cell2);
        // 2. Record that cell1 and cell2 have targetCell as a dependent.
        addDependent(cell1, targetCell);
        addDependent(cell2, targetCell);

        printf("Performed operation %s=%s%c%s, result: %d\n", targetRef, sourceRef1, operation, sourceRef2, targetCell->value);
        // Propagate changes: update any cells that depend on targetCell.
        recalculateDependents(targetCell, spreadsheet);
        printSpreadsheet(spreadsheet);
        return;
    }

    printf("Error: Invalid input '%s'.\n", input);
}
