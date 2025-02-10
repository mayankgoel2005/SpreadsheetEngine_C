#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
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

void parseCellReference(const char *ref, int *row, int *col);

typedef struct {
    Cell *operands[2];
    int count;
} OperandCollector;

static void collect_operands_callback(Cell *cell, void *data) {
    OperandCollector *collector = (OperandCollector *) data;
    if (collector->count < 2) {
        collector->operands[collector->count] = cell;
        collector->count++;
    }
}

// Callback used by clearDependencies() to remove the target cell from each dependency’s dependents tree.
static void remove_dependent_callback(Cell *source, void *target_ptr) {
    Cell *target = (Cell *) target_ptr;
    source->dependents = avl_delete(source->dependents, target, avl_cell_compare);
}

// For every cell in the dependency tree, remove this cell from its dependents tree.
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
        // Advanced operations
        if (cell->op >= OP_ADV_SUM && cell->op <= OP_ADV_STDEV) {
            int rStart = cell->row1;
            int cStart = cell->col1;
            int rEnd = cell->row2;
            int cEnd = cell->col2;
            long sum = 0;
            int count = 0;
            int minVal = INT_MAX;
            int maxVal = INT_MIN;
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
                case OP_ADV_SUM:
                    result = (int) sum;
                    break;
                case OP_ADV_MIN:
                    result = minVal;
                    break;
                case OP_ADV_MAX:
                    result = maxVal;
                    break;
                case OP_ADV_AVG:
                    result = (count > 0) ? (int)(sum / count) : 0;
                    break;
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
                default:
                    break;
            }
            cell->value = result;
            printf("Recalculated advanced formula cell new value: %d\n", cell->value);
        }
        // Simple operations
        else {
            OperandCollector collector = { .operands = {NULL, NULL}, .count = 0 };
            avl_traverse(cell->dependencies, collect_operands_callback, &collector);
            if (collector.count >= 2 && collector.operands[0] && collector.operands[1]) {
                switch (cell->op) {
                    case 1:
                        cell->value = collector.operands[0]->value + collector.operands[1]->value;
                        break;
                    case 2:
                        cell->value = collector.operands[0]->value - collector.operands[1]->value;
                        break;
                    case 3:
                        cell->value = collector.operands[0]->value * collector.operands[1]->value;
                        break;
                    case 4:
                        if (collector.operands[1]->value != 0)
                            cell->value = collector.operands[0]->value / collector.operands[1]->value;
                        else {
                            printf("Error: Division by zero in simple formula recalculation.\n");
                            return;
                        }
                        break;
                    default:
                        break;
                }
                printf("Recalculated simple formula cell new value: %d\n", cell->value);
            }
        }
    }
}

int findAffectedIndex(Cell **arr, int count, Cell *cell) {
    for (int i = 0; i < count; i++) {
        if (arr[i] == cell)
            return i;
    }
    return -1;
}

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

