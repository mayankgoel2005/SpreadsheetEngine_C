#ifndef AVL_TREE_H
#define AVL_TREE_H

#include "cell.h"

typedef struct AVLNode {
    struct Cell *cell;
    struct AVLNode *left;
    struct AVLNode *right;
    int height;
} AVLNode;

int avl_height(AVLNode *node);
int avl_get_balance(AVLNode *node);
AVLNode* avl_insert(AVLNode *root, struct Cell *cell, int (*cmp)(struct Cell*, struct Cell*));
AVLNode* avl_delete(AVLNode *root, struct Cell *cell, int (*cmp)(struct Cell*, struct Cell*));
AVLNode* avl_find(AVLNode *root, struct Cell *cell, int (*cmp)(struct Cell*, struct Cell*));
void avl_traverse(AVLNode *root, void (*callback)(struct Cell*, void*), void *data);
void avl_free(AVLNode *root);
int avl_cell_compare(struct Cell *a, struct Cell *b);

#endif  // AVL_TREE_H
