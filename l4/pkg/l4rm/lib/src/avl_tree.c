/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/avl_tree.c
 * \brief  AVL tree implementation.
 *
 * \date   05/27/2000   
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>
#include <string.h>

/* L4 includes */
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

/* private includes */
#include "__avl_tree.h"
#include "__avl_tree_alloc.h"
#include "__debug.h"

/*****************************************************************************
 *** global structures
 *****************************************************************************/

/**
 * 'head' node of AVL tree, head.rigth points to the root node
 */
static avlt_t tree_head;

/**
 * total tree height
 */
static l4_uint32_t tree_height;

/*****************************************************************************/
/**
 * \brief Init AVL tree. 
 *
 * \retval 0 on success, -1 if tree node allocation setup failed
 */
/*****************************************************************************/ 
int 
avlt_init(void)
{
  int ret;

  /* init node allocation */
  ret = avlt_alloc_init();
  if (ret < 0)
    return -1;

  /* setup tree head */
  tree_head.left = tree_head.right = NULL;
  tree_height = 0;

  /* done */
  return 0;
}

/*****************************************************************************
 *** Insert
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Insert new element with key and data into AVL tree.
 * 
 * \param  key           new key
 * \param  data          new data
 *	
 * \return 0 on success (new element inserted), error code otherwise:
 *         - -#AVLT_NO_MEM  out of memory allocation tree node
 *         - -#AVLT_EXISTS  new key already exists in tree
 */
