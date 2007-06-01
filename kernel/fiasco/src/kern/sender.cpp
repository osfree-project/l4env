INTERFACE:

#include "l4_types.h"
#include "prio_list.h"

class Receiver;

/** A sender.  This is a role class, so real senders need to inherit from it.
 */
class Sender : private Prio_list_elem
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

  friend class Jdb;
  friend class Jdb_thread_list;
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
  return Prio_list_elem::prio();
}

/** Sender in a queue of senders?.  
    @return true if sender has enqueued in a receiver's list of waiting 
            senders
 */
PUBLIC inline
bool
Sender::in_sender_list() const
{
  return Prio_list_elem::in_list();
}

PUBLIC inline
bool
Sender::is_head_of(Prio_list const *l) const
{ return l->head() == this; }


PUBLIC static inline
Sender *
Sender::cast(Prio_list_elem *e)
{ return static_cast<Sender*>(e); }


PUBLIC
//PROTECTED inline NEEDS [<cassert>, "cpu_lock.h", "lock_guard.h",
//                      Sender::replace_node, Sender::tree_insert]
void Sender::sender_enqueue(Prio_list *head, unsigned short prio)
{
  assert(prio <256);

  Lock_guard<Cpu_lock> guard (&cpu_lock);
  head->insert(this, prio);
}



PUBLIC
//PUBLIC inline NEEDS [<cassert>, "cpu_lock.h", "lock_guard.h",
//                   Sender::remove_tree_elem, Sender::remove_head]
void Sender::sender_dequeue(Prio_list *list)
{

  if (!in_sender_list())
    return;

  Lock_guard<Cpu_lock> guard (&cpu_lock);
  list->dequeue(this);
}

// An special version, only to remove the head
// this is neccessary if the receiver removes the old know head
// after an unsuccessful ipc_receiver_ready.
PUBLIC
void Sender::sender_dequeue_head(Prio_list *list)
{

  if (!in_sender_list())
    return;

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  if (this == list->head())
    list->dequeue(this);
}

PROTECTED
void Sender::sender_update_prio(Prio_list *list, unsigned short newprio)
{
  if(EXPECT_FALSE(sender_prio() == newprio))
    return;

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  if (!in_sender_list())
    return;

  sender_dequeue(list);
  sender_enqueue(list, newprio);
}

/** Constructor.
    @param id user-visible thread ID of the sender
 */
PROTECTED inline
Sender::Sender(const Global_id& id)
  : _id (id)
{}

