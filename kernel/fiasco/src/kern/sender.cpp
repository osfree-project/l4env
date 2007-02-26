INTERFACE:

#include "l4_types.h"

class Receiver;

/** A sender.  This is a role class, so real senders need to inherit from it.
 */
class Sender
{
public:
  /** Receiver-ready callback.  Receivers make sure to call this
      function on waiting senders when they get ready to receive a
      message from that sender.  Senders need to overwrite this interface. */
  virtual bool ipc_receiver_ready(Receiver *) = 0;


protected:
  Receiver *  _receiver;

private:
  Global_id   _id;
  Sender *sender_next, *sender_prev;

  friend class Jdb;
  friend class Jdb_thread_list;

  // enqueing priority
  unsigned short _sender_prio;
  
  Sender *_sender_l, *_sender_r;

  /* 0 head,
     -1 non tree elem, only included in some double linked list,
     !0 && !-1 tree elem */
  Sender *_sender_parent;
};


IMPLEMENTATION:

#include <cassert>

#include "cpu_lock.h"
#include "lock_guard.h"

//
// state requests/manipulation
//

/** Optimized constructor.  This constructor assumes that the object storage
    is zero-initialized.
    @param id user-visible thread ID of the sender
    @param ignored an integer argument.  The value doesn't matter and 
                   is used just to distinguish this constructor from the 
		   default one.
 */
PROTECTED inline
explicit
Sender::Sender (const Global_id& id, int /*ignored*/)
      : _receiver (0),
        _id (id)
{}

/** Sender ID.
    @return user-visible thread ID of the sender
 */
PUBLIC inline 
Global_id
Sender::id() const
{
  return _id;
}

/** Current receiver.
    @return receiver this sender is currently trying to send a message to.
 */
PUBLIC inline 
Receiver *
Sender::receiver() const
{
  return _receiver;
}

/** Set current receiver.
    @param receiver the receiver we're going to send a message to
 */
PROTECTED inline
void
Sender::set_receiver (Receiver* receiver)
{
  _receiver = receiver;
}

PUBLIC inline
unsigned short Sender::sender_prio()
{
  return _sender_prio;
}

/** Sender in a queue of senders?.  
    @return true if sender has enqueued in a receiver's list of waiting 
            senders
 */
PUBLIC inline 
bool 
Sender::in_sender_list()
{
  return sender_next;
}

PRIVATE inline
void Sender::replace_node(Sender *node)
{
  _sender_l = node->_sender_l;
  _sender_r = node->_sender_r;

  // update parent pointer of the childs
  if(_sender_l)
    _sender_l->_sender_parent = this;

  if(_sender_r)
    _sender_r->_sender_parent = this;

  _sender_parent = node->_sender_parent;
}

PRIVATE inline
bool Sender::is_tree_elem()
{
  // we using the prev pointer for tree purposes
  // 0 if head
  // 1 if not tree elem but inside the linked list
  // parent pointer

  return ((Sender*)1L != _sender_parent);
}


PRIVATE inline
bool Sender::empty_list()
{
  assert(sender_next);
  return (sender_next == this);
}



PRIVATE inline
void Sender::list_enqueue(Sender *first)
{
  assert(first);

  // mark as non tree elem
  _sender_parent = (Sender*) 1L;

  sender_next = first;
  sender_prev = sender_next->sender_prev;
  sender_prev->sender_next = this;
  sender_next->sender_prev = this;
}

PRIVATE inline
void Sender::list_dequeue()
{
  sender_prev->sender_next = sender_next;
  sender_next->sender_prev = sender_prev;
}

PRIVATE inline NEEDS [<cassert>, Sender::list_enqueue]
void Sender::tree_insert(Sender *head)
{

  Sender *p = head;
  Sender *x = p;

  assert(p);

  int b = 7;
  while(x)
    {

      // already an elemen with the same prio
      if(x->_sender_prio == _sender_prio)
        {
          list_enqueue(x); // enqueue in the double linked list from x
          return;
        }

      p = x;
      x = ((_sender_prio >> b--) & 0x1) ? x->_sender_r : x->_sender_l;

      assert(b>=-1);
      assert(b<=7);
    }

  _sender_l = _sender_r = 0;
  _sender_parent = p;

  if((_sender_prio >> (b+1)) & 0x1)
    p->_sender_r = this;
  else

    p->_sender_l = this;
}


PRIVATE inline NEEDS [<cassert>, Sender::empty_list, Sender::is_tree_elem,
                      Sender::replace_node, Sender::list_dequeue]