/*****************************************************************************/ 
int
avlt_insert(avlt_key_t key, avlt_data_t data)
{
  avlt_t * p;   /* search variable */
  avlt_t * q;   /* new node */
  avlt_t * r;   /* search variable (balancing) */
  avlt_t * s;   /* node where rebalancing may be necessary */
  avlt_t * t;   /* parent of s */
  int a;        /* indicates which subtree of s has grown, -1 left, +1 right */

  LOGdL(DEBUG_AVLT_INSERT, "key 0x"l4_addr_fmt"-0x"l4_addr_fmt"\n  data = %lu",
        key.start, key.end, (l4_addr_t)data);

  /* empty tree */
  if (tree_head.right == NULL)
    {
      /* add root node */
      q = avlt_new_node();
      if (q == NULL)
        return -AVLT_NO_MEM;

#if DEBUG_AVLT_INSERT
      LOG_printf("  new root node, 0x"l4_addr_fmt"\n", (l4_addr_t)q);
#endif

      /* setup node */
      q->key = key;
      q->data = data;
      q->b = 0;
      q->left = NULL;
      q->right = NULL;
      
      /* insert node */
      tree_head.right = q;
      tree_height = 1;
    }
  else
    {
      t = &tree_head;
      s = p = tree_head.right;
      q = NULL;

      while (p != NULL)
	{
	  if (AVLT_IS_LOWER(key, p->key))	  
	    {
	      /* move left */
	      q = p->left;
	      if (q == NULL)
		{
		  /* found insert position */
		  q = avlt_new_node();
		  if (q == NULL)
		    return -AVLT_NO_MEM;
		  p->left = q;
		  break;
		}
	    }
	  else /* key < p->key */
	    {
	      if (AVLT_IS_GREATER(key, p->key))
		{
		  /* move right */
		  q = p->right;
		  if (q == NULL)
		    {
		      /* found insert position */
		      q = avlt_new_node();
		      if (q == NULL)
			return -AVLT_NO_MEM;
		      p->right = q;
		      break;
		    }
		}		      
	      else
		{
		  /* key already exists */
#if DEBUG_AVLT_INSERT
		  LOG_printf("  key already exists\n");
#endif
		  return -AVLT_EXISTS;
		}
	    }
	     
	  /* insert position not yet found, move on */
	  if (q->b != 0)
	    {
	      /* q points to node where rebalancing may be necessary */
	      t = p;
	      s = q;
	    }
	  p = q;
	}

      /* inserted new node q, p is parent of q */
      ASSERT((p != NULL) && (q != NULL));

      /* setup new node */
      q->key = key;
      q->data = data;
      q->b = 0;
      q->left = NULL;
      q->right = NULL;
	      
      /* adjust balance factors */
      if (AVLT_IS_LOWER(key, s->key))
	{
	  a = -1;
	  r = p = s->left;
	}
      else
	{
	  a = +1;
	  r = p = s->right;
	}

      /* adjust balance factors down to the new node */
      while (p != q)
	{
	  if (AVLT_IS_LOWER(key, p->key))
	    {
	      p->b = -1;
	      p = p->left;
	    }
	  else
	    {
	      p->b = +1;
	      p = p->right;
	    }
	}
      
      /* balancing */
      if (s->b == 0)
	{
	  /* tree has grown higher */
	  s->b = a;
	  tree_height++;
	  return 0;
	}

      if (s->b == -a)
	{
	  /* node s got balanced (shorter subtree of s has grown) */
	  s->b = 0;
	  return 0;
	}

      /* tree got out of balance */
      if (r->b == a)
	{
	  /* single rotation */
	  p = r;
	  if (a == -1)
	    {
	      /* single rotation right */
#if DEBUG_AVLT_INSERT
	      LOG_printf("  single rotation to the right\n");
#endif
	      s->left = r->right;
	      r->right = s;
	    }
	  else
	    {
	      /* single rotation left */
#if DEBUG_AVLT_INSERT
	      LOG_printf("  single rotation to the left\n");
#endif
	      s->right = r->left;
	      r->left = s;
	    }

	  /* set new balance factors */
	  s->b = 0;
	  r->b = 0;
	}
      else
	{
	  /* double rotation */
	  if (a == -1)
	    {
	      /* left-right rotation */
#if DEBUG_AVLT_INSERT
	      LOG_printf("  double rotation left-right\n");
#endif
	      p = r->right;
	      r->right = p->left;
              p->left = r;
	      s->left = p->right;
	      p->right = s;
	    }
	  else
	    {
	      /* right-left rotation */
#if DEBUG_AVLT_INSERT
	      LOG_printf("  double rotation rigth-left\n");
#endif
	      p = r->left;
	      r->left = p->right;
	      p->right = r;
	      s->right = p->left;
	      p->left = s;
	    }

	  /* set new balance factors */
	  if (p->b == a)
	    {
	      s->b = -a;
	      r->b = 0;
	    }
	  else if (p->b == 0)
	    {
	      s->b = 0;
	      r->b = 0;
	    }
	  else /* p->b == -a */
	    {
	      s->b = 0;
	      r->b = a;
	    }
	  p->b = 0;
	}

      /* set new subtree head */
      if (s == t->left)
	t->left = p;
      else
	t->right = p;
    }
			    
  /* done */
  return 0;
}

/*****************************************************************************
 *** Remove
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Remove node with key k from tree
 * 
 * \param  key           key to remove
 *	
 * \return 0 on success (node removed), error code otherwise:
 *         - -#AVLT_NOT_FOUND  key not found
 *         - -#AVLT_INVAL      corrupted AVL tree
 */
