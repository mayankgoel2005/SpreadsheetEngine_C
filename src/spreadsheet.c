#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include "cell.h"
#include "spreadsheet.h"
#include "avl_tree.h"

/* Operation codes for advanced formulas */
#define OP_ADV_SUM    5
#define OP_ADV_MIN    6
#define OP_ADV_MAX    7
#define OP_ADV_AVG    8
#define OP_ADV_STDEV  9
#define OP_SLEEP      10  

/* Operation codes for simple formulas */
#define OP_ADD        1
#define OP_SUB        2
#define OP_MUL        3
#define OP_DIV        4
#define OP_NONE       0

clock_t global_end;
double global_cpu_time_used;

typedef struct Node {
    Cell *cell;
    struct Node *next;
} Node;

int dfs_cycle(Cell *current, Cell *target, Spreadsheet *spreadsheet, int *inStack);

void enqueue(Node **head, Node **tail, Cell *cell) {
    Node *newNode = malloc(sizeof(Node));
    if (!newNode) {
        perror("Failed to allocate Node");
        exit(EXIT_FAILURE);
    }
    newNode->cell = cell;
    newNode->next = NULL;
    if (*tail == NULL) {
        *head = *tail = newNode;
    } else {
        (*tail)->next = newNode;
        *tail = newNode;
    }
}

Cell *dequeue(Node **head, Node **tail) {
    if (*head == NULL)
        return NULL;
    Node *temp = *head;
    Cell *cell = temp->cell;
    *head = temp->next;
    if (*head == NULL)
        *tail = NULL;
    free(temp);
    return cell;
}

typedef struct {
    Node **head;
    Node **tail;
    int *visited; 
    int totalCells;
    Spreadsheet *spreadsheet;
} BFSData;

void bfs_enqueue_if_not_visited(Cell *cell, void *data) {
    BFSData *bfsData = (BFSData *) data;
    int idx = cell->selfRow * bfsData->spreadsheet->cols + cell->selfCol;
    if (!bfsData->visited[idx]) {
        bfsData->visited[idx] = 1;
        enqueue(bfsData->head, bfsData->tail, cell);
    }
}

int existsPath(Cell *source, Cell *target, Spreadsheet *spreadsheet) {
    int totalCells = spreadsheet->rows * spreadsheet->cols;
    int *visited = calloc(totalCells, sizeof(int));
    if (!visited) {
        perror("calloc failed in existsPath");
        exit(EXIT_FAILURE);
    }
    Node *queueHead = NULL, *queueTail = NULL;
    BFSData bfsData;
    bfsData.head = &queueHead;
    bfsData.tail = &queueTail;
    bfsData.visited = visited;
    bfsData.totalCells = totalCells;
    bfsData.spreadsheet = spreadsheet;
    int startIndex = source->selfRow * spreadsheet->cols + source->selfCol;
    visited[startIndex] = 1;
    enqueue(&queueHead, &queueTail, source);
    int found = 0;
    while (queueHead != NULL) {
        Cell *curr = dequeue(&queueHead, &queueTail);
        if (curr == target) {
            found = 1;
            break;
        }
        if (curr->dependents)
            avl_traverse(curr->dependents, bfs_enqueue_if_not_visited, &bfsData);
    }
    free(visited);
    return found;
}

typedef struct {
    Cell *target;
    Spreadsheet *spreadsheet;
    int *inStack;
    int found;  
} DFSContext;

void dfs_dependency_callback(Cell *dep, void *data) {
    DFSContext *ctx = (DFSContext *) data;
    if (dfs_cycle(dep, ctx->target, ctx->spreadsheet, ctx->inStack))
        ctx->found = 1;
}

