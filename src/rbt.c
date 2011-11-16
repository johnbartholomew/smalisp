/* rbt.c, the implementation of the Reb-Black Tree data structure

  Copyright (C) 2002 Pouya Larjani

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Pouya Larjani
  pouya@rogers.com

  -----

  As according to point 2: THIS VERSION OF THE SOURCE CODE HAS BEEN ALTERED
  by John Bartholomew.

  The changes are the inclusion of "mem.h", and replacments of calls to
  malloc and free with calls to the macros X_MALLOC and X_FREE, in order to
  interface with the aforementioned memory checking utility functions.

*/

#include "global.h"

#include "rbt.h"

#define RBTN_BLACK  0
#define RBTN_RED    1
#define RBTN_NIL    (&rbtn_sentinel)


/* The sentinel node for the tree */
static rbtn_t rbtn_sentinel =
{
    NULL,       /* Parent */
    RBTN_NIL,   /* Left */
    RBTN_NIL,   /* Right */
    RBTN_BLACK, /* Color */
    0,          /* Key */
    NULL,       /* Data */
    NULL,       /* Extra */
};


/* Rotate the given node to left */
void rbtn_rotleft(rbtn_t **root, rbtn_t *x);
/* Rotate a given node to right */
void rbtn_rotright(rbtn_t **root, rbtn_t *x);
/* Fixup the tree after insertion of the new node */
void rbtn_insfix(rbtn_t **root, rbtn_t *x);
/* Fixup the tree after deletion of a node */
void rbtn_delfix(rbtn_t **root, rbtn_t *x);


/* Rotate the given node to left */
void rbtn_rotleft(rbtn_t **root, rbtn_t *x)
{
    rbtn_t *y = x->right;
    x->right = y->left;

    if (y->left != RBTN_NIL)
        y->left->parent = x;
    if (y != RBTN_NIL)
        y->parent = x->parent;
    if (x->parent)
    {
        if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
    }
    else
        *root = y;

    y->left = x;
    if (x != RBTN_NIL)
        x->parent = y;
}

/* Rotate a given node to right */
void rbtn_rotright(rbtn_t **root, rbtn_t *x)
{
    rbtn_t *y = x->left;
    x->left = y->right;

    if (y->right != RBTN_NIL)
        y->right->parent = x;
    if (y != RBTN_NIL)
        y->parent = x->parent;
    if (x->parent)
    {
        if (x == x->parent->right)
            x->parent->right = y;
        else
            x->parent->left = y;
    }
    else
        *root = y;

    y->right = x;
    if (x != RBTN_NIL)
        x->parent = y;
}

/* Fixup the tree after insertion of the new node */
void rbtn_insfix(rbtn_t **root, rbtn_t *x)
{
    rbtn_t *y;

    while ((x != *root) && (x->parent->color == RBTN_RED))
    {
        if (x->parent == x->parent->parent->left)
        {
            y = x->parent->parent->right;
            if (y->color == RBTN_RED)
            {
                x->parent->color = RBTN_BLACK;
                y->color = RBTN_BLACK;
                x->parent->parent->color = RBTN_RED;
                x = x->parent->parent;
            }
            else
            {
                if (x == x->parent->right)
                {
                    x = x->parent;
                    rbtn_rotleft(root, x);
                }

                x->parent->color = RBTN_BLACK;
                x->parent->parent->color = RBTN_RED;
                rbtn_rotright(root, x->parent->parent);
            }
        }
        else
        {
            y = x->parent->parent->left;
            if (y->color == RBTN_RED)
            {
                x->parent->color = RBTN_BLACK;
                y->color = RBTN_BLACK;
                x->parent->parent->color = RBTN_RED;
                x = x->parent->parent;
            }
            else
            {
                if (x == x->parent->left)
                {
                    x = x->parent;
                    rbtn_rotright(root, x);
                }

                x->parent->color = RBTN_BLACK;
                x->parent->parent->color = RBTN_RED;
                rbtn_rotleft(root, x->parent->parent);
            }
        }
    }

    (*root)->color = RBTN_BLACK;
}