/*****************************************************************************/ 
int 
avlt_remove(avlt_key_t key)
{
  avlt_t * q;    /* search variable */
  avlt_t * p;    /* parent node of q */
  avlt_t * r;    /* 'symetric predecessor' of q, subtree to rotate while 
		  * rebalancing */
  avlt_t * o;    /* subtree while rebalancing */
  avlt_t * s;    /* last ('deepest') node on the search path to q with 
		  * balance 0, at this place the rebalancing stops in any 
		  * case */
  avlt_t * t;    /* parent node of p */
  int a;         /* changes of the balance factor, -1 right subtree got
		  * shorter, +1 left subtree got shorter */
  int done;      /* done with rebalancing */

  /***************************************************************************
   * search node with key == k, on the way down also search for the last
   * ('deepest') node with balance zero, this is the last node where
   * rebalancing might be necessary.
   ***************************************************************************/
  q = tree_head.right;
  p = &tree_head;
  s = &tree_head;
  t = NULL;
  while (q)
    {
      if (AVLT_IS_EQUAL(key, q->key))
	/* found */
	break;
      
      /* balance 0? */
      if (q->b == 0)
	s = q;

      t = p;
      p = q;
      if (AVLT_IS_LOWER(key, q->key))
	q = q->left;
      else if (AVLT_IS_GREATER(key, q->key))
	q = q->right;
      else
	{
	  LOG_Error("AVL remove: invalid key range!");
	  return -AVLT_NOT_FOUND;
	}
    }

  if (q == NULL)
    {
      LOG_Error("AVL remove: key not found!");
      return -AVLT_NOT_FOUND;
    }


  /***************************************************************************
   * remove node
   ***************************************************************************/
  if ((q->left != NULL) && (q->right != NULL))
    {
      /* replace q with its 'symetric predecessor' (the node with the largest
       * key in the left subtree of q, its the rightmost key in this subtree) */
      if (q->b == 0)
	s = q;

      t = p;
      p = q;
      r = q->left;
      while (r->right)
	{
	  if (r->b == 0)
	    s = r;

	  t = p;
	  p = r;
	  r = r->right;
	}
 
      /* found, replace q */
      LOGdL(DEBUG_AVLT_REMOVE, "replace\n" \
            "  key 0x"l4_addr_fmt"-0x"l4_addr_fmt" with 0x"l4_addr_fmt
	    "-0x"l4_addr_fmt, 
            q->key.start, q->key.end, r->key.start, r->key.end);
      
      q->key = r->key;
      q->data = r->data;
      q = r;

    }

  /* at this point at least one subtree of q must be NULL */
  Assert((q->left == NULL) || (q->right == NULL));

  if (q == tree_head.right)
    {
      /* remove tree head */
#if DEBUG_AVLT_REMOVE
      LOG_printf("  remove tree head\n");
#endif
      if ((q->left == NULL) && (q->right == NULL))
	{
	  /* got empty tree */
#if DEBUG_AVLT_REMOVE
	  LOG_printf("  empty tree\n");
#endif
	  tree_head.right = NULL;
	  tree_height = 0;
	}
      else 
	{
#if DEBUG_AVLT_REMOVE
	  LOG_printf("  new tree head\n");
#endif
	  if (q->right != NULL)
	    tree_head.right = q->right;
	  else
	    tree_head.right = q->left;
	  tree_height--;
	}

      /* release q */
      avlt_free_node(q);
    }
  else
    {
      /* q is not the tree head */
      Assert(t != NULL);

      if (q == p->right)
	{
#if DEBUG_AVLT_REMOVE
	  LOG_printf("  remove right successor of key 0x"l4_addr_fmt
	         "-0x"l4_addr_fmt" (%d)\n",
                 p->key.start, p->key.end, avlt_node_index(q));
#endif
	  if (q->left != NULL)
	    p->right = q->left;
	  else if (q->right != NULL)
	    p->right = q->right;
	  else
	    p->right = NULL;

	  /* right subtree of p got shorter */
	  a = -1;
	}
      else
	{
#if DEBUG_AVLT_REMOVE
	  LOG_printf("  remove left successor of key 0x"l4_addr_fmt
	         "-0x"l4_addr_fmt" (%d)\n",
                 p->key.start, p->key.end, avlt_node_index(q));
#endif
	  if (q->left != NULL)
	    p->left = q->left;
	  else if (q->right != NULL)
	    p->left = q->right;
	  else
	    p->left = NULL;

	  /* left subtree of p got shorter */
	  a = +1;
	}

      /* release q */
      avlt_free_node(q);

      /***********************************************************************
       * Rebalancing 
       * q is the removed node
       * p points to the node which must be rebalanced,
       * t points to the parent node of p
       * s points to the last node with balance 0 on the way down to p
       ***********************************************************************/
#if DEBUG_AVLT_REMOVE
      LOG_printf("  removed node (q): %d\n", avlt_node_index(q));
      LOG_printf("  parent (p): %d\n", avlt_node_index(p));
      if (t != &tree_head)
	LOG_printf("  p's parent (t): %d\n", avlt_node_index(t));
      else
	LOG_printf("  p's parent (t): head\n");
      if (s != &tree_head)
	LOG_printf("  last balanced node (s): %d\n", avlt_node_index(s));
      else
	LOG_printf("  last balanced node (s): head\n");
#endif 
      done = 0;
      do 
	{
#if DEBUG_AVLT_REMOVE
	  LOG_printf("  rebalance %d, balance %d, change %d\n",
                 avlt_node_index(p), p->b, a);
#endif
	  if (a == +1)
	    {
	      /***************************************************************
               * left subtree of p got shorter
               ***************************************************************/
#if DEBUG_AVLT_REMOVE
	      LOG_printf("  left subtree got shorter\n");
#endif
	      if (p->b == -1)
		{
		  /* p got balanced => the whole tree with head p got shorter,
		   * must rebalance parent node of p (a.1.) */
#if DEBUG_AVLT_REMOVE
		  LOG_printf("  new balance 0, rebalance parent\n");
#endif
		  p->b = 0;
		}
	      else if (p->b == 0)
		{
		  /* p got out of balance, but kept its overall height
		   * => done (a.2.) */
#if DEBUG_AVLT_REMOVE
		  LOG_printf("  new balance +1, done\n");
#endif
		  p->b = +1;
		  done = 1;
		}
	      else /* p->b == +1 */
		{
		  /* left subtree of p was already shorter than right, 
		   * need to rotate */
		  r = p->right;
		  
		  if (r->b == 0)
		    {
		      /* single rotation left and done (a.3.1) */
#if DEBUG_AVLT_REMOVE
		      LOG_printf("  single rotation left, done\n");
#endif
		      p->right = r->left;
		      r->left = p;
		      p->b = +1;
		      r->b = -1;
		      if (t->right == p)
			t->right = r;
		      else
			t->left = r; 
		      done = 1;
		    }
		  else if (r->b == +1)
		    {
		      /* single rotation left and rebalance parent node 
		       * (a.3.2) */
#if DEBUG_AVLT_REMOVE
		      LOG_printf("  single rotation left, rebalance parent\n");
#endif
		      p->right = r->left;
		      r->left = p;
		      p->b = 0;
		      r->b = 0;
		      if (t->right == p)
			t->right = r;
		      else
			t->left = r;
		      p = r;
		    }
		  else /* r->b == -1 */
		    {
		      /* double rotation right - left and rebalance parent 
		       * node (a.3.3) */
#if DEBUG_AVLT_REMOVE
		      LOG_printf("  double rotation right - left\n");
#endif
		      o = r->left;
		      
		      /* right rotation o/r */
		      r->left = o->right;
		      o->right = r;

                      /* left rotate p/o */
		      p->right = o->left;
		      o->left = p;
		      if (t->right == p)
			t->right = o;
		      else
			t->left = o;

		      /* adjust balance factors */
		      if (o->b == 0)
			{
			  p->b = 0;
			  r->b = 0;
			}
		      else if (o->b == -1)
			{
			  p->b = 0;
			  r->b = +1;
			}
		      else
			{
			  p->b = -1;
			  r->b = 0;
			}
		      o->b = 0;			
		      p = o;
		    }
		}
	    }
	  else /* a == -1 */
	    {
	      /***************************************************************
               * right subtree of p got shorter
               ***************************************************************/
#if DEBUG_AVLT_REMOVE
	      LOG_printf("  right subtree got shorter\n");
#endif
	      if (p->b == +1)
		{
		  /* p got balanced,  => the whole tree with head p got 
		   * shorter, must rebalance parent node of p (b.1.) */
#if DEBUG_AVLT_REMOVE
		  LOG_printf("  new balance 0, rebalance parent\n");
#endif
		  p->b = 0;
		}
	      else if (p->b == 0)
		{
		  /* p got out of balance, but kept its overall height 
		   * => done (b.2.) */
#if DEBUG_AVLT_REMOVE
		  LOG_printf("  new balance -1, done\n");
#endif
		  p->b = -1;
		  done = 1;
		}
	      else /* p->b == -1 */
		{
		  /* the right subtree of p already was shorter than the left, 
		   * need to rotate to rebuild the AVL tree */
		  r = p->left;
		  
		  if (r->b == 0)
		    {
		      /* single rotation right and done (b.3.1) */
#if DEBUG_AVLT_REMOVE
		      LOG_printf("  single rotation right, done\n");
#endif
		      p->left = r->right;
		      r->right = p;
		      r->b = +1;
		      p->b = -1;
		      if (t->right == p)
			t->right = r;
		      else
			t->left = r;
		      done = 1;		   
		    }
		  else if (r->b == -1)
		    {
		      /* single rotation right and rebalance parent 
		       * node (b.3.2) */
#if DEBUG_AVLT_REMOVE
		      LOG_printf("  single rotation right, rebalance parent\n");
#endif
		      p->left = r->right;
		      r->right = p;
		      r->b = 0;
		      p->b = 0;
		      if (t->right == p)
			t->right = r;
		      else
			t->left = r;
		      p = r;
		    }
		  else /* r->b == +1 */
		    {
		      /* double rotation left - right and rebalance parent
		       * node (b.3.3) */
#if DEBUG_AVLT_REMOVE
		      LOG_printf("  double rotation left - right\n");
#endif
		      o = r->right;
		      
		      /* left rotate o/r */
		      r->right = o->left;
		      o->left = r;

		      /* right rotate o/p */
		      p->left = o->right;
		      o->right = p;
		      if (t->right == p)
			t->right = o;
		      else
			t->left = o;

		      /* adjust balance factors */
		      if (o->b == 0)
			{
			  r->b = 0;
			  p->b = 0;
			}
		      else if (o->b == -1)
			{
			  r->b = 0;
			  p->b = +1;
			}
		      else /* o->b == +1 */
			{
			  r->b = -1;
			  p->b = 0;
			}
		      o->b = 0;
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
		  if (s == &tree_head)
		    {
		      /* whole tree got shorter */
#if DEBUG_AVLT_REMOVE
		      LOG_printf("  whole tree got shorter\n");
#endif
		      tree_height--;
		      done = 1;
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
			{
			  Panic("corrupted AVL tree!");
			  return -AVLT_INVAL;
			}
		      
		      Assert(p->b == 0);
		    }
		}
	      else
		{
		  /* search node q */
		  t = p = s;

		  if (s == &tree_head)
		    p = s->right;

		  while (p && (p != r)) /* r points to t of the prev. round */
		    {
		      t = p;
		      if (AVLT_IS_LOWER(q->key, p->key))
			p = p->left;
		      else
			p = p->right;		       
		    }
		  Assert(p);
		  
		  if (p->left == q)
		    /* left subtree got shorter */
		    a = +1;
		  else if (p->right == q)
		    /* right subtree got shorter */
		    a = -1;
		  else
		    {
		      Panic("corrupted AVL tree!");
		      return -AVLT_INVAL;
		    }

		  /* now p is the node which needs to be rebalanced, t its 
		   * parent node */
		  Assert(t != p);
		}  		  
	    }
	}
      while (!done);
    }

  /* done */
  return 0;
}

/*****************************************************************************
 *** Find
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Search node with key key in tree.
 * 
 * \param  key           search key 
 * \retval data          data of node if key found
 *	
 * \return 0 on succes (found key, \a data contains data of node), error
 *         code otherwise:
 *         - -#AVLT_NOT_FOUND  key not found
 */
/*****************************************************************************/ 
int
avlt_find(avlt_key_t key, 
	  avlt_data_t * data)
{
  avlt_t * q;   /* search node */

  /* search node */
  q = tree_head.right;
  while (q)
    {
      if (AVLT_IS_IN(key,q->key))
	/* found */
	break;
      
      if (AVLT_IS_LOWER(key,q->key))
	q = q->left;
      else if (AVLT_IS_GREATER(key,q->key))
	q = q->right;
      else
	/* overlapping key range */
	return -AVLT_NOT_FOUND;
    }

  if (q == NULL)
    /* not found */
    return -AVLT_NOT_FOUND;
  else
    {
      /* found */
      *data = q->data;
      return 0;
    }
}

/*****************************************************************************
 *** DEBUG stuff
 *****************************************************************************/
#if DO_DEBUG_AVLT

#define SHOW_LONG        0

#define SHOW_WIDTH       210  
#define SHOW_DEPTH       10

#if SHOW_LONG
#define SHOW_NODE_WIDTH  18
#else
#define SHOW_NODE_WIDTH   6
#endif 

/* the tree presentation is build in a local buffer first */
static char __tree[2 * SHOW_DEPTH][SHOW_WIDTH];

/*****************************************************************************/
/**
 * \brief Build tree presentation.
 */
/*****************************************************************************/ 
static void 
__avlt_build_tree(void)
{
  int num_nodes = (1 << tree_height) - 1;
  int level,row,pos,bits,w;
  l4_umword_t i;
  char str[32];
  avlt_t *p;
#if SHOW_LONG
  char left[16],right[16];
#endif
  
  LOGdL(DEBUG_AVLT_SHOW, "num_nodes = %d", num_nodes);
  
  i = 1;
  while (i <= num_nodes)
    {
      level = l4util_log2(i);

      LOGdL(DEBUG_AVLT_SHOW, "i = %ld, level = %d", i, level);

      bits = level - 1;
      p = tree_head.right;
      while ((bits >= 0) && (p != NULL))
	{
	  if (l4util_test_bit(bits, &i))
	    p = p->right;
	  else
	    p = p->left;

	  bits--;
	}

      if (bits >= 0)
	{
	  /* node does not exist */
	  i++;
	  continue;
	}

      if (p != NULL)
	{
#if 0
	  pos = (i - (1 << level) + 1) * (SHOW_WIDTH / ((1 << level) + 1)) -
            (SHOW_NODE_WIDTH / 2);
#else
	  w = SHOW_WIDTH / (1 << level);
	  pos = (i - (1 << level)) * w + (w / 2) - (SHOW_NODE_WIDTH / 2);
#endif
	  row = 2 * (level + 1);

	  LOGdL(DEBUG_AVLT_SHOW, "node at 0x"l4_addr_fmt", level = %d\n" \
                "  0x"l4_addr_fmt" - 0x"l4_addr_fmt": 0x"l4_addr_fmt"",
		(l4_addr_t)p, level, p->key.start, p->key.end,
		(l4_addr_t)p->data);
	  LOGdL(DEBUG_AVLT_SHOW,"row = %d, pos = %d",row,pos);

#if SHOW_LONG
	  if (p->left != NULL)
	    sprintf(left, "%2d", avlt_node_index(p->left));
	  else
	    sprintf(left, "xx");

	  if (p->right != NULL)
	    sprintf(right, "%2d", avlt_node_index(p->right));
	  else
	    sprintf(right, "xx");

	  sprintf(str, " %2x,%2d,%2u,%2s,%2s", avlt_node_index(p), p->b,
		  p->data, left, right);
	  memcpy(&__tree[row][pos], str, strlen(str));
	  sprintf(str, l4_addr_fmt"-"l4_addr_fmt"", p->key.start, p->key.end);
	  memcpy(&__tree[row + 1][pos], str, strlen(str));
#else
	  sprintf(str, "%2ld,%2d", avlt_node_index(p), p->b);
	  memcpy(&__tree[row][pos], str, strlen(str));
	  sprintf(str, "%2lu-%2lu", p->key.start, p->key.end);
	  memcpy(&__tree[row + 1][pos], str, strlen(str));
#endif
	}

      /* next node */
      i++;
    }
}

/*****************************************************************************/
/**
 * \brief Show tree (currently broken???).
 *
 * Warning: due to the changes in the tree node allocation and region 
 *          descriptor handling the shown indices for the nodes / regions
 *          are just the addresses of the descriptors, not longer the indices
 *          in the node / region tables.
 */
/*****************************************************************************/ 
void
avlt_show_tree(void)
{
  char root[16];
  int i;

  LOGL("avlt_show_tree: broken\n");
  return;

  /* init buffer */
  memset(__tree, ' ', 2 * SHOW_DEPTH * SHOW_WIDTH);

  /* head node */
  if (tree_head.right != NULL)
    sprintf(root, "%2ld", avlt_node_index(tree_head.right));
  else 
    sprintf(root, "xx");

  sprintf(&__tree[0][SHOW_WIDTH / 2 - 9], "height %2u, root %2s",
	  tree_height, root);

  /* build tree */
  if (tree_head.right != NULL)
    __avlt_build_tree();

  /* show tree */
  LOG_printf("\n\n");
  for (i = 0; i < 2 * (tree_height + 1); i++)
    {
      __tree[i][SHOW_WIDTH - 1] = 0;
      LOG_printf("%s\n", __tree[i]);
    }

  LOG_printf("\n");

#if 0
  KDEBUG("AVL tree");
#endif
}

/*****************************************************************************/
/**
 * \brief Insert new node.
 * 
 * \param  start         Key start
 * \param  end           Key end
 * \param  data          Node data
 *	
 * \return 0 on success (new element inserted), error code otherwise:
 *         - \c -AVLT_NO_MEM  out of memory allocation tree node
 *         - \c -AVLT_EXISTS  new key already exists in tree
 */
/*****************************************************************************/ 
int
AVLT_insert(l4_addr_t start, l4_addr_t end, l4_addr_t data)
{
  avlt_key_t key;

  key.start = start;
  key.end = end;

  return avlt_insert(key, (void *)data);
}

/*****************************************************************************/
/**
 * \brief Remove node.
 * 
 * \param  start         Key start
 * \param  end           Key end
 *	
 * \return 0 on success (node removed), error code otherwise:
 *         - \c -AVLT_NOT_FOUND  key not found
 *         - \c -AVLT_INVAL      corrupted AVL tree
 */
/*****************************************************************************/ 
int
AVLT_remove(l4_addr_t start, l4_addr_t end)
{
  avlt_key_t key;

  key.start = start;
  key.end = end;

  return avlt_remove(key);
}

/*****************************************************************************/
/**
 * \brief Search key.
 * 
 * \param  start         Key start
 * \param  end           Key end
 *	
 * \return 0 on succes, error code otherwise:
 *         - \c -AVLT_NOT_FOUND  key not found
 */
/*****************************************************************************/ 
int 
AVLT_find(l4_addr_t start, l4_addr_t end)
{
  avlt_key_t key;
  avlt_data_t data;

  key.start = start;
  key.end = end;

  return avlt_find(key, &data);
}

#else /* DO_DEBUG_AVLT */

void
avlt_show_tree(void)
{
  /* do nothing */
}

#endif
