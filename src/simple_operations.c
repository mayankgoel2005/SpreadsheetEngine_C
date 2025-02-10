#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>       // For sleep()
#include "cell.h"
#include "spreadsheet.h"
#include "avl_tree.h"       
#include "simple_operations.h"

// Advanced operation codes.
#define OP_ADV_SUM    5
#define OP_ADV_MIN    6
#define OP_ADV_MAX    7
#define OP_ADV_AVG    8
#define OP_ADV_STDEV  9
#define OP_SLEEP      10  

// Forward declarations.
void recalcUsingTopoOrder(Cell *start, Spreadsheet *spreadsheet);
void parseCellReference(const char *ref, int *row, int *col);

typedef struct {
    Cell *operands[2];
    int count;
} OperandCollector;

static void remove_dependent_callback(Cell *source, void *target_ptr) {
    Cell *target = (Cell *) target_ptr;
    source->dependents = avl_delete(source->dependents, target, avl_cell_compare);
}

void clearDependencies(Cell *cell) {
    if (cell->dependencies) {
        avl_traverse(cell->dependencies, remove_dependent_callback, cell);
        avl_free(cell->dependencies);
        cell->dependencies = NULL;
    }
}

void addDependency(Cell *targetCell, Cell *source) {
    targetCell->dependencies = avl_insert(targetCell->dependencies, source, avl_cell_compare);
}

void addDependent(Cell *sourceCell, Cell *target) {
    sourceCell->dependents = avl_insert(sourceCell->dependents, target, avl_cell_compare);
}

void recalc_cell(Cell *cell, Spreadsheet *spreadsheet) {
    if (cell->op != 0) {
        // Advanced operations.
        if (cell->op >= OP_ADV_SUM && cell->op <= OP_ADV_STDEV) {
            int rStart = cell->row1, cStart = cell->col1;
            int rEnd = cell->row2, cEnd = cell->col2;
            long sum = 0;
            int count = 0, minVal = INT_MAX, maxVal = INT_MIN;
            for (int r = rStart; r <= rEnd; r++) {
                for (int c = cStart; c <= cEnd; c++) {
                    int val = spreadsheet->table[r][c].value;
                    sum += val;
                    if (val < minVal) minVal = val;
                    if (val > maxVal) maxVal = val;
                    count++;
                }
            }
            int result = 0;
            switch (cell->op) {
                case OP_ADV_SUM: result = (int) sum; break;
                case OP_ADV_MIN: result = minVal; break;
                case OP_ADV_MAX: result = maxVal; break;
                case OP_ADV_AVG: result = (count > 0) ? (int)(sum / count) : 0; break;
                case OP_ADV_STDEV: {
                    double mean = (count > 0) ? (double) sum / count : 0.0;
                    double sqDiffSum = 0.0;
                    for (int r = rStart; r <= rEnd; r++) {
                        for (int c = cStart; c <= cEnd; c++) {
                            double diff = spreadsheet->table[r][c].value - mean;
                            sqDiffSum += diff * diff;
                        }
                    }
                    double stdev = (count > 0) ? sqrt(sqDiffSum / count) : 0.0;
                    result = (int) stdev;
                    break;
                }
                default: break;
            }
            cell->value = result;
        }
        // SLEEP operation.
        else if (cell->op == OP_SLEEP) {
            // Do nothing
        }
        // Simple operations: use the stored operand fields.
        else {
            int op1 = cell->operand1IsLiteral ? cell->operand1Literal : (cell->operand1 ? cell->operand1->value : 0);
            int op2 = cell->operand2IsLiteral ? cell->operand2Literal : (cell->operand2 ? cell->operand2->value : 0);
            switch (cell->op) {
                case 1: cell->value = op1 + op2; break;
                case 2: cell->value = op1 - op2; break;
                case 3: cell->value = op1 * op2; break;
                case 4: 
                    if (op2 != 0)
                        cell->value = op1 / op2; 
                    else {
                        printf("Error: Division by zero in simple formula recalculation.\n");
                        return;
                    }
                    break;
                default: break;
            }
        }
    }
}