void recalcUsingTopoOrder(Cell *start, Spreadsheet *spreadsheet) {
    int queueCapacity = 100;
    int queueSize = 0;
    Cell **queue = malloc(queueCapacity * sizeof(Cell *));
    
    int affectedCapacity = 100;
    int affectedCount = 0;
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
    
    int *zeroQueue = malloc(affectedCount * sizeof(int));
    int zeroQueueSize = 0;
    int zeroQueueFront = 0;
    for (int i = 0; i < affectedCount; i++) {
        if (inDegree[i] == 0)
            zeroQueue[zeroQueueSize++] = i;
    }
    
    int processedCount = 0;
    ProcessDepData pData;
    pData.affected = affected;
    pData.affectedCount = affectedCount;
    pData.inDegree = inDegree;
    pData.zeroQueue = zeroQueue;
    pData.zeroQueueSize = &zeroQueueSize;
    
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

void handleOperation(const char *input, Spreadsheet *spreadsheet) {
    // Check if the input contains a '('
    if (strchr(input, '(') != NULL) {
        char targetRef[10], opStr[10], rangeStr[30];
        char extra[100];
        if (sscanf(input, "%9[^=]=%9[A-Z](%29[^)])%9s", targetRef, opStr, rangeStr, extra) == 4) {
            printf("Error: Invalid input format, unexpected characters found after function.\n");
            return;
        } else if (sscanf(input, "%9[^=]=%9[A-Z](%29[^)])", targetRef, opStr, rangeStr) != 3) {
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
        char startRef[10], endRef[10];
        const char *colon = strchr(rangeStr, ':');
        if (!colon) {
            printf("Error: Invalid range format: %s\n", rangeStr);
            return;
        }
        size_t len1 = colon - rangeStr;
        strncpy(startRef, rangeStr, len1);
        startRef[len1] = '\0';
        strcpy(endRef, colon + 1);
        int rStart, cStart, rEnd, cEnd;
        parseCellReference(startRef, &rStart, &cStart);
        parseCellReference(endRef, &rEnd, &cEnd);
        if (rStart > rEnd || cStart > cEnd) {
            printf("Error: Invalid range order: %s (should be top-left:bottom-right).\n", rangeStr);
            return;
        }
        long sum = 0;
        int count = 0;
        int minVal = INT_MAX;
        int maxVal = INT_MIN;
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
        int opCode = 0;
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
        Cell *targetCell = &spreadsheet->table[targetRow][targetCol];
        clearDependencies(targetCell);
        targetCell->op = opCode;
        targetCell->value = result;
        // Save the range boundaries.
        targetCell->row1 = rStart;
        targetCell->col1 = cStart;
        targetCell->row2 = rEnd;
        targetCell->col2 = cEnd;
        printf("Performed advanced operation %s on range %s, result: %d\n", opStr, rangeStr, result);
        // Set up dependencies
        for (int r = rStart; r <= rEnd; r++) {
            for (int c = cStart; c <= cEnd; c++) {
                Cell *source = &spreadsheet->table[r][c];
                addDependent(source, targetCell);
                addDependency(targetCell, source);
            }
        }
        recalcUsingTopoOrder(targetCell, spreadsheet);
        printSpreadsheet(spreadsheet);
    } else {
        // Simple Operation 
        char extra[100];
        char targetRef[10], sourceRef1[10], sourceRef2[10];
        char opChar;
        int value;
        if (sscanf(input, "%9[^=]=%9[^+*/-]%c%9s%9s", targetRef, sourceRef1, &opChar, sourceRef2, extra) == 5) {
            printf("Error: Invalid input format, unexpected characters found after operation.\n");
            return;
        } else if (sscanf(input, "%9[^=]=%9[^+*/-]%c%9s", targetRef, sourceRef1, &opChar, sourceRef2) == 4) {
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
            clearDependencies(targetCell);
            switch (opChar) {
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
                    printf("Error: Unsupported operation '%c'.\n", opChar);
                    return;
            }
            targetCell->row1 = row1;
            targetCell->col1 = col1;
            targetCell->row2 = row2;
            targetCell->col2 = col2;
            addDependency(targetCell, cell1);
            addDependency(targetCell, cell2);
            addDependent(cell1, targetCell);
            addDependent(cell2, targetCell);
            printf("Performed operation %s=%s%c%s, result: %d\n", targetRef, sourceRef1, opChar, sourceRef2, targetCell->value);
            recalcUsingTopoOrder(targetCell, spreadsheet);
            printSpreadsheet(spreadsheet);
        } else if (sscanf(input, "%9[^=]=%d%9s", targetRef, &value, extra) == 3) {
            printf("Error: Invalid input format, unexpected characters found after value.\n");
            return;
        } else if (sscanf(input, "%9[^=]=%d", targetRef, &value) == 2) {
            int row, col;
            parseCellReference(targetRef, &row, &col);
            if (row < 0 || row >= spreadsheet->rows || col < 0 || col >= spreadsheet->cols) {
                printf("Error: Cell reference out of bounds (%s).\n", targetRef);
                return;
            }
            Cell *targetCell = &spreadsheet->table[row][col];
            clearDependencies(targetCell);
            targetCell->op = 0;
            targetCell->value = value;
            printf("Set %s to %d\n", targetRef, value);
            recalcUsingTopoOrder(targetCell, spreadsheet);
            printSpreadsheet(spreadsheet);
        } else {
            printf("(unrecognized cmd) ");
        }
    }
}
