#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <stddef.h>

// Forward declaration for Cell.
struct Cell;

// AVL tree node structure.
typedef struct AVLNode {
    struct Cell *cell;     // Pointer to the cell stored in this node.
    int height;            // Height of this node (for balancing).
    struct AVLNode *left;  // Pointer to the left child.
    struct AVLNode *right; // Pointer to the right child.
} AVLNode;

// Function prototypes for AVL tree operations.
AVLNode* avl_insert(AVLNode *root, struct Cell *cell, int (*cmp)(struct Cell*, struct Cell*));
AVLNode* avl_delete(AVLNode *root, struct Cell *cell, int (*cmp)(struct Cell*, struct Cell*));
AVLNode* avl_find(AVLNode *root, struct Cell *cell, int (*cmp)(struct Cell*, struct Cell*));
void avl_traverse(AVLNode *root, void (*callback)(struct Cell*, void*), void *data);
void avl_free(AVLNode *root);

// Helper functions.
int avl_height(AVLNode *node);
int avl_get_balance(AVLNode *node);
AVLNode* avl_right_rotate(AVLNode *y);
AVLNode* avl_left_rotate(AVLNode *x);

// Default comparison function for cells.
int avl_cell_compare(struct Cell *a, struct Cell *b);

#endif
