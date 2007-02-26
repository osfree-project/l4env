#ifndef AVL_TREE_H__
#define AVL_TREE_H__

#include <l4/cxx/std_alloc.h>
#include <l4/cxx/std_ops.h>
#include <l4/cxx/pair.h>

namespace cxx {

template< typename Key, class Compare = Lt_functor<Key>,
  template<typename A> class Alloc = New_allocator >
class Avl_tree_single
{
public:
  typedef Key Key_type;
  typedef Compare Key_compare;

  struct Node_type
  {
    Key_type  key;
    Node_type *left;
    Node_type *right;

    Node_type() : left(0), right(0) {}
    Node_type(Key_type const &key) 
      : key(key), left(0), right(0) {}
  };
  
private:
  class Node : public Node_type
  {
  public:
    using Node_type::left;
    using Node_type::right;
    
    unsigned short _flags;
    short _balance;
    
    Node() : _flags(0), _balance(0) 
    {}
    
    Node(Key_type const &key) 
      : Node_type(key), _flags(0), _balance(0)
    {}
    
    void adj_balance(Node *b, Node *c, int v = -1);
    void replace_child(Node_type *c, Node_type *repl);

    void r_rotate(Node *r)
    { left = r->right; r->right = this; }

    void l_rotate(Node *r)
    { right = r->left; r->left = this; }

    template< typename OS >
    void dump(OS &os, char const *prefix);

    Node *_left() { return static_cast<Node*>(left); }
    Node *_right() { return static_cast<Node*>(right); }
  };

public:
  typedef Alloc<Node> Node_allocator;

private:
  unsigned long _height;
  Node _head;

  Node_allocator _alloc;
  Key_compare const &_comp;

public:
  Avl_tree_single(Key_compare const &comp = Key_compare(),
      Node_allocator const &alloc = Node_allocator()) 
  : _height(0), _alloc(alloc), _comp(comp)
  {}
  
  ~Avl_tree_single();
  
  int insert(Key_type const &key);
  int remove(Key_type const &key);
  inline Node_type const *find(Key_type const &key) const;

