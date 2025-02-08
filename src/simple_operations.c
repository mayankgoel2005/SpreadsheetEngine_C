// simple_operations.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "simple_operations.h"
#include "spreadsheet.h"
#include "cell.h"

//---------------------------------------------------------------------
// Forward Declarations for Static Helper Functions
//---------------------------------------------------------------------
static void topoDFS_recursive(Cell *cell, bool *visited, Cell **stack, int *stackIndex, Spreadsheet *spreadsheet);
static void topoDFS(Cell *cell, bool *visited, Cell **stack, int *stackIndex, Spreadsheet *spreadsheet);
static bool hasCycleUtil(Cell *current, Cell *target, bool *visited, int totalCells, Spreadsheet *spreadsheet);
static void traverseAVLCycle(Cell *node, bool *found, Cell *target, bool *visited, int totalCells, Spreadsheet *spreadsheet);

//---------------------------------------------------------------------
// AVL Tree Helper Functions (for dependency trees)
// These functions use the cell’s own left/right pointers for AVL tree management.
//---------------------------------------------------------------------
static int getHeight(Cell *node) {
    return (node == NULL) ? 0 : node->height;
}

static int maxInt(int a, int b) {
    return (a > b) ? a : b;
}

static Cell* rightRotate(Cell *y) {
    Cell *x = y->left;
    Cell *T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = maxInt(getHeight(y->left), getHeight(y->right)) + 1;
    x->height = maxInt(getHeight(x->left), getHeight(x->right)) + 1;
    return x;
}

static Cell* leftRotate(Cell *x) {
    Cell *y = x->right;
    Cell *T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = maxInt(getHeight(x->left), getHeight(x->right)) + 1;
    y->height = maxInt(getHeight(y->left), getHeight(y->right)) + 1;
    return y;
}

static int getBalance(Cell *node) {
    return (node == NULL) ? 0 : getHeight(node->left) - getHeight(node->right);
}

static int compareCells(Cell *a, Cell *b) {
    if (a->row != b->row)
        return a->row - b->row;
    return a->col - b->col;
}

// Insert a cell (as a dependency) into an AVL tree rooted at 'root'.
static Cell* avl_insert(Cell *root, Cell *node) {
    if (root == NULL) {
        node->left = node->right = NULL;
        node->height = 1;
        return node;
    }
    int cmp = compareCells(node, root);
    if (cmp < 0)
        root->left = avl_insert(root->left, node);
    else if (cmp > 0)
        root->right = avl_insert(root->right, node);
    else
        return root; // Already present.

    root->height = maxInt(getHeight(root->left), getHeight(root->right)) + 1;
    int balance = getBalance(root);
    if (balance > 1 && compareCells(node, root->left) < 0)
        return rightRotate(root);
    if (balance < -1 && compareCells(node, root->right) > 0)
        return leftRotate(root);
    if (balance > 1 && compareCells(node, root->left) > 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }
    if (balance < -1 && compareCells(node, root->right) < 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }
    return root;
}

static Cell* minValueNode(Cell *node) {
    Cell *current = node;
    while (current && current->left != NULL)
        current = current->left;
    return current;
}

// Delete a cell (dependency) from an AVL tree rooted at 'root'.
static Cell* avl_delete(Cell *root, Cell *node) {
    if (root == NULL)
        return root;
    int cmp = compareCells(node, root);
    if (cmp < 0)
        root->left = avl_delete(root->left, node);
    else if (cmp > 0)
        root->right = avl_delete(root->right, node);
    else {
        if (root->left == NULL || root->right == NULL) {
            Cell *temp = root->left ? root->left : root->right;
            free(root);
            return temp;
        } else {
            Cell *temp = minValueNode(root->right);
            // Copy temp's data into root.
            root->row = temp->row;
            root->col = temp->col;
            root->value = temp->value;
            root->op = temp->op;
            root->row1 = temp->row1;
            root->col1 = temp->col1;
            root->cell1 = temp->cell1;
            root->row2 = temp->row2;
            root->col2 = temp->col2;
            root->cell2 = temp->cell2;
            root->isLiteral1 = temp->isLiteral1;
            root->literal1 = temp->literal1;
            root->isLiteral2 = temp->isLiteral2;
            root->literal2 = temp->literal2;
            root->avlroot = temp->avlroot;
            root->left = root->right = NULL;
            root->right = avl_delete(root->right, temp);
        }
    }
    if (root == NULL)
        return root;
    root->height = maxInt(getHeight(root->left), getHeight(root->right)) + 1;
    int balance = getBalance(root);
    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);
    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }
    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);
    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }
    return root;
}

