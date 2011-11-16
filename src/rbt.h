/* vim: set ts=4 sts=4 sw=4 noet ai: */
/* rbt.h, the interface to the Reb-Black Tree data structure

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
*/

/* Include only once */
#ifndef _RBT_H_
#define _RBT_H_

/* Allow in C++ code */
#ifdef __cplusplus
extern "C" {
#endif


/* The primary type of the nodes in the tree */
typedef struct rbtn_s
{
    /* Parent, Left and Right pointers of each node */
    struct rbtn_s *parent, *left, *right;
    /* The color of the node, either red or black */
    char color;
    /* The key associated with the node */
    int key;
    /* The data stroed in the node. At least one of key or data must be
        specified for each node */
    void *data;
    /* Extra information, user-specific */
    void *extra;
} rbtn_t;


/* Node operations */

/* Search for a node / insert a new node. 'force' determines whether to insert
 *  the node in case it doesn't exist or not. 'compar' is the comparison
 *  function given if 'sortbykey' is off (i.e. sort by data, not keys).
 *  'nofixup' describes that the tree must not rebalance after each operation,
 *  this will immediately lose the red-black property and should not be mixed
 *  type. 'added' can be NULL, if not, then it will fill it with value 0 or 1
 *  depending on whether the node was inserted or not.
 *  Returns NULL on failure
 */
rbtn_t* rbtn_findins(rbtn_t **root, int key, void *data, int force,
    int (*compar)(void*, void*), int sortbykey, int nofixup, int *added);
/* Delete a node from the tree. The arguments acts as in the rbtn_findins
 *  function. Returns true on success
 */
int rbtn_del(rbtn_t **root, int key, void *data, int (*compar)(void*, void*),
    int sortbykey, int nofixup);
/* Traverse a tree bottom-up, left-to-right, and call a function on nodes */
void rbtn_traverse(rbtn_t *root, void (*func)(void*));

/* Free all node data (note that this will not free data pointed to by nodes,
 *  only the nodes themselves
 */
void rbtn_free_all(rbtn_t *root);


#ifdef __cplusplus
}
#endif

#endif /* !_RBT_H_ */
