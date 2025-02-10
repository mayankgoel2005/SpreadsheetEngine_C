#include "avl_tree.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "cell.h"
int avl_height(AVLNode *node) {
    return node ? node->height : 0;
}

int avl_get_balance(AVLNode *node) {
    return node ? avl_height(node->left) - avl_height(node->right) : 0;
}

AVLNode* avl_right_rotate(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = 1 + (avl_height(y->left) > avl_height(y->right) ? avl_height(y->left) : avl_height(y->right));
    x->height = 1 + (avl_height(x->left) > avl_height(x->right) ? avl_height(x->left) : avl_height(x->right));
    return x;
}

AVLNode* avl_left_rotate(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = 1 + (avl_height(x->left) > avl_height(x->right) ? avl_height(x->left) : avl_height(x->right));
    y->height = 1 + (avl_height(y->left) > avl_height(y->right) ? avl_height(y->left) : avl_height(y->right));
    return y;
}

int avl_cell_compare(struct Cell *a, struct Cell *b) {
    // Compare using pointer values.
    if ((uintptr_t)a < (uintptr_t)b) return -1;
    else if ((uintptr_t)a > (uintptr_t)b) return 1;
    else return 0;
}

static AVLNode* create_node(struct Cell *cell) {
    AVLNode *node = (AVLNode*) malloc(sizeof(AVLNode));
    if (!node) {
        perror("Failed to allocate AVLNode");
        exit(EXIT_FAILURE);
    }
    node->cell = cell;
    node->left = node->right = NULL;
    node->height = 1;
    return node;
}

AVLNode* avl_insert(AVLNode *root, struct Cell *cell, int (*cmp)(struct Cell*, struct Cell*)) {
    if (!root)
        return create_node(cell);

    int comp = cmp(cell, root->cell);
    if (comp < 0)
        root->left = avl_insert(root->left, cell, cmp);
    else if (comp > 0)
        root->right = avl_insert(root->right, cell, cmp);
    else
        return root;  // duplicate key, do nothing

    root->height = 1 + (avl_height(root->left) > avl_height(root->right) ? avl_height(root->left) : avl_height(root->right));
    int balance = avl_get_balance(root);

    // Left Left Case
    if (balance > 1 && cmp(cell, root->left->cell) < 0)
        return avl_right_rotate(root);
    // Right Right Case
    if (balance < -1 && cmp(cell, root->right->cell) > 0)
        return avl_left_rotate(root);
    // Left Right Case
    if (balance > 1 && cmp(cell, root->left->cell) > 0) {
        root->left = avl_left_rotate(root->left);
        return avl_right_rotate(root);
    }
    // Right Left Case
    if (balance < -1 && cmp(cell, root->right->cell) < 0) {
        root->right = avl_right_rotate(root->right);
        return avl_left_rotate(root);
    }
    return root;
}

static AVLNode* min_value_node(AVLNode* node) {
    AVLNode *current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

AVLNode* avl_delete(AVLNode *root, struct Cell *cell, int (*cmp)(struct Cell*, struct Cell*)) {
    if (!root)
        return root;

    int comp = cmp(cell, root->cell);
    if (comp < 0)
        root->left = avl_delete(root->left, cell, cmp);
    else if (comp > 0)
        root->right = avl_delete(root->right, cell, cmp);
    else {
        // Node found.
        if (!root->left || !root->right) {
            AVLNode *temp = root->left ? root->left : root->right;
            if (!temp) {
                temp = root;
                root = NULL;
            } else {
                *root = *temp;
            }
            free(temp);
        } else {
            AVLNode *temp = min_value_node(root->right);
            root->cell = temp->cell;
            root->right = avl_delete(root->right, temp->cell, cmp);
        }
    }
    if (!root)
        return root;

    root->height = 1 + (avl_height(root->left) > avl_height(root->right) ? avl_height(root->left) : avl_height(root->right));
    int balance = avl_get_balance(root);

    // Left Left Case
    if (balance > 1 && avl_get_balance(root->left) >= 0)
        return avl_right_rotate(root);
    // Left Right Case
    if (balance > 1 && avl_get_balance(root->left) < 0) {
        root->left = avl_left_rotate(root->left);
        return avl_right_rotate(root);
    }
    // Right Right Case
    if (balance < -1 && avl_get_balance(root->right) <= 0)
        return avl_left_rotate(root);
    // Right Left Case
    if (balance < -1 && avl_get_balance(root->right) > 0) {
        root->right = avl_right_rotate(root->right);
        return avl_left_rotate(root);
    }
    return root;
}

AVLNode* avl_find(AVLNode *root, struct Cell *cell, int (*cmp)(struct Cell*, struct Cell*)) {
    if (!root)
        return NULL;
    int comp = cmp(cell, root->cell);
    if (comp == 0)
        return root;
    else if (comp < 0)
        return avl_find(root->left, cell, cmp);
    else
        return avl_find(root->right, cell, cmp);
}

void avl_traverse(AVLNode *root, void (*callback)(struct Cell*, void*), void *data) {
    if (root) {
        avl_traverse(root->left, callback, data);
        callback(root->cell, data);
        avl_traverse(root->right, callback, data);
    }
}

void avl_free(AVLNode *root) {
    if (root) {
        avl_free(root->left);
        avl_free(root->right);
        free(root);
    }
}