// findAffectedIndex: returns index of cell in array or -1 if not found.
int findAffectedIndex(Cell **arr, int count, Cell *cell) {
    for (int i = 0; i < count; i++) {
        if (arr[i] == cell)
            return i;
    }
    return -1;
}

// Structures and callbacks for BFS & topological sorting.
typedef struct {
    Cell **queue;
    int *queueSize;
    int *queueCapacity;
    AVLNode **visited; 
} BFSQueueData;

void enqueue_cell(Cell *cell, BFSQueueData *data) {
    if (avl_find(*(data->visited), cell, avl_cell_compare))
        return;
    *(data->visited) = avl_insert(*(data->visited), cell, avl_cell_compare);
    if (*(data->queueSize) >= *(data->queueCapacity)) {
        *(data->queueCapacity) *= 2;
        data->queue = realloc(data->queue, (*(data->queueCapacity)) * sizeof(Cell *));
    }
    data->queue[*(data->queueSize)] = cell;
    (*(data->queueSize))++;
}

void bfs_enqueue_callback(Cell *cell, void *data) {
    BFSQueueData *bfsData = (BFSQueueData *) data;
    enqueue_cell(cell, bfsData);
}

typedef struct {
    Cell **affected;
    int affectedCount;
    int targetIndex;
    int *inDegree;
} DepCallbackData;

void dep_check_callback(Cell *dep, void *data) {
    DepCallbackData *dData = (DepCallbackData *) data;
    int idx = findAffectedIndex(dData->affected, dData->affectedCount, dep);
    if (idx != -1) {
        dData->inDegree[dData->targetIndex]++;
    }
}

typedef struct {
    Cell **affected;
    int affectedCount;
    int *inDegree;
    int *zeroQueue;  
    int *zeroQueueSize;
} ProcessDepData;

void process_dependent_callback(Cell *dep, void *data) {
    ProcessDepData *pData = (ProcessDepData *) data;
    int idx = findAffectedIndex(pData->affected, pData->affectedCount, dep);
    if (idx != -1) {
        pData->inDegree[idx]--;
        if (pData->inDegree[idx] == 0) {
            pData->zeroQueue[*(pData->zeroQueueSize)] = idx;
            (*(pData->zeroQueueSize))++;
        }
    }
}