  template< typename OS >
  void dump(OS &os);
  
public:
  Node_type const *root() const
  { return _head.right; }

};

template< typename Key, typename Data, 
  template<typename A> class Compare = Lt_functor,
  template<typename B> class Alloc = New_allocator >
class Avl_tree : public Avl_tree_single<Pair<Key, Data>, 
  Pair_compare< Compare<Key>, Pair<Key, Data> >,
  Alloc>
{
private:
  typedef Pair<Key, Data> Local_key_type;
  typedef Pair_compare< Compare<Key>, Local_key_type > Local_compare;
  typedef Avl_tree_single<Local_key_type, Local_compare, Alloc> Base_type;

public:
  typedef Compare<Key> Key_compare;
  typedef Key Key_type;
  typedef Data Data_type;
  typedef typename Base_type::Node_type Node_type;
  typedef typename Base_type::Node_allocator Node_allocator;

  Avl_tree(Key_compare const &comp = Key_compare(),
      Node_allocator const &alloc = Node_allocator())
    : Base_type(Local_compare(comp), alloc)
  {}
  
  int insert(Key_type key, Data_type const &data)
  { return Base_type::insert(Pair<Key_type, Data_type>(key, data)); }

  Node_type const *find(Key_type const &key) const
  { return Base_type::find(Local_key_type(key, Data_type())); }
  
  int remove(Key_type const &key)
  { return Base_type::remove(Local_key_type(key, Data_type())); }
   
  Data_type const &operator [] (Key_type key) const
  { return find(key)->key.s; }

  Data_type &operator [] (Key_type key)
  { return const_cast<Node_type*>(find(key))->key.s; }
};


/* Implementation of Avl_tree_single::Node */

/** Adjust balance factors */
template< typename Key, class Compare, template<typename A> class Alloc >
void Avl_tree_single<Key,Compare,Alloc>::Node::adj_balance(Node *b, Node *c, int v)
{
  if (_balance == 0)
    {
      b->_balance = 0;
      c->_balance = 0;
    }
  else if (_balance == v)
    {
      b->_balance = 0;
      c->_balance = -v;
    }
  else
    {
      b->_balance = v;
      c->_balance = 0;
    }
  _balance = 0;
}

/** Replace left or right child dependent on camparison. */
template< typename Key, class Compare, template< typename A > class Alloc >
void Avl_tree_single<Key,Compare,Alloc>::Node::replace_child(Node_type *c,
    Node_type *repl)
{
  if (right == c)
    right = repl;
  else
    left = repl;
}


/* Implementation of AVL Tree */

/** Insert new Node. */
template< typename Key, class Compare, template< typename A > class Alloc >
int Avl_tree_single<Key,Compare,Alloc>::insert(Key const &key)
{
  if (!_head.right)
    {
      /* empty tree */
      _head.right = _alloc.alloc();
      if (!_head.right)
	return -1;
      
      new (_head.right) Node(key);

      _height = 1;
    }
  else
    {
      /* non-empty tree */
      Node *p = _head._right(); /* search variable */
      Node *q = 0;            /* new node */
      Node *r;                /* search variable (balancing) */
      Node *s = _head._right(); /* node where rebalancing may occur */
      Node *t = &_head;       /* parent of s */
      int a;                  /* which subtree of s has grown -1 left ... */
      while (p)
	{
	  if (_comp(key, p->key))
	    {
	      /* move left */
	      q = p->_left();
	      if (!q)
		{
		  /* found insert position */
		  q = _alloc.alloc();
		  if (!q)
		    return -1;
		  new (q) Node(key);

		  p->left = q;
		  break;
		}
	    }
	  else if (_comp(p->key, key))
	    {
	      /* move right */
	      q = p->_right();
	      if (!q)
		{
		  /* found insert position */
		  q = _alloc.alloc();
		  if (!q)
		    return -1;
		  new (q) Node(key);
		  p->right = q;
		  break;
		}
	    }
	  else
	    /* key already exists */
	    return -2;

	  /* insert position not yet found, move on */
	  if (q->_balance)
	    {
	      t = p;
	      s = q;
	    }

	  p = q;
	}

      /* adj balanc factors */
      if (_comp(key, s->key))
	{
	  a = -1;
	  r = p = s->_left();
	}
      else
	{
	  a = +1;
	  r = p = s->_right();
	}

      /* adj balanc factors down to the new node */
      while (p != q)
	{
	  if (_comp(key, p->key))
	    {
	      p->_balance = -1;
	      p = p->_left();
	    }
	  else
	    {
	      p->_balance = +1;
	      p = p->_right();
	    }
	}

      /* balancing */
      if (!s->_balance)
	{
	  /* tree has grown higher */
	  s->_balance = a;
	  ++_height;
	  return 0;
	}

      if (s->_balance == -a)
	{
	  /* node s got balanced (shorter subtree of s has grown) */
	  s->_balance = 0;
	  return 0;
	}

      /* tree got out of balance */
      if (r->_balance == a)
	{
	  /* single rotation */
	  p = r;
	  if (a == -1)
	    s->r_rotate(r);
	  else
	    s->l_rotate(r);

	  /* set new balance factors */
	  s->_balance = 0;
	  r->_balance = 0;
	}
      else
	{
	  /* double rotation */
	  if (a == -1)
	    {
	      p = r->_right();
	      r->l_rotate(p);
	      s->r_rotate(p);
	    }
	  else
	    {
	      p = r->_left();
	      r->r_rotate(p);
	      s->l_rotate(p);
	    }

	  /* set new balance factors */
	  p->adj_balance(r, s, a);
	}

      /* set new subtree head */
      if (s == t->left)
	t->left = p;
      else
	t->right = p;
    }
  
  return 0;
}
  
template< typename Key, class Compare, template< typename A > class Alloc >
typename Avl_tree_single<Key,Compare,Alloc>::Node_type const *
Avl_tree_single<Key,Compare,Alloc>::find(Key_type const &key) const
{
  Node_type *q = _head.right;

  while (q)
    {
      if (_comp(key, q->key))
	q = q->left;
      else if (_comp(q->key, key))
	q = q->right;
      else
	return q;
    }

  return 0;
}


template< typename Key, class Compare, template< typename A > class Alloc >
int Avl_tree_single<Key,Compare,Alloc>::remove(Key_type const &key)
{
  Node *q = _head._right(); /* search variable */
  Node *p = &_head;       /* parent node of q */
  Node *r;                /* 'symetric predecessor' of q, subtree to rotate 
			   * while rebalancing */
  Node *s = &_head;       /* last ('deepest') node on the search path to q
			   * with balance 0, at this place the rebalancing
			   * stops in any case */
  Node *t = 0;            /* parent node of p */
  
  /***************************************************************************
   * search node with key == k, on the way down also search for the last
   * ('deepest') node with balance zero, this is the last node where
   * rebalancing might be necessary.
   ***************************************************************************/
  while (q)
    {
      if (!_comp(key, q->key) && !_comp(q->key, key))
	break; /* found */

      if (!q->_balance)
	s = q;

      t = p;
      p = q;

      if (_comp(key, p->key))
	q = q->_left();
      else if (_comp(p->key, key))
	q = q->_right();
      else
	return -3;
    }

  if (!q)
    return -3;

  /***************************************************************************
   * remove node
   ***************************************************************************/
  if (q->left && q->right)
    {
      /* replace q with its 'symetric predecessor' (the node with the largest
       * key in the left subtree of q, its the rightmost key in this subtree) */      if (!q->_balance)
        s = q;

      t = p;
      p = q;
      r = q->_left();

      while (r->right)
	{
	  if (r->_balance == 0)
	    s = r;

	  t = p;
	  p = r;
	  r = r->_right();
	}

      /* found, replace q */
      q->key = r->key;
      q = r;
    }

  if (q == _head.right)
    {
      /* remove tree head */
      if (!q->left && !q->right)
	{
	  /* got empty tree */
	  _head.right = 0;
	  _height = 0;
	}
      else
	{
	  if (q->right)
	    _head.right = q->right;
	  else
	    _head.right = q->left;

	  --_height;
	}
      q->~Node();
      _alloc.free(q);
    }
  else
    {
      /* q is not the tree head */
      int a;  /* changes of the balance factor, -1 right subtree got
               * shorter, +1 left subtree got shorter */
      
      if (q == p->right)
	{
	  if (q->left)
	    p->right = q->left;
	  else if (q->right)
	    p->right = q->right;
	  else
	    p->right = 0;

	  /* right subtree of p got shorter */
	  a = -1;
	}
      else
	{
	  if (q->left)
	    p->left = q->left;
	  else if (q->right)
	    p->left = q->right;
	  else
	    p->left = 0;

	  /* left subtree of p got shorter */
	  a = +1;
	}
      q->~Node();
      _alloc.free(q);
        
      /***********************************************************************
       * Rebalancing 
       * q is the removed node
       * p points to the node which must be rebalanced,
       * t points to the parent node of p
       * s points to the last node with balance 0 on the way down to p
       ***********************************************************************/
      
      Node *o;            /* subtree while rebalancing */
      bool done = false;  /* done with rebalancing */

      do 
	{
	  if (a == +1)
	    {
	      /***************************************************************
               * left subtree of p got shorter
               ***************************************************************/
	      if (p->_balance == -1)
		/* p got balanced => the whole tree with head p got shorter,
		 * must rebalance parent node of p (a.1.) */
		p->_balance = 0;
	      else if (p->_balance == 0)
		{
		  /* p got out of balance, but kept its overall height
		   * => done (a.2.) */
		  p->_balance = +1;
	          done = true;
		}
	      else
		{
		  /* left subtree of p was already shorter than right, 
		   * need to rotate */
		  r = p->_right();
		  
		  if (!r->_balance)
		    {
		      /* single rotation left and done (a.3.1) */
		      p->l_rotate(r);
		      p->_balance = +1;
		      r->_balance = -1;

		      t->replace_child(p, r);

		      done = 1;
		    }
		  else if (r->_balance == +1)
		    {
		      /* single rotation left and rebalance parent node 
		       * (a.3.2) */
		      p->l_rotate(r);
		      p->_balance = 0;
		      r->_balance = 0;

		      t->replace_child(p, r);

		      p = r;
		    }
		  else
		    {
		      /* double rotation right - left and rebalance parent 
		       * node (a.3.3) */
		      o = r->_left();
		      r->r_rotate(o);
		      p->l_rotate(o);
		      t->replace_child(p, o);
		      o->adj_balance(p, r);
		      p = o;
		    }
		}
		      
	    }
	  else /* a == -1 */
	    {
	      /***************************************************************
               * right subtree of p got shorter
               ***************************************************************/
	      if (p->_balance == +1)
		/* p got balanced,  => the whole tree with head p got 
		 * shorter, must rebalance parent node of p (b.1.) */
		p->_balance = 0;
	      else if (p->_balance == 0)
		{
		  /* p got out of balance, but kept its overall height 
		   * => done (b.2.) */
		  p->_balance = -1;
		  done = true;
		}
	      else
		{
		  /* the right subtree of p already was shorter than the left, 
		   * need to rotate to rebuild the AVL tree */
		  r = p->_left();

		  if (r->_balance == 0)
		    {
		      /* single rotation right and done (b.3.1) */
		      p->r_rotate(r);
		      r->_balance = +1;
		      p->_balance = -1;

		      t->replace_child(p, r);

		      done = true;
		    }
		  else if (r->_balance == -1)
		    {
		      /* single rotation right and rebalance parent
		       * node (b.3.2) */
		      p->r_rotate(r);
		      r->_balance = 0;
		      p->_balance = 0;

		      t->replace_child(p, r);
		      p = r;
		    }
		  else
		    {
		      /* double rotation left - right and rebalance parent
		       * node (b.3.3) */
		      o = r->_right();
		      r->l_rotate(o);
		      p->r_rotate(o);
		      t->replace_child(p, o);
		      o->adj_balance(r, p);
		      p = o;
		    }
		}
	    }
	  if (!done)
	    {
	      /* need to rebalance parent node, go one level up in the tree.
	       * Actually, we go down the tree until we find p and store its 
	       * parent and the parent of the parent. We start at the last 
	       * node with balance 0, because the rebalancing definitely 
	       * stops there. 
	       * Current situation: 
	       * t points to the node which must be rebalanced, p to the 
	       * subtree of t which got shorter */
	      q = p;
	      r = t;
	      if (t == s)
		{
		  if (s == &_head)
		    {
		      /* whole tree got shorter */
		      --_height;
		      done = true;
		    }
		  else
		    {
		      /* reached the last node which needs to be rebalanced */
		      p = s;
		      if (p->left == q)
			/* left subtree got shorter */
			a = +1;
		      else if (p->right == q)
			/* right subtree got shorter */
			a = -1;
		      else
			return -4;
		    }
		}
	      else
		{
		  /* search node q */
		  t = p = s;
		  while (p && (p != r))
		    {
		      t = p;
		      if (_comp(q->key, p->key))
			p = p->_left();
		      else
			p = p->_right();
		    }

		  if (p->left == q)
		    /* left subtree got shorter */
		    a = +1;
		  else if (p->right == q)
		    /* right subtree got shorter */
		    a = -1;
		  else
		    return -4;
		}
	    }
	}
      while (!done);
    }

  /* done */
  return 0;
}

template< typename Key, class Compare, template< typename A > class Alloc >
Avl_tree_single<Key,Compare,Alloc>::~Avl_tree_single()
{
  Node_type const *n;
  while ((n = root()))
    remove(n->key);
}

}

#if defined(DEBUG_AVL_TREE)
namespace cxx {

template< typename Key, class Compare, template< typename A > class Alloc >
template< typename OS >
void Avl_tree_single<Key,Compare,Alloc>::Node::dump(OS &os, char const *prefix)
{
  os << prefix << ' ' << Node_type::key << "; B" << _balance << '\n';
  char *c = const_cast<char*>(prefix);
  unsigned l;
  for (l = 0; c[l]; ++l)
    ;
  
  if (left)
    {
      c[l] = 'L';
      c[l+1] = 0;
      _left()->dump(os, c);
    }
  if (right)
    {
      c[l] = 'R';
      c[l+1] = 0;
      _right()->dump(os, c);
    }
  c[l] = 0;
}

template< typename Key, class Compare, template< typename A > class Alloc >
template< typename OS >
void Avl_tree_single<Key,Compare,Alloc>::dump(OS &os)
{
  if (_head.right)
    {
      char c[256];
      c[0] = '*';
      c[1] = 0;
      _head._right()->dump(os, c);
    }
  else
    os << "empty\n";
}


}

#endif

#endif
