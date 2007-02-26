INTERFACE:

template<typename T>
class Stack;

template<typename T>
class Stack_top
{
private:
  friend class Stack<T>;

  // Dont change the layout !!, comp_and_swap2 expects
  // version and _next  next to each other, in that order
  int _version;
  T   *_next;
};

template<typename T>
class Stack
{
private:
  Stack_top<T> _head;
};

IMPLEMENTATION:

#include "atomic.h"

// 
// Stack_top
// 

// template<typename T>
// Stack_top<T> &
// Stack_top<T>::operator=(const Stack_top& _copy){
//   _version = _copy._version;
//   _next    = _copy._next;
//   return *this;
// }

PUBLIC inline
template<typename T>
Stack_top<T>::Stack_top (int version, T *next)
  : _version (version),
    _next (next)
{}

PUBLIC inline
template<typename T>
Stack_top<T>::Stack_top ()
  : _version (0),
    _next (0)
{}

// 
// Stack
// 

PUBLIC
template<typename T>
Stack<T>::Stack()
  : _head (0, 0)
{}

PUBLIC
template<typename T>
int 
Stack<T>::insert(T *e)
{
  Stack_top<T>  old_head,
                  new_head;

  do {
      old_head             = _head;
      e->set_next(old_head._next);
      new_head._version    = old_head._version+1;
      new_head._next       = e;
  }  while (! smp_cas2(&_head, 
		       &old_head,
		       &new_head));
  return new_head._version;
}

PUBLIC
template<typename T>
T* 
Stack<T>::dequeue()
{
  Stack_top<T> old_head,
                 new_head;

  T *first;

  do {
    old_head          = _head;

    first = old_head._next;
    if(! first)
      break;

    new_head._next    = first->get_next();
    new_head._version = old_head._version + 1;
  }  while (! smp_cas2(&_head,
		       &old_head,
		       &new_head));
  //  XXX Why did the old implementation test on e ?
  //  while (e && ! compare_and_swap(&_first, e, e->list_property.next));
  //  This is necessary to handle the case of a empty stack.

  //  return old_head._next;
  return first;
}

// This version of dequeue only returns a value
// if it is equal to the one passed as top
PUBLIC
template<typename T>
T* 
Stack<T>::dequeue(T *top)
{
  Stack_top<T> old_head,
                 new_head;
  //  stack_elem_t  *first;

  old_head._version  = _head._version;   // version doesnt matter
  old_head._next     = top;              // cas will fail, if top aint at top
  if (!old_head._next)		// empty stack
    return 0;

  new_head._version = old_head._version + 1;
  new_head._next    = top->get_next();
  

  if(! smp_cas2(&_head,
		&old_head,
		&new_head))
    // we didnt succeed
    return 0;
  else 
    // top was on top , so we dequeued it
    return top;
}

PUBLIC
template<typename T>
T* 
Stack<T>::first()
{
  return _head._next;
}

PUBLIC
template<typename T>
void 
Stack<T>::reset()
{
  _head._version = 0;
  _head._next    = 0;
}