int dfs_cycle(Cell *current, Cell *target, Spreadsheet *spreadsheet, int *inStack) {
    int index = current->selfRow * spreadsheet->cols + current->selfCol;
    if (current == target)
        return 1;  /* Cycle found */
    if (inStack[index])
        return 0;  
    inStack[index] = 1;
    int found = 0;
    if (current->dependencies) {
        DFSContext ctx;
        ctx.target = target;
        ctx.spreadsheet = spreadsheet;
        ctx.inStack = inStack;
        ctx.found = 0;
        avl_traverse(current->dependencies, dfs_dependency_callback, &ctx);
        if (ctx.found)
            found = 1;
    }
    if (!found) {
        if (current->op == OP_ADD || current->op == OP_SUB ||
            current->op == OP_MUL || current->op == OP_DIV) {
            if (current->operand1 && !current->operand1IsLiteral) {
                if (dfs_cycle(current->operand1, target, spreadsheet, inStack))
                    found = 1;
            }
            if (!found && current->operand2 && !current->operand2IsLiteral) {
                if (dfs_cycle(current->operand2, target, spreadsheet, inStack))
                    found = 1;
            }
        } else if (current->op >= OP_ADV_SUM && current->op <= OP_ADV_STDEV) {
            for (int r = current->row1; r <= current->row2 && !found; r++) {
                for (int c = current->col1; c <= current->col2 && !found; c++) {
                    if (dfs_cycle(&spreadsheet->table[r][c], target, spreadsheet, inStack))
                        found = 1;
                }
            }
        }
    }
    inStack[index] = 0;
    return found;
}

int checkCycle(Cell *source, Cell *target, Spreadsheet *spreadsheet) {
    int totalCells = spreadsheet->rows * spreadsheet->cols;
    int *inStack = calloc(totalCells, sizeof(int));
    if (!inStack) {
        perror("calloc failed in checkCycle");
        exit(EXIT_FAILURE);
    }
    int cycleFound = dfs_cycle(source, target, spreadsheet, inStack);
    free(inStack);
    return cycleFound;
}

int checkAdvancedFormulaCycle(Cell *target, int rStart, int cStart, int rEnd, int cEnd, Spreadsheet *spreadsheet) {
    int totalCells = spreadsheet->rows * spreadsheet->cols;
    int *inStack = calloc(totalCells, sizeof(int));
    if (!inStack) {
        perror("calloc failed in DFS cycle check");
        exit(EXIT_FAILURE);
    }
    int cycleFound = 0;
    for (int r = rStart; r <= rEnd && !cycleFound; r++) {
        for (int c = cStart; c <= cEnd && !cycleFound; c++) {
            Cell *curr = &spreadsheet->table[r][c];
            if (dfs_cycle(curr, target, spreadsheet, inStack))
                cycleFound = 1;
        }
    }
    free(inStack);
    return cycleFound;
}

typedef struct {
    Node **head;
    Node **tail;
} LLQueueData;