// recalcUsingTopoOrder: Collect affected cells and update them once in topological order.
void recalcUsingTopoOrder(Cell *start, Spreadsheet *spreadsheet) {
    // Phase 1: BFS to collect affected cells.
    int queueCapacity = 100, queueSize = 0;
    Cell **queue = malloc(queueCapacity * sizeof(Cell *));
    int affectedCapacity = 100, affectedCount = 0;
    Cell **affected = malloc(affectedCapacity * sizeof(Cell *));
    AVLNode *visited = NULL;
    BFSQueueData bfsData;
    bfsData.queue = queue;
    bfsData.queueSize = &queueSize;
    bfsData.queueCapacity = &queueCapacity;
    bfsData.visited = &visited;
    if (start->dependents) {
        avl_traverse(start->dependents, bfs_enqueue_callback, &bfsData);
    }
    int front = 0;
    while (front < queueSize) {
        Cell *curr = queue[front++];
        if (affectedCount >= affectedCapacity) {
            affectedCapacity *= 2;
            affected = realloc(affected, affectedCapacity * sizeof(Cell *));
        }
        affected[affectedCount++] = curr;
        if (curr->dependents) {
            avl_traverse(curr->dependents, bfs_enqueue_callback, &bfsData);
        }
    }
    free(queue);
    if (visited) {
        avl_free(visited);
    }
    
    // Phase 2: Compute inDegree for each affected cell (only counting dependencies within affected).
    int *inDegree = malloc(affectedCount * sizeof(int));
    for (int i = 0; i < affectedCount; i++) {
        inDegree[i] = 0;
        if (affected[i]->dependencies) {
            DepCallbackData depData;
            depData.affected = affected;
            depData.affectedCount = affectedCount;
            depData.targetIndex = i;
            depData.inDegree = inDegree;
            avl_traverse(affected[i]->dependencies, dep_check_callback, &depData);
        }
    }
    
    // Phase 3: Build zeroQueue.
    int *zeroQueue = malloc(affectedCount * sizeof(int));
    int zeroQueueSize = 0;
    for (int i = 0; i < affectedCount; i++) {
        if (inDegree[i] == 0) {
            zeroQueue[zeroQueueSize++] = i;
        }
    }
    
    // Phase 4: Process cells in topological order (each updated exactly once).
    int processedCount = 0;
    ProcessDepData pData;
    pData.affected = affected;
    pData.affectedCount = affectedCount;
    pData.inDegree = inDegree;
    pData.zeroQueue = zeroQueue;
    pData.zeroQueueSize = &zeroQueueSize;
    
    int zeroQueueFront = 0;
    while (zeroQueueFront < zeroQueueSize) {
        int idx = zeroQueue[zeroQueueFront++];
        Cell *cell = affected[idx];
        recalc_cell(cell, spreadsheet);
        processedCount++;
        if (cell->dependents)
            avl_traverse(cell->dependents, process_dependent_callback, &pData);
    }
    
    if (processedCount != affectedCount) {
        printf("Error: Cycle detected in dependencies. Recalculation aborted.\n");
        free(affected);
        free(inDegree);
        free(zeroQueue);
        return;
    }
    
    free(affected);
    free(inDegree);
    free(zeroQueue);
}