Sender *Sender::remove_head()
{

  assert(_sender_parent == 0);

  assert(is_tree_elem());

  // check if we have an sibling with the same prio
  if(!empty_list())
    {
      assert(sender_next && (sender_next !=this));

      sender_next->replace_node(this);
      list_dequeue();
      return sender_next;
    }

  if(!_sender_l && !_sender_r)
    return 0;

  assert(_sender_r || _sender_l);

  Sender **leaf_parent = 0;
  Sender **parent_max = 0;

  Sender *max_s = 0;
  int max_prio=0;

  Sender *x= this; //start at head

  // search for an empty leaf node
  // by the walking down the way, get the new maximum prio too
  for(bool loop = true; loop;)
    {

      // try right first
      if(x->_sender_r)
        {
          leaf_parent = &x->_sender_r;
          x = x->_sender_r;
        }
      else if (x->_sender_l)
        {
          leaf_parent = &x->_sender_l;
          x = x->_sender_l;
        }
      else  // ok we have an leaf now, but we need to do the maximum test too
        loop = false;

      assert(x);

      if(x->_sender_prio < max_prio)
        continue;

      max_prio = x->_sender_prio;
      max_s = x;
      parent_max = leaf_parent;
    }

  assert(x && max_s);

  assert(leaf_parent);
  assert(x!=this);
  assert(!x->_sender_r && !x->_sender_l);

  // clearing the old link to x
  // we do this before everything else, to avoid cyclic
  // references, if max_s is the parent of x
  *leaf_parent = 0;

  // max_s points to the tree elem with the new highest prio
  // x ist just an leaf node
  // the linked lists in both nodes is untouched here
  if(x != max_s)
    {
      // we first replacing the new maximum node with the leaf node x
      // then we can replace the old head with max_s
      assert(x->_sender_prio < max_s->_sender_prio);

      // replace the new maximum elem with the leaf node
      x->replace_node(max_s);

      *parent_max=x; // update the parent of max_s to point to x
      // ok make the new highest prio sender now top
      max_s->replace_node(this);
    }
  else
    // max_s & x are the same, simply replace head
    x->replace_node(this);

  return max_s;
}

PRIVATE inline NEEDS [<cassert>, Sender::empty_list, Sender::is_tree_elem,
                      Sender::replace_node, Sender::list_dequeue]
void Sender::remove_tree_elem()
{

  if(!is_tree_elem())
    {
      list_dequeue();
      return;
    }

  assert(_sender_parent);
  assert((_sender_parent->_sender_l == this)
         || (_sender_parent->_sender_r == this));

  Sender **p = (_sender_parent->_sender_l == this) ?
    &_sender_parent->_sender_l : &_sender_parent->_sender_r;

  if(!empty_list())
    {
      *p = sender_next;
      sender_next->replace_node(this); // copy the necessary stuff to next
      list_dequeue();
      return;
    }

  if(!_sender_l && !_sender_r)
    {
      *p = 0;
      return;
    }

  Sender *x= this;
  assert(_sender_l || _sender_r);
  Sender **leaf_parent = 0;

  // we always loop through the whole tree to get the next maximum
  // now find an leaf node, so we can replace this with the empty leafnode
  for(;;)
    {

    // always try right first
    if(x->_sender_r)
      {
        leaf_parent = &x->_sender_r;
        x = x->_sender_r;
      }
    else if(x->_sender_l)
      {
        leaf_parent = &x->_sender_l;
        x = x->_sender_l;
      }
    else
      break;
    }

  assert(x);
  assert(leaf_parent);
  assert(x!=this);
  assert(!x->_sender_r && !x->_sender_l);

  // clearing the old link to x
  // should be done before copying the rest
  *leaf_parent = 0;
  x->replace_node(this);

  *p = x;
}

PROTECTED
//PROTECTED inline NEEDS [<cassert>, "cpu_lock.h", "lock_guard.h",
//                      Sender::replace_node, Sender::tree_insert]
void Sender::sender_enqueue(Sender **head, unsigned short prio)
{
  assert(prio <256);

  _sender_prio = prio;

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  if (in_sender_list())
    return;

  _sender_l = _sender_r = 0;
  _sender_parent = 0;
  sender_next = sender_prev = this;

  Sender *p = *head;

  if(!p)
    {
      *head = this;
      return;
    }

  Sender *x = this;

  // bigger prio, replace top element
  // we dont handle the case same max prio here!
  if(_sender_prio > p->_sender_prio)
    {
      replace_node(p);
      x = p; // ok, insert the old top element too
      *head = this;
    }

  x->tree_insert(*head);
}



PUBLIC
//PUBLIC inline NEEDS [<cassert>, "cpu_lock.h", "lock_guard.h",
//                   Sender::remove_tree_elem, Sender::remove_head]
void Sender::sender_dequeue(Sender **head)
{

  if (!in_sender_list())
    return;

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  // we are removing top (the sender element with the highest prio
  // so we need to calculate an new top element

  if(this == *head)
    *head = remove_head();
  else
    remove_tree_elem();
  // mark as dequeued
  sender_next = 0;
}

// An special version, only to remove the head
// this is neccessary if the receiver removes the old know head
// after an unsuccessful ipc_receiver_ready.
PUBLIC
void Sender::sender_dequeue_head(Sender **head)
{

  if (!in_sender_list())
    return;

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  // we are removing top (the sender element with the highest prio
  // so we need to calculate an new top element

  if(this == *head)
    *head = remove_head();

  // mark as dequeued
  sender_next = 0;
}

PROTECTED
void Sender::sender_update_prio(Sender **head, unsigned short newprio)
{
  if(EXPECT_FALSE(sender_prio() == newprio))
    return;

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  if (!in_sender_list())
    return;

  sender_dequeue(head);
  sender_enqueue(head, newprio);
}

/** Constructor.
    @param id user-visible thread ID of the sender
 */
PROTECTED inline
Sender::Sender(const Global_id& id)
  : _id (id), sender_next (0)
{}