//---------------------------------------------------------------------
// Topological Sorting for Recalculation
//---------------------------------------------------------------------
// We perform a DFS from the changed cell (and through its dependency tree) to collect affected cells in a stack.
static void topoDFS_recursive(Cell *cell, bool *visited, Cell **stack, int *stackIndex, Spreadsheet *spreadsheet) {
    if (cell == NULL)
        return;
    int idx = cell->row * spreadsheet->cols + cell->col;
    if (visited[idx])
        return;
    visited[idx] = true;
    // Traverse the AVL tree of dependents once.
    if (cell->avlroot)
        topoDFS_recursive(cell->avlroot, visited, stack, stackIndex, spreadsheet);
    // Traverse the left and right children of this cell in the dependency tree.
    topoDFS_recursive(cell->left, visited, stack, stackIndex, spreadsheet);
    topoDFS_recursive(cell->right, visited, stack, stackIndex, spreadsheet);
    // After processing dependents, push this cell on the stack.
    stack[(*stackIndex)++] = cell;
}

static void topoDFS(Cell *cell, bool *visited, Cell **stack, int *stackIndex, Spreadsheet *spreadsheet) {
    topoDFS_recursive(cell, visited, stack, stackIndex, spreadsheet);
}

// Recalculate affected cells in topological order.
void recalcUsingTopoOrder(Cell *start, Spreadsheet *spreadsheet) {
    int totalCells = spreadsheet->rows * spreadsheet->cols;
    bool *visited = calloc(totalCells, sizeof(bool));
    if (!visited) {
        perror("Memory allocation failed in recalcUsingTopoOrder");
        exit(EXIT_FAILURE);
    }
    Cell **stack = malloc(totalCells * sizeof(Cell *));
    if (!stack) {
        perror("Memory allocation failed for stack in recalcUsingTopoOrder");
        exit(EXIT_FAILURE);
    }
    int stackIndex = 0;
    topoDFS(start, visited, stack, &stackIndex, spreadsheet);
    // Process the stack in reverse order.
    for (int i = stackIndex - 1; i >= 0; i--) {
        Cell *current = stack[i];
        if (current->op != 0) { // Only recalc formula cells.
            int op1 = current->isLiteral1 ? current->literal1 :
                      spreadsheet->table[current->row1][current->col1].value;
            int op2 = current->isLiteral2 ? current->literal2 :
                      spreadsheet->table[current->row2][current->col2].value;
            switch (current->op) {
                case 1: current->value = op1 + op2; break;
                case 2: current->value = op1 - op2; break;
                case 3: current->value = op1 * op2; break;
                case 4:
                    if (op2 != 0)
                        current->value = op1 / op2;
                    else {
                        printf("Error: Division by zero in cell %c%d.\n",
                               current->col + 'A', current->row + 1);
                        current->value = 0;
                    }
                    break;
                default: break;
            }
        }
    }
    free(stack);
    free(visited);
}

//---------------------------------------------------------------------
// Cycle Detection Functions
//---------------------------------------------------------------------
// The dependency graph is defined by edges from an operand cell to the formula cell that depends on it.
// A cycle exists if, starting from a cell, you can eventually reach that same cell.
static void traverseAVLCycle(Cell *node, bool *found, Cell *target, bool *visited, int totalCells, Spreadsheet *spreadsheet) {
    if (node == NULL || *found)
        return;
    if (hasCycleUtil(node, target, visited, totalCells, spreadsheet)) {
        *found = true;
        return;
    }
    traverseAVLCycle(node->left, found, target, visited, totalCells, spreadsheet);
    traverseAVLCycle(node->right, found, target, visited, totalCells, spreadsheet);
}