/* Fixup the tree after deletion of a node */
void rbtn_delfix(rbtn_t **root, rbtn_t *x)
{
    rbtn_t *w;

    while ((x != *root) && (x->color == RBTN_BLACK))
    {
        if (x == x->parent->left)
        {
            w = x->parent->right;
            if (w->color == RBTN_RED)
            {
                w->color = RBTN_BLACK;
                x->parent->color = RBTN_RED;
                rbtn_rotleft(root, x->parent);
                w = x->parent->right;
            }

            if ((w->left->color == RBTN_BLACK) && (w->right->color == RBTN_BLACK))
            {
                w->color = RBTN_RED;
                x = x->parent;
            }
            else
            {
                if (w->right->color == RBTN_BLACK)
                {
                    w->left->color = RBTN_BLACK;
                    w->color = RBTN_RED;
                    rbtn_rotright(root, w);
                    w = x->parent->right;
                }

                w->color = x->parent->color;
                x->parent->color = RBTN_BLACK;
                w->right->color = RBTN_BLACK;
                rbtn_rotleft(root, x->parent);
                x = *root;
            }
        }
        else
        {
            w = x->parent->left;
            if (w->color == RBTN_RED)
            {
                w->color = RBTN_BLACK;
                x->parent->color = RBTN_RED;
                rbtn_rotright(root, x->parent);
                w = x->parent->left;
            }

            if ((w->right->color == RBTN_BLACK) && (w->left->color == RBTN_BLACK))
            {
                w->color = RBTN_RED;
                x = x->parent;
            }
            else
            {
                if (w->left->color == RBTN_BLACK)
                {
                    w->right->color = RBTN_BLACK;
                    w->color = RBTN_RED;
                    rbtn_rotleft(root, w);
                    w = x->parent->left;
                }

                w->color = x->parent->color;
                x->parent->color = RBTN_BLACK;
                w->left->color = RBTN_BLACK;
                rbtn_rotright(root, x->parent);
                x = *root;
            }
        }
    }

    x->color = RBTN_BLACK;
}

/* Find a node / Insert a new node */
rbtn_t *rbtn_findins(rbtn_t **root, int key, void* data, int force,
    int (*compar)(void*, void*), int sortbykey, int nofixup, int* added)
{
    rbtn_t *current, *parent, *x;
    int c;

    current = (*root == 0) ? RBTN_NIL : *root;
    parent = NULL;
    if (added)
        *added = 0;

    while (current != RBTN_NIL)
    {
        c = (sortbykey) ? (key - current->key) : compar(data, current->data);
        if (c == 0)
            return current;

        parent = current;
        current = (c < 0) ? (current->left) : (current->right);
    }

    if (!force)
        return NULL;

    x = (rbtn_t*)X_MALLOC(sizeof(rbtn_t));
    x->parent = parent;
    x->left = RBTN_NIL;
    x->right = RBTN_NIL;
    x->color = RBTN_RED;
    x->key = key;
    x->data = data;

    if (parent)
    {
        c = (sortbykey) ? (key - parent->key) : compar(data, parent->data);
        if (c < 0)
            parent->left = x;
        else
            parent->right = x;
    }
    else
        *root = x;

    if (!nofixup)
        rbtn_insfix(root, x);
    if (added)
        *added = 1;

    return x;
}

/* Delete a node from the tree */
int rbtn_del(rbtn_t **root, int key, void* data, int (*compar)(void*, void*),
    int sortbykey, int nofixup)
{
    rbtn_t *x, *y, *z;
    int c;
    char color;

    z = (*root == NULL) ? RBTN_NIL : *root;
    while (z != RBTN_NIL)
    {
        c = (sortbykey) ? (key - z->key) : compar(data, z->data);
        if (c == 0)
            break;
        else
            z = (c < 0) ? z->left : z->right;
    }

    if (z == RBTN_NIL)
        return 0;

    if ((z->left == RBTN_NIL) || (z->right == RBTN_NIL))
        y = z;
    else
    {
        y = z->right;
        while (y->left != RBTN_NIL)
            y = y->left;
    }

    if (y->left != RBTN_NIL)
        x = y->left;
    else
        x = y->right;

    x->parent = y->parent;
    if (y->parent)
    {
        if (y == y->parent->left)
            y->parent->left = x;
        else
            y->parent->right = x;
    }
    else
        *root = x;

    color = y->color;
    if (y != z)
    {
        y->left = z->left;
        y->right = z->right;
        y->parent = z->parent;

        if (z->parent)
        {
            if (z->parent->left == z)
                z->parent->left = y;
            else
                z->parent->right = y;
        }
        else
            *root = y;

        y->right->parent = y;
        y->left->parent = y;
        y->color = z->color;
    }

    X_FREE(z);

    if ((color == RBTN_BLACK) && (!nofixup))
        rbtn_delfix(root, x);

    return 1;
}

/* Traverse the tree bottom-up, left-right */
void rbtn_traverse(rbtn_t *root, void (*func)(void*))
{
    if (!root)
        return;

    if (root->left != RBTN_NIL)
        rbtn_traverse(root->left, func);

    if (root->right != RBTN_NIL)
        rbtn_traverse(root->right, func);

    if (root != RBTN_NIL)
        if (func)
            func(root);
}

/* Free all node data (note that this will not free data pointed to by nodes,
 *  only the nodes themselves
 */
void rbtn_free_all(rbtn_t *root)
{
	if (!root)
		return;

	if (root->left != RBTN_NIL)
		rbtn_free_all(root->left);

	if (root->right != RBTN_NIL)
		rbtn_free_all(root->right);

	if (root != RBTN_NIL)
		X_FREE(root);
}