// handleOperation: Processes input commands.
void handleOperation(const char *input, Spreadsheet *spreadsheet) {
    // If input contains '(', treat as advanced operation.
    if (strchr(input, '(') != NULL) {
        char targetRef[10], opStr[10], paramStr[30];
        char extra[100];
        if (sscanf(input, "%9[^=]=%9[A-Z](%29[^)])%9s", targetRef, opStr, paramStr, extra) == 4) {
            printf("Error: Invalid input format, unexpected characters found after function.\n");
            return;
        } else if (sscanf(input, "%9[^=]=%9[A-Z](%29[^)])", targetRef, opStr, paramStr) != 3) {
            printf("Error: Invalid advanced formula format.\n");
            return;
        }
        int targetRow, targetCol;
        parseCellReference(targetRef, &targetRow, &targetCol);
        if (targetRow < 0 || targetRow >= spreadsheet->rows ||
            targetCol < 0 || targetCol >= spreadsheet->cols) {
            printf("Error: Target cell %s is out of bounds.\n", targetRef);
            return;
        }
        Cell *targetCell = &spreadsheet->table[targetRow][targetCol];
        clearDependencies(targetCell);
        int result = 0, opCode = 0;
        int rStart, cStart, rEnd, cEnd;
        // SLEEP operation.
        if (strcmp(opStr, "SLEEP") == 0) {
            int seconds = atoi(paramStr);
            printf("Sleeping for %d seconds...\n", seconds);
            sleep(seconds);
            result = seconds;
            opCode = OP_SLEEP;
            rStart = cStart = rEnd = cEnd = -1;
        } else {
            char startRef[10], endRef[10];
            const char *colon = strchr(paramStr, ':');
            if (!colon) {
                printf("Error: Invalid range format: %s\n", paramStr);
                return;
            }
            size_t len1 = colon - paramStr;
            strncpy(startRef, paramStr, len1);
            startRef[len1] = '\0';
            strcpy(endRef, colon + 1);
            parseCellReference(startRef, &rStart, &cStart);
            parseCellReference(endRef, &rEnd, &cEnd);
            if (rStart > rEnd || cStart > cEnd) {
                printf("Error: Invalid range order: %s (should be top-left:bottom-right).\n", paramStr);
                return;
            }
            long sum = 0;
            int count = 0, minVal = INT_MAX, maxVal = INT_MIN;
            for (int r = rStart; r <= rEnd; r++) {
                for (int c = cStart; c <= cEnd; c++) {
                    int val = spreadsheet->table[r][c].value;
                    sum += val;
                    if (val < minVal) minVal = val;
                    if (val > maxVal) maxVal = val;
                    count++;
                }
            }
            if (strcmp(opStr, "SUM") == 0) {
                result = (int) sum;
                opCode = OP_ADV_SUM;
            } else if (strcmp(opStr, "MIN") == 0) {
                result = minVal;
                opCode = OP_ADV_MIN;
            } else if (strcmp(opStr, "MAX") == 0) {
                result = maxVal;
                opCode = OP_ADV_MAX;
            } else if (strcmp(opStr, "AVG") == 0) {
                result = (count > 0) ? (int)(sum / count) : 0;
                opCode = OP_ADV_AVG;
            } else if (strcmp(opStr, "STDEV") == 0) {
                double mean = (count > 0) ? (double) sum / count : 0.0;
                double sqDiffSum = 0.0;
                for (int r = rStart; r <= rEnd; r++) {
                    for (int c = cStart; c <= cEnd; c++) {
                        double diff = spreadsheet->table[r][c].value - mean;
                        sqDiffSum += diff * diff;
                    }
                }
                double stdev = (count > 0) ? sqrt(sqDiffSum / count) : 0.0;
                result = (int) stdev;
                opCode = OP_ADV_STDEV;
            } else {
                printf("Error: Unsupported advanced operation '%s'.\n", opStr);
                return;
            }
        }
        targetCell->op = opCode;
        targetCell->value = result;
        targetCell->row1 = rStart;
        targetCell->col1 = cStart;
        targetCell->row2 = rEnd;
        targetCell->col2 = cEnd;
        if (opCode != OP_SLEEP) {
            for (int r = rStart; r <= rEnd; r++) {
                for (int c = cStart; c <= cEnd; c++) {
                    Cell *source = &spreadsheet->table[r][c];
                    addDependent(source, targetCell);
                    addDependency(targetCell, source);
                }
            }
        }
        recalcUsingTopoOrder(targetCell, spreadsheet);
        printSpreadsheet(spreadsheet);
    } else {
        // Simple Operation branch.
        // First, extract the target cell and the RHS as a string.
        char targetRef[10], rhs[100];
        if (sscanf(input, "%9[^=]=%99s", targetRef, rhs) != 2) {
            printf("Error: Invalid input format.\n");
            return;
        }
        int targetRow, targetCol;
        parseCellReference(targetRef, &targetRow, &targetCol);
        Cell *targetCell = &spreadsheet->table[targetRow][targetCol];
        clearDependencies(targetCell);
        
        // Check if the RHS contains an operator.
        if (strchr(rhs, '+') || strchr(rhs, '-') || strchr(rhs, '*') || strchr(rhs, '/')) {
            // Binary operation.
            char operand1Str[20], operand2Str[20];
            char opChar;
            if (sscanf(rhs, "%19[^+*/-]%c%19s", operand1Str, &opChar, operand2Str) != 3) {
                printf("Error: Invalid binary operation format.\n");
                return;
            }
            int operand1IsLiteral = 0, operand2IsLiteral = 0;
            int literal1 = 0, literal2 = 0;
            Cell *operand1 = NULL, *operand2 = NULL;
            if (isalpha(operand1Str[0])) {
                int row1, col1;
                parseCellReference(operand1Str, &row1, &col1);
                if (row1 < 0 || row1 >= spreadsheet->rows || col1 < 0 || col1 >= spreadsheet->cols) {
                    printf("Error: Operand cell %s is out of bounds.\n", operand1Str);
                    return;
                }
                operand1 = &spreadsheet->table[row1][col1];
            } else {
                if (sscanf(operand1Str, "%d", &literal1) != 1) {
                    printf("Error: Invalid literal operand '%s'.\n", operand1Str);
                    return;
                }
                operand1IsLiteral = 1;
            }
            if (isalpha(operand2Str[0])) {
                int row2, col2;
                parseCellReference(operand2Str, &row2, &col2);
                if (row2 < 0 || row2 >= spreadsheet->rows || col2 < 0 || col2 >= spreadsheet->cols) {
                    printf("Error: Operand cell %s is out of bounds.\n", operand2Str);
                    return;
                }
                operand2 = &spreadsheet->table[row2][col2];
            } else {
                if (sscanf(operand2Str, "%d", &literal2) != 1) {
                    printf("Error: Invalid literal operand '%s'.\n", operand2Str);
                    return;
                }
                operand2IsLiteral = 1;
            }
            int result = 0;
            switch (opChar) {
                case '+': result = (operand1IsLiteral ? literal1 : operand1->value) +
                                 (operand2IsLiteral ? literal2 : operand2->value);
                          targetCell->op = 1;
                          break;
                case '-': result = (operand1IsLiteral ? literal1 : operand1->value) -
                                 (operand2IsLiteral ? literal2 : operand2->value);
                          targetCell->op = 2;
                          break;
                case '*': result = (operand1IsLiteral ? literal1 : operand1->value) *
                                 (operand2IsLiteral ? literal2 : operand2->value);
                          targetCell->op = 3;
                          break;
                case '/': if ((operand2IsLiteral ? literal2 : operand2->value) == 0) {
                              printf("Error: Division by zero.\n");
                              return;
                          }
                          result = (operand1IsLiteral ? literal1 : operand1->value) /
                                   (operand2IsLiteral ? literal2 : operand2->value);
                          targetCell->op = 4;
                          break;
                default:
                          printf("Error: Unsupported operation '%c'.\n", opChar);
                          return;
            }
            targetCell->value = result;
            // Store operand information for future recalculations.
            targetCell->operand1IsLiteral = operand1IsLiteral;
            targetCell->operand2IsLiteral = operand2IsLiteral;
            if (!operand1IsLiteral) {
                targetCell->operand1 = operand1;
                addDependency(targetCell, operand1);
                addDependent(operand1, targetCell);
            } else {
                targetCell->operand1Literal = literal1;
            }
            if (!operand2IsLiteral) {
                targetCell->operand2 = operand2;
                addDependency(targetCell, operand2);
                addDependent(operand2, targetCell);
            } else {
                targetCell->operand2Literal = literal2;
            }
            //printf("Performed operation %s=%s%c%s, result: %d\n", targetRef, operand1Str, opChar, operand2Str, result);
            recalcUsingTopoOrder(targetCell, spreadsheet);
            printSpreadsheet(spreadsheet);
        } else {
            // No operator: direct assignment.
            if (isalpha(rhs[0])) {
                int row, col;
                parseCellReference(rhs, &row, &col);
                if (row < 0 || row >= spreadsheet->rows || col < 0 || col >= spreadsheet->cols) {
                    printf("Error: Cell reference out of bounds (%s).\n", rhs);
                    return;
                }
                Cell *source = &spreadsheet->table[row][col];
                targetCell->value = source->value;
                addDependency(targetCell, source);
                addDependent(source, targetCell);
            } else {
                int val;
                if (sscanf(rhs, "%d", &val) != 1) {
                    printf("Error: Invalid literal in assignment.\n");
                    return;
                }
                targetCell->value = val;
            }
            targetCell->op = 0;
            //printf("Set %s to %d\n", targetRef, targetCell->value);
            recalcUsingTopoOrder(targetCell, spreadsheet);
            printSpreadsheet(spreadsheet);
        }
    }
}
