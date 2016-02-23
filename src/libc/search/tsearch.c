// Copyright (c) 2015 Nuxi, https://nuxi.nl/
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <search.h>
#include <stdlib.h>

#include "search_impl.h"

// Inserts an object into an AVL tree if not present.
//
// This algorithm is based on the non-recursive algorithm for
// AVL tree insertion by Neil Brown:
//
// http://neil.brown.name/blog/20041124101820
// http://neil.brown.name/blog/20041124141849
void *tsearch(const void *key, void **rootp,
              int (*compar)(const void *, const void *)) {
  // POSIX requires that tsearch() returns NULL if rootp is NULL.
  if (rootp == NULL)
    return NULL;
  struct __tnode *root = *rootp;

  // Find the leaf where the new key needs to be inserted. Return if
  // we've found an existing entry. Keep track of the path that is taken
  // to get to the node, as we will need it to adjust the balances.
  struct path path;
  path_init(&path);
  struct __tnode **base = &root, **leaf = &root;
  while (*leaf != NULL) {
    if ((*leaf)->__balance != 0) {
      // If we reach a node that has a non-zero balance on the way, we
      // know that we won't need to perform any rotations above this
      // point. In this case rotations are always capable of keeping the
      // subtree in balance. Make this the base node and reset the path.
      base = leaf;
      path_init(&path);
    }
    int cmp = compar(key, (*leaf)->__key);
    if (cmp < 0) {
      path_taking_left(&path);
      leaf = &(*leaf)->__left;
    } else if (cmp > 0) {
      path_taking_right(&path);
      leaf = &(*leaf)->__right;
    } else {
      return &(*leaf)->__key;
    }
  }

  // Did not find a matching key in the tree. Insert a new node.
  struct __tnode *result = *leaf = malloc(sizeof(**leaf));
  if (result == NULL)
    return NULL;
  result->__key = (void *)key;
  result->__left = NULL;
  result->__right = NULL;
  result->__balance = 0;

  // Walk along the same path a second time and adjust the balances.
  // Except for the first node, all of these nodes must have a balance
  // of zero, meaning that these nodes will not get out of balance.
  for (struct __tnode *n = *base; n != *leaf;) {
    if (path_took_left(&path)) {
      n->__balance += 1;
      n = n->__left;
    } else {
      n->__balance -= 1;
      n = n->__right;
    }
  }

  // Adjusting the balances may have pushed the balance of the base node
  // out of range. Perform a rotation to bring the balance back in range.
  struct __tnode *x = *base;
  if (x->__balance > 1) {
    struct __tnode *y = x->__left;
    if (y->__balance < 0) {
      // Left-right case.
      //
      //         x
      //        / \            z
      //       y   D          / \
      //      / \     -->    y   x
      //     A   z          /|   |\
      //        / \        A B   C D
      //       B   C
      struct __tnode *z = y->__right;
      y->__right = z->__left;
      z->__left = y;
      x->__left = z->__right;
      z->__right = x;
      *base = z;

      x->__balance = z->__balance > 0 ? -1 : 0;
      y->__balance = z->__balance < 0 ? 1 : 0;
      z->__balance = 0;
    } else {
      // Left-left case.
      //
      //        x           y
      //       / \         / \
      //      y   C  -->  A   x
      //     / \             / \
      //    A   B           B   C
      x->__left = y->__right;
      y->__right = x;
      *base = y;

      x->__balance = 0;
      y->__balance = 0;
    }
  } else if (x->__balance < -1) {
    struct __tnode *y = x->__right;
    if (y->__balance > 0) {
      // Right-left case.
      //
      //       x
      //      / \              z
      //     A   y            / \
      //        / \   -->    x   y
      //       z   D        /|   |\
      //      / \          A B   C D
      //     B   C
      struct __tnode *z = y->__left;
      x->__right = z->__left;
      z->__left = x;
      y->__left = z->__right;
      z->__right = y;
      *base = z;

      x->__balance = z->__balance < 0 ? 1 : 0;
      y->__balance = z->__balance > 0 ? -1 : 0;
      z->__balance = 0;
    } else {
      // Right-right case.
      //
      //       x               y
      //      / \             / \
      //     A   y    -->    x   C
      //        / \         / \
      //       B   C       A   B
      x->__right = y->__left;
      y->__left = x;
      *base = y;

      x->__balance = 0;
      y->__balance = 0;
    }
  }

  // Return the new entry.
  *rootp = root;
  return &result->__key;
}