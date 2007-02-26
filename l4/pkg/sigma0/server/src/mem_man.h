#ifndef SIGMA0_MEM_MAN_H__
#define SIGMA0_MEM_MAN_H__
#define DEBUG_AVL_TREE
#include <l4/cxx/avl_tree.h>

#include "page_alloc.h"
#include "region.h"


class Mem_man 
{
private:
  bool alloc_from(Region const *r2, Region const &r);

  static Mem_man _ram;

public:
  typedef cxx::Avl_tree_single< Region, cxx::Lt_functor<Region>, Slab_alloc> Tree;

private:
  Tree _tree;

public:
  static Mem_man *ram() { return &_ram; }

  unsigned long alloc(Region const &r);
  bool add_free(Region const &r);
  bool add(Region const &r);
  Region const *find(Region const &r) const;

  template< typename Op >
  Tree::Node_type const *rev_iterate(Tree::Node_type const *n, Op const &op);
  
  template< typename Op >
  Tree::Node_type const *iterate(Tree::Node_type const *n, Op const &op);

  bool morecore();
  unsigned long alloc_first(unsigned long size, unsigned owner = 2);

  void dump();
};

template< typename Op >
Mem_man::Tree::Node_type const *
Mem_man::rev_iterate(Tree::Node_type const *n, Op const &op)
{
  if (!n)
    return 0;

  Tree::Node_type const *r;

  if (n->right)
    if ((r = rev_iterate(n->right, op)))
      return r;
       
  if (op(n))
    return n;

  if (n->left)
    if ((r = rev_iterate(n->left, op)))
      return r;

  return 0;
}

template< typename Op >
Mem_man::Tree::Node_type const *
Mem_man::iterate(Tree::Node_type const *n, Op const &op)
{
  if (!n)
    return 0;

  Tree::Node_type const *r;

  if (n->left)
    if ((r = iterate(n->left, op)))
      return r;
       
  if (op(n))
    return n;

  if (n->right)
    if ((r = iterate(n->right, op)))
      return r;

  return 0;
}

#endif