void bfs_enqueue_callback_ll(Cell *cell, void *data) {
    LLQueueData *qdata = (LLQueueData *) data;
    enqueue(qdata->head, qdata->tail, cell);
}

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
    if (cell->op != OP_NONE) {
        if (cell->op >= OP_ADV_SUM && cell->op <= OP_ADV_STDEV) {
            int rStart = cell->row1, cStart = cell->col1;
            int rEnd = cell->row2, cEnd = cell->col2;
            long sum = 0;
            int count = 0, minVal = INT_MAX, maxVal = INT_MIN;
            int foundError = 0;
            for (int r = rStart; r <= rEnd; r++) {
                for (int c = cStart; c <= cEnd; c++) {
                    Cell *curr = &spreadsheet->table[r][c];
                    if (curr->error)
                        foundError = 1;
                    int val = curr->value;
                    sum += val;
                    if (val < minVal) minVal = val;
                    if (val > maxVal) maxVal = val;
                    count++;
                }
            }
            if (foundError) {
                cell->error = 1;
                return;
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
            cell->error = 0;
        }
        else if (cell->op == OP_SLEEP) {
            /* Do nothing */
        }
        else {
            if ((!cell->operand1IsLiteral && cell->operand1 && cell->operand1->error) ||
                (!cell->operand2IsLiteral && cell->operand2 && cell->operand2->error)) {
                cell->error = 1;
                return;
            }
            int op1 = cell->operand1IsLiteral ? cell->operand1Literal : (cell->operand1 ? cell->operand1->value : 0);
            int op2 = cell->operand2IsLiteral ? cell->operand2Literal : (cell->operand2 ? cell->operand2->value : 0);
            switch (cell->op) {
                case OP_ADD:
                    cell->value = op1 + op2;
                    cell->error = 0;
                    break;
                case OP_SUB:
                    cell->value = op1 - op2;
                    cell->error = 0;
                    break;
                case OP_MUL:
                    cell->value = op1 * op2;
                    cell->error = 0;
                    break;
                case OP_DIV:
                    if (op2 == 0) {
                        cell->error = 1;
                        cell->value = 0;  // Optional: assign a default value if desired.
                    } else {
                        cell->value = op1 / op2;
                        cell->error = 0;
                    }
                    break;
                
                default:
                    break;
            }
        }
    }
    else {
        if (!cell->operand1IsLiteral && cell->operand1 != NULL) {
            cell->value = cell->operand1->value;
            cell->error = cell->operand1->error;
        }
        return;
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
    Node *queueHead = NULL, *queueTail = NULL;
    LLQueueData llData;
    llData.head = &queueHead;
    llData.tail = &queueTail;
    
    int affectedCapacity = 100, affectedCount = 0;
    Cell **affected = malloc(affectedCapacity * sizeof(Cell *));
    if (!affected) {
        perror("Failed to allocate affected cells array");
        exit(EXIT_FAILURE);
    }
 
    if (start->dependents) {
        avl_traverse(start->dependents, bfs_enqueue_callback_ll, &llData);
    }
    while (queueHead != NULL) {
        Cell *curr = dequeue(&queueHead, &queueTail);
        if (affectedCount >= affectedCapacity) {
            affectedCapacity *= 2;
            affected = realloc(affected, affectedCapacity * sizeof(Cell *));
            if (!affected) {
                perror("Failed to reallocate affected cells array");
                exit(EXIT_FAILURE);
            }
        }
        affected[affectedCount++] = curr;
        if (curr->dependents)
            avl_traverse(curr->dependents, bfs_enqueue_callback_ll, &llData);
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
    for (int i = 0; i < affectedCount; i++) {
        if (inDegree[i] == 0) {
            zeroQueue[zeroQueueSize++] = i;
        }
    }
    
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
        if (cell->dependents)
            avl_traverse(cell->dependents, process_dependent_callback, &pData);
    }
    free(affected);
    free(inDegree);
    free(zeroQueue);
}

static void addAdvancedFormula(Spreadsheet *spreadsheet, Cell *cell) {
    for (int i = 0; i < spreadsheet->advancedFormulasCount; i++) {
        if (spreadsheet->advancedFormulas[i] == cell)
            return;
    }
    if (spreadsheet->advancedFormulasCount >= spreadsheet->advancedFormulasCapacity) {
        spreadsheet->advancedFormulasCapacity *= 2;
        spreadsheet->advancedFormulas = realloc(spreadsheet->advancedFormulas,
            spreadsheet->advancedFormulasCapacity * sizeof(Cell *));
        if (!spreadsheet->advancedFormulas) {
            perror("Failed to reallocate advanced formulas array");
            exit(EXIT_FAILURE);
        }
    }
    spreadsheet->advancedFormulas[spreadsheet->advancedFormulasCount++] = cell;
}

static void removeAdvancedFormula(Spreadsheet *spreadsheet, Cell *cell) {
    for (int i = 0; i < spreadsheet->advancedFormulasCount; i++) {
        if (spreadsheet->advancedFormulas[i] == cell) {
            spreadsheet->advancedFormulas[i] = spreadsheet->advancedFormulas[spreadsheet->advancedFormulasCount - 1];
            spreadsheet->advancedFormulasCount--;
            break;
        }
    }
}

static void recalcAllAdvancedFormulas(Spreadsheet *spreadsheet) {
    int count = spreadsheet->advancedFormulasCount;
    if (count == 0)
         return;
    int *inDegree = malloc(count * sizeof(int));
    for (int i = 0; i < count; i++) {
         inDegree[i] = 0;
    }
 
    for (int i = 0; i < count; i++) {
         Cell *X = spreadsheet->advancedFormulas[i];
         for (int j = 0; j < count; j++) {
             if (i == j) continue;
             Cell *Y = spreadsheet->advancedFormulas[j];
             if (Y->selfRow >= X->row1 && Y->selfRow <= X->row2 &&
                 Y->selfCol >= X->col1 && Y->selfCol <= X->col2) {
                 inDegree[i]++;
             }
         }
    }
    int *zeroQueue = malloc(count * sizeof(int));
    int zeroQueueSize = 0;
    for (int i = 0; i < count; i++) {
         if (inDegree[i] == 0) {
             zeroQueue[zeroQueueSize++] = i;
         }
    }
    int processedCount = 0;
    int *topoOrder = malloc(count * sizeof(int));
    int topoIndex = 0;
    while (zeroQueueSize > 0) {
         int idx = zeroQueue[--zeroQueueSize];
         topoOrder[topoIndex++] = idx;
         processedCount++;
         Cell *Y = spreadsheet->advancedFormulas[idx];
         for (int j = 0; j < count; j++) {
              if (j == idx) continue;
              Cell *X = spreadsheet->advancedFormulas[j];
              if (Y->selfRow >= X->row1 && Y->selfRow <= X->row2 &&
                  Y->selfCol >= X->col1 && Y->selfCol <= X->col2) {
                   inDegree[j]--;
                   if (inDegree[j] == 0) {
                        zeroQueue[zeroQueueSize++] = j;
                   }
              }
         }
    }
    if (processedCount != count) {
         printf("Error: Cycle detected in advanced formulas.\n");
         free(inDegree);
         free(zeroQueue);
         free(topoOrder);
         return;
    }
    for (int i = 0; i < count; i++) {
         int idx = topoOrder[i];
         Cell *cell = spreadsheet->advancedFormulas[idx];
         recalc_cell(cell, spreadsheet);
         recalcUsingTopoOrder(cell, spreadsheet);
    }
    free(inDegree);
    free(zeroQueue);
    free(topoOrder);
}

void handleOperation(const char *input, Spreadsheet *spreadsheet, clock_t start) {
    if (strcmp(input, "disable_output") == 0) {
        spreadsheet->display = 1;
        global_end = clock();
        global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = global_cpu_time_used;
        printf("[%.2f] (ok)\n", spreadsheet->time);
        return;
    }
    if (strcmp(input, "enable_output") == 0) {
        spreadsheet->display = 0;
        global_end = clock();
        global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = global_cpu_time_used;
        printf("[%.2f] (ok)\n", spreadsheet->time);
        return;
    }
 
    /* Advanced formulas (input contains '(') */
    if (strchr(input, '(') != NULL) {
        char targetRef[10], opStr[10], paramStr[30];
        char extra[100];
        if (sscanf(input, "%9[^=]=%9[A-Z](%29[^)])%9s", targetRef, opStr, paramStr, extra) == 4) {
            global_end = clock();
            global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
            spreadsheet->time = global_cpu_time_used;
            printf("[%.2f] Error: Invalid input format, unexpected characters found after function.\n", spreadsheet->time);
            return;
        } else if (sscanf(input, "%9[^=]=%9[A-Z](%29[^)])", targetRef, opStr, paramStr) != 3) {
            global_end = clock();
            global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
            spreadsheet->time = global_cpu_time_used;
            printf("[%.2f] Error: Invalid advanced formula format.\n", spreadsheet->time);
            return;
        }
        int targetRow, targetCol;
        parseCellReference(targetRef, &targetRow, &targetCol);
        if (targetRow < 0 || targetRow >= spreadsheet->rows ||
            targetCol < 0 || targetCol >= spreadsheet->cols) {
            global_end = clock();
            global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
            spreadsheet->time = global_cpu_time_used;
            printf("[%.2f] Error: Target cell %s is out of bounds.\n", spreadsheet->time, targetRef);
            return;
        }
        Cell *targetCell = &spreadsheet->table[targetRow][targetCol];
        clearDependencies(targetCell);
        removeAdvancedFormula(spreadsheet, targetCell);
 
        int result = 0, opCode = 0;
        int rStart = -1, cStart = -1, rEnd = -1, cEnd = -1;
 
        if (strcmp(opStr, "SLEEP") == 0) {
            int seconds = 0;
            if (isalpha(paramStr[0])) {
                int row, col;
                parseCellReference(paramStr, &row, &col);
                if (row < 0 || row >= spreadsheet->rows || col < 0 || col >= spreadsheet->cols) {
                    global_end = clock();
                    global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                    spreadsheet->time = global_cpu_time_used;
                    printf("[%.2f] Error: Cell reference %s is out of bounds.\n", spreadsheet->time, paramStr);
                    return;
                }
                Cell *source = &spreadsheet->table[row][col];
                addDependency(targetCell, source);
                addDependent(source, targetCell);
                if (source->error) {
                    targetCell->error = 1;
                    printSpreadsheet(spreadsheet);
                    return;
                }
                seconds = source->value;
            } else {
                seconds = atoi(paramStr);
            }
            printf("Sleeping for %d seconds...\n", seconds);
            sleep(seconds);
            result = seconds;
            opCode = OP_SLEEP;
            targetCell->op = opCode;
            targetCell->value = result;
            targetCell->row1 = rStart;
            targetCell->col1 = cStart;
            targetCell->row2 = rEnd;
            targetCell->col2 = cEnd;
            
    
            recalc_cell(targetCell, spreadsheet);
            recalcUsingTopoOrder(targetCell, spreadsheet);
            recalcAllAdvancedFormulas(spreadsheet);
            
            
            spreadsheet->time = result;
            // printf("(ok) [%.2f]\n", spreadsheet->time);
            printf("ok %.0f\n", spreadsheet->time);
            printSpreadsheet(spreadsheet);
            return;
        } else {
            char startRef[10], endRef[10];
            const char *colon = strchr(paramStr, ':');
            if (!colon) {
                global_end = clock();
                global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                spreadsheet->time = global_cpu_time_used;
                printf("[%.2f] Error: Invalid range format: %s\n", spreadsheet->time, paramStr);
                return;
            }
            size_t len1 = colon - paramStr;
            strncpy(startRef, paramStr, len1);
            startRef[len1] = '\0';
            strcpy(endRef, colon + 1);
            parseCellReference(startRef, &rStart, &cStart);
            parseCellReference(endRef, &rEnd, &cEnd);
            if (rStart > rEnd || cStart > cEnd) {
                global_end = clock();
                global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                spreadsheet->time = global_cpu_time_used;
                printf("[%.2f] Error: Invalid range order: %s (should be top-left:bottom-right).\n", spreadsheet->time, paramStr);
                return;
            }
            if (targetRow >= rStart && targetRow <= rEnd &&
                targetCol >= cStart && targetCol <= cEnd) {
                global_end = clock();
                global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                spreadsheet->time = global_cpu_time_used;
                printf("[%.2f] Error: Advanced formula creates a direct self-reference. Formula rejected.\n", spreadsheet->time);
                return;
            }
            /* Run DFS-based cycle check on the advanced formula's range */
            if (checkAdvancedFormulaCycle(targetCell, rStart, cStart, rEnd, cEnd, spreadsheet)) {
                global_end = clock();
                global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                spreadsheet->time = global_cpu_time_used;
                printf("[%.2f] Error: Advanced formula would create a cyclic dependency. Formula rejected.\n", spreadsheet->time);
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
                global_end = clock();
                global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                spreadsheet->time = global_cpu_time_used;
                printf("[%.2f] Error: Unsupported advanced operation '%s'.\n", spreadsheet->time, opStr);
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
            addAdvancedFormula(spreadsheet, targetCell);
        }
 
        recalc_cell(targetCell, spreadsheet);
        recalcUsingTopoOrder(targetCell, spreadsheet);
        recalcAllAdvancedFormulas(spreadsheet);
        printSpreadsheet(spreadsheet);
        global_end = clock();
        global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
        spreadsheet->time = global_cpu_time_used;
        printf("[%.2f] (ok)\n", spreadsheet->time);
        return;
    } else {
        /* Simple assignment or reference branch */
        char targetRef[10], rhs[100];
        char extra[10]; // Buffer to capture unexpected trailing characters
        if (sscanf(input, "%9[^=]=%99s%9s", targetRef, rhs, extra) == 3) {
            global_end = clock();
            global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
            spreadsheet->time = global_cpu_time_used;
            printf("[%.2f] Error: Invalid input format, unexpected characters after assignment.\n", spreadsheet->time);
            return;
        }
        int targetRow, targetCol;
        parseCellReference(targetRef, &targetRow, &targetCol);
        if (targetRow < 0 || targetRow >= spreadsheet->rows || targetCol < 0 || targetCol >= spreadsheet->cols) {
            printf("Error: Target cell out of bounds.\n");
            return;
        }
        Cell *targetCell = &spreadsheet->table[targetRow][targetCol];
        clearDependencies(targetCell);
 
        int val;
        if (rhs[0] == '-')
        {
            char extra[10]; // Buffer to capture unexpected characters
            if (sscanf(rhs + 1, "%d%9s", &val, extra) != 1) {
                global_end = clock();
                global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                spreadsheet->time = global_cpu_time_used;
                printf("Error %.0f \n", spreadsheet->time);
                return;
            }
            targetCell->value = -val;
            targetCell->op = OP_NONE;
            recalcUsingTopoOrder(targetCell, spreadsheet);
            recalcAllAdvancedFormulas(spreadsheet);
            
            global_end = clock();
            global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
            spreadsheet->time = global_cpu_time_used;
            // printf("(ok) [%.2f]\n", spreadsheet->time);
            printf("ok %.0f\n", spreadsheet->time);
            fflush(stdout);
            printSpreadsheet(spreadsheet);
        }
        else if (strchr(rhs, '+') || strchr(rhs, '-') || strchr(rhs, '*') || strchr(rhs, '/')) {
            char operand1Str[20], operand2Str[20];
            char opChar;
            if (sscanf(rhs, "%19[^+*/-]%c%19s", operand1Str, &opChar, operand2Str) != 3) {
                global_end = clock();
                global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                spreadsheet->time = global_cpu_time_used;
                printf("[%.2f] Error: Invalid binary operation format.\n", spreadsheet->time);
                return;
            }
            int operand1IsLiteral = 0, operand2IsLiteral = 0;
            int literal1 = 0, literal2 = 0;
            Cell *operand1 = NULL, *operand2 = NULL;
            if (isalpha(operand1Str[0])) {
                int row1, col1;
                parseCellReference(operand1Str, &row1, &col1);
                if (row1 < 0 || row1 >= spreadsheet->rows || col1 < 0 || col1 >= spreadsheet->cols) {
                    global_end = clock();
                    global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                    spreadsheet->time = global_cpu_time_used;
                    printf("[%.2f] Error: Operand cell %s is out of bounds.\n", spreadsheet->time, operand1Str);
                    return;
                }
                operand1 = &spreadsheet->table[row1][col1];
                /* Use DFS-based check for cycles on full dependency chain */
                if (checkCycle(operand1, targetCell, spreadsheet)) {
                    global_end = clock();
                    global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                    spreadsheet->time = global_cpu_time_used;
                    printf("[%.2f] Error: Cyclic dependency detected via operand %s. Formula rejected.\n", 
                           spreadsheet->time, operand1Str);
                    return;
                }
            } else {
                if (sscanf(operand1Str, "%d", &literal1) != 1) {
                    global_end = clock();
                    global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                    spreadsheet->time = global_cpu_time_used;
                    printf("[%.2f] Error: Invalid literal operand '%s'.\n", spreadsheet->time, operand1Str);
                    return;
                }
                operand1IsLiteral = 1;
            }
            if (isalpha(operand2Str[0])) {
                int row2, col2;
                parseCellReference(operand2Str, &row2, &col2);
                if (row2 < 0 || row2 >= spreadsheet->rows || col2 < 0 || col2 >= spreadsheet->cols) {
                    global_end = clock();
                    global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                    spreadsheet->time = global_cpu_time_used;
                    printf("[%.2f] Error: Operand cell %s is out of bounds.\n", spreadsheet->time, operand2Str);
                    return;
                }
                operand2 = &spreadsheet->table[row2][col2];
                if (checkCycle(operand2, targetCell, spreadsheet)) {
                    global_end = clock();
                    global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                    spreadsheet->time = global_cpu_time_used;
                    printf("[%.2f] Error: Cyclic dependency detected via operand %s. Formula rejected.\n", 
                           spreadsheet->time, operand2Str);
                    return;
                }
            } else {
                if (sscanf(operand2Str, "%d", &literal2) != 1) {
                    global_end = clock();
                    global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                    spreadsheet->time = global_cpu_time_used;
                    printf("[%.2f] Error: Invalid literal operand '%s'.\n", spreadsheet->time, operand2Str);
                    return;
                }
                operand2IsLiteral = 1;
            }
            int result = 0;
            switch (opChar) {
                case '+': result = (operand1IsLiteral ? literal1 : operand1->value) +
                                 (operand2IsLiteral ? literal2 : operand2->value);
                          targetCell->op = OP_ADD;
                          break;
                case '-': result = (operand1IsLiteral ? literal1 : operand1->value) -
                                 (operand2IsLiteral ? literal2 : operand2->value);
                          targetCell->op = OP_SUB;
                          break;
                case '*': result = (operand1IsLiteral ? literal1 : operand1->value) *
                                 (operand2IsLiteral ? literal2 : operand2->value);
                          targetCell->op = OP_MUL;
                          break;
                case '/': 
                          if ((operand2IsLiteral ? literal2 : operand2->value) == 0) {
                              targetCell->error = 1;
                              result = 0;  // Optional: assign a default value if desired.
                          } else {
                              result = (operand1IsLiteral ? literal1 : operand1->value) /
                                       (operand2IsLiteral ? literal2 : operand2->value);
                              targetCell->error = 0;
                          }
                          targetCell->op = OP_DIV;
                          break;
                      
                default:
                          global_end = clock();
                          global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                          spreadsheet->time = global_cpu_time_used;
                          printf("[%.2f] Error: Unsupported operation '%c'.\n", spreadsheet->time, opChar);
                          return;
            }
            targetCell->value = result;
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
            removeAdvancedFormula(spreadsheet, targetCell);
            recalcUsingTopoOrder(targetCell, spreadsheet);
            recalcAllAdvancedFormulas(spreadsheet);
            printSpreadsheet(spreadsheet);
            global_end = clock();
            global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
            spreadsheet->time = global_cpu_time_used;
            printf("[%.2f] (ok)\n", spreadsheet->time);
            return;
        } else {
            /* Direct assignment branch */
            if (isalpha(rhs[0])) {
                int row, col;
                parseCellReference(rhs, &row, &col);
                if (row < 0 || row >= spreadsheet->rows || col < 0 || col >= spreadsheet->cols) {
                    global_end = clock();
                    global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                    spreadsheet->time = global_cpu_time_used;
                    printf("[%.2f] Error: Cell reference out of bounds (%s).\n", spreadsheet->time, rhs);
                    return;
                }
                Cell *source = &spreadsheet->table[row][col];
                /* Use DFS-based check to see if source (which might be advanced) leads to target */
                if (checkCycle(source, targetCell, spreadsheet)) {
                    global_end = clock();
                    global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                    spreadsheet->time = global_cpu_time_used;
                    printf("[%.2f] Error: Cyclic dependency detected via direct assignment (%s).\n", spreadsheet->time, rhs);
                    return;
                }
                targetCell->value = source->value;
                targetCell->operand1 = source;
                targetCell->operand1IsLiteral = 0;
                addDependency(targetCell, source);
                addDependent(source, targetCell);
            } else {
                int val;
                char extra[10]; // Buffer to capture unexpected characters
                if (sscanf(rhs, "%d%9s", &val, extra) != 1) {
                    global_end = clock();
                    global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
                    spreadsheet->time = global_cpu_time_used;
                    printf("[%.2f] Error: Invalid literal in assignment.\n", spreadsheet->time);
                    return;
                }
                targetCell->value = val;
                targetCell->error = 0;
            }
            targetCell->op = OP_NONE;
            removeAdvancedFormula(spreadsheet, targetCell);
            recalcUsingTopoOrder(targetCell, spreadsheet);
            recalcAllAdvancedFormulas(spreadsheet);
            printSpreadsheet(spreadsheet);
            global_end = clock();
            global_cpu_time_used = ((double)(global_end - start)) / CLOCKS_PER_SEC;
            spreadsheet->time = global_cpu_time_used;
            printf("[%.2f] (ok)\n", spreadsheet->time);
        }
    }
}

Spreadsheet *initializeSpreadsheet(int rows, int cols) {
    Spreadsheet *spreadsheet = malloc(sizeof(Spreadsheet));
    if (!spreadsheet) {
        perror("Failed to allocate memory for Spreadsheet");
        exit(EXIT_FAILURE);
    }
    spreadsheet->display = 0;
    spreadsheet->rows = rows;
    spreadsheet->cols = cols;
    spreadsheet->time = 0.0;
    spreadsheet->startRow = 0;
    spreadsheet->startCol = 0;
    spreadsheet->table = malloc(rows * sizeof(Cell *));
    if (!spreadsheet->table) {
        perror("Failed to allocate memory for spreadsheet table");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < rows; i++) {
        spreadsheet->table[i] = malloc(cols * sizeof(Cell));
        if (!spreadsheet->table[i]) {
            perror("Failed to allocate memory for a row of cells");
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < cols; j++) {
            initCell(&spreadsheet->table[i][j], i, j);
            spreadsheet->table[i][j].selfRow = i;
            spreadsheet->table[i][j].selfCol = j;
        }
    }
    spreadsheet->advancedFormulasCapacity = 10;
    spreadsheet->advancedFormulasCount = 0;
    spreadsheet->advancedFormulas = malloc(spreadsheet->advancedFormulasCapacity * sizeof(Cell *));
    if (!spreadsheet->advancedFormulas) {
        perror("Failed to allocate memory for advanced formulas list");
        exit(EXIT_FAILURE);
    }
    return spreadsheet;
}
 
void getColumnLabel(int colIndex, char *label) {
    int i = 0;
    char temp[4];
    while (colIndex >= 0) {
        temp[i++] = 'A' + (colIndex % 26);
        colIndex = (colIndex / 26) - 1;
    }
    temp[i] = '\0';
    int len = strlen(temp);
    for (int j = 0; j < len; j++) {
        label[j] = temp[len - j - 1];
    }
    label[len] = '\0';
}
 
void printSpreadsheet(Spreadsheet *spreadsheet) {
    if (spreadsheet->display == 1)
        return;
    int endRow = (spreadsheet->startRow + 10 < spreadsheet->rows) ? spreadsheet->startRow + 10 : spreadsheet->rows;
    int endCol = (spreadsheet->startCol + 10 < spreadsheet->cols) ? spreadsheet->startCol + 10 : spreadsheet->cols;
    printf("%4s", "");
    for (int col = spreadsheet->startCol; col < endCol; col++) {
        char label[4];
        getColumnLabel(col, label);
        printf("%12s", label);
    }
    printf("\n");
    for (int row = spreadsheet->startRow; row < endRow; row++) {
        printf("%4d", row + 1);
        for (int col = spreadsheet->startCol; col < endCol; col++) {
            Cell *cell = &spreadsheet->table[row][col];
            if (cell->error) {
                printf("%12s", "ERR");
            } else {
                printf("%12d", cell->value);
            }
        }
        printf("\n");
    }
}
 
void freeSpreadsheet(Spreadsheet *spreadsheet) {
    for (int i = 0; i < spreadsheet->rows; i++) {
        for (int j = 0; j < spreadsheet->cols; j++) {
            freeCell(&spreadsheet->table[i][j]);
        }
        free(spreadsheet->table[i]);
    }
    free(spreadsheet->table);
    free(spreadsheet->advancedFormulas);
    free(spreadsheet);
}