static bool hasCycleUtil(Cell *current, Cell *target, bool *visited, int totalCells, Spreadsheet *spreadsheet) {
    int idx = current->row * spreadsheet->cols + current->col;
    if (current == target)
        return true;
    if (visited[idx])
        return false;
    visited[idx] = true;
    bool found = false;
    if (current->avlroot)
        traverseAVLCycle(current->avlroot, &found, target, visited, totalCells, spreadsheet);
    return found;
}

bool hasCycle(Cell *start, Cell *target, Spreadsheet *spreadsheet) {
    int total = spreadsheet->rows * spreadsheet->cols;
    bool *visited = calloc(total, sizeof(bool));
    if (!visited) {
        perror("Memory allocation failed in cycle detection");
        exit(EXIT_FAILURE);
    }
    bool result = hasCycleUtil(start, target, visited, total, spreadsheet);
    free(visited);
    return result;
}

//---------------------------------------------------------------------
// Helper: Parse a cell reference string (e.g., "A1") into row and col indices.
//---------------------------------------------------------------------
void parseCellReference(const char *ref, int *row, int *col) {
    *col = ref[0] - 'A';
    *row = atoi(ref + 1) - 1;
}

//---------------------------------------------------------------------
// Simple Operations Handling
//---------------------------------------------------------------------
// For direct assignments, remove the target cell from any operand dependency trees,
// then set the new value and recalc using topological order.
// For formula assignments, check for cycles, update formula fields, update dependency trees,
// and then recalc.
void handleSimpleOperation(const char *input, Spreadsheet *spreadsheet) {
    char target[4], source1[16], source2[16], opChar;
    int value;

    // Direct assignment, e.g., "A1=10"
    if (sscanf(input, "%3[^=]=%d", target, &value) == 2) {
        int row, col;
        parseCellReference(target, &row, &col);
        Cell *targetCell = &spreadsheet->table[row][col];
        if (targetCell->cell1 != NULL && targetCell->cell1->avlroot != NULL) {
            targetCell->cell1->avlroot = avl_delete(targetCell->cell1->avlroot, targetCell);
            targetCell->cell1 = NULL;
        }
        if (targetCell->cell2 != NULL && targetCell->cell2->avlroot != NULL) {
            targetCell->cell2->avlroot = avl_delete(targetCell->cell2->avlroot, targetCell);
            targetCell->cell2 = NULL;
        }
        targetCell->value = value;
        targetCell->op = 0;
        targetCell->row1 = targetCell->col1 = targetCell->row2 = targetCell->col2 = -1;
        targetCell->isLiteral1 = targetCell->isLiteral2 = 0;
        recalcUsingTopoOrder(targetCell, spreadsheet);
        printSpreadsheet(spreadsheet);
        return;
    }

    // Formula assignment, e.g., "C1=A1+B1"
    if (sscanf(input, "%3[^=]=%15[^+*/-]%c%15s", target, source1, &opChar, source2) == 4) {
        int targetRow, targetCol;
        parseCellReference(target, &targetRow, &targetCol);
        int row1 = -1, col1 = -1, row2 = -1, col2 = -1;
        int isLiteral1 = 0, literal1 = 0;
        int isLiteral2 = 0, literal2 = 0;

        if (source1[0] >= 'A' && source1[0] <= 'Z')
            parseCellReference(source1, &row1, &col1);
        else {
            literal1 = atoi(source1);
            isLiteral1 = 1;
        }
        if (source2[0] >= 'A' && source2[0] <= 'Z')
            parseCellReference(source2, &row2, &col2);
        else {
            literal2 = atoi(source2);
            isLiteral2 = 1;
        }

        Cell *cell1 = (!isLiteral1) ? &spreadsheet->table[row1][col1] : NULL;
        Cell *cell2 = (!isLiteral2) ? &spreadsheet->table[row2][col2] : NULL;
        Cell *targetCell = &spreadsheet->table[targetRow][targetCol];

        if (targetCell->cell1 != NULL && targetCell->cell1->avlroot != NULL) {
            targetCell->cell1->avlroot = avl_delete(targetCell->cell1->avlroot, targetCell);
        }
        if (targetCell->cell2 != NULL && targetCell->cell2->avlroot != NULL) {
            targetCell->cell2->avlroot = avl_delete(targetCell->cell2->avlroot, targetCell);
        }

        if (!isLiteral1 && cell1 != NULL) {
            if (hasCycle(targetCell, cell1, spreadsheet)) {
                printf("Error: Cycle detected when adding dependency from cell %c%d to %c%d.\n",
                       cell1->col + 'A', cell1->row + 1, targetCell->col + 'A', targetCell->row + 1);
                return;
            }
        }
        if (!isLiteral2 && cell2 != NULL) {
            if (hasCycle(targetCell, cell2, spreadsheet)) {
                printf("Error: Cycle detected when adding dependency from cell %c%d to %c%d.\n",
                       cell2->col + 'A', cell2->row + 1, targetCell->col + 'A', targetCell->row + 1);
                return;
            }
        }

        switch (opChar) {
            case '+':
                if (!isLiteral1 && !isLiteral2)
                    targetCell->value = cell1->value + cell2->value;
                else if (isLiteral1 && !isLiteral2)
                    targetCell->value = literal1 + cell2->value;
                else if (!isLiteral1 && isLiteral2)
                    targetCell->value = cell1->value + literal2;
                else
                    targetCell->value = literal1 + literal2;
                targetCell->op = 1;
                break;
            case '-':
                if (!isLiteral1 && !isLiteral2)
                    targetCell->value = cell1->value - cell2->value;
                else if (isLiteral1 && !isLiteral2)
                    targetCell->value = literal1 - cell2->value;
                else if (!isLiteral1 && isLiteral2)
                    targetCell->value = cell1->value - literal2;
                else
                    targetCell->value = literal1 - literal2;
                targetCell->op = 2;
                break;
            case '*':
                if (!isLiteral1 && !isLiteral2)
                    targetCell->value = cell1->value * cell2->value;
                else if (isLiteral1 && !isLiteral2)
                    targetCell->value = literal1 * cell2->value;
                else if (!isLiteral1 && isLiteral2)
                    targetCell->value = cell1->value * literal2;
                else
                    targetCell->value = literal1 * literal2;
                targetCell->op = 3;
                break;
            case '/': {
                int divisor = (!isLiteral2) ? cell2->value : literal2;
                if (divisor != 0) {
                    if (!isLiteral1 && !isLiteral2)
                        targetCell->value = cell1->value / cell2->value;
                    else if (isLiteral1 && !isLiteral2)
                        targetCell->value = literal1 / cell2->value;
                    else if (!isLiteral1 && isLiteral2)
                        targetCell->value = cell1->value / literal2;
                    else
                        targetCell->value = literal1 / literal2;
                    targetCell->op = 4;
                } else {
                    printf("Error: Division by zero.\n");
                    return;
                }
                break;
            }
            default:
                printf("Error: Unsupported operation '%c'.\n", opChar);
                return;
        }
        targetCell->row1 = row1; targetCell->col1 = col1;
        targetCell->isLiteral1 = isLiteral1; targetCell->literal1 = literal1;
        targetCell->row2 = row2; targetCell->col2 = col2;
        targetCell->isLiteral2 = isLiteral2; targetCell->literal2 = literal2;
        targetCell->cell1 = cell1; targetCell->cell2 = cell2;

        if (!isLiteral1 && cell1 != NULL)
            cell1->avlroot = avl_insert(cell1->avlroot, targetCell);
        if (!isLiteral2 && cell2 != NULL)
            cell2->avlroot = avl_insert(cell2->avlroot, targetCell);

        recalcUsingTopoOrder(targetCell, spreadsheet);
        printSpreadsheet(spreadsheet);
        return;
    }
    printf("Error: Invalid input '%s'.\n", input);
}