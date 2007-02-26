#ifndef L4_EXEC_REFERENCE_COUNTED_OBJECT_H
#define L4_EXEC_REFERENCE_COUNTED_OBJECT_H

/** Base class for reference-counted objects.  This implementation is
    similar to the one of RCObject in Scott Meyer's ``More Effective
    C++.'' */
class obj_refcounted
{
public:

  /** @name Reference-counting functions */
  
  //@{
  /** Add a reference.  Returns new reference count. */
  int add_reference () const;

  /** Remove a reference.  Deletes object if reference count drops to
      0.  Returns new reference count. */
  int remove_reference () const;

  /** Return reference count */
  int reference_count () const;

  //@}

protected:

  obj_refcounted ();		///< Initialize reference count.
  obj_refcounted (const obj_refcounted&); ///< Copy reference count.

  /** Protected destructor.  Clients using subclasses derived from
      this class are not allowed to delete objects directly -- they
      must use the reference counting mechanism. */
  virtual ~obj_refcounted () = 0;

  /// Disallow assignment for clients.
  obj_refcounted& operator= (const obj_refcounted& right);

private:

  mutable int d_ref_count;
};

inline int 
obj_refcounted::add_reference () const
{
  return ++d_ref_count;
}

inline int
obj_refcounted::remove_reference () const
{
  --d_ref_count;

  if (d_ref_count < 1)
    {
      delete this;
      return 0;
    }
	
  return d_ref_count;
}

inline int
obj_refcounted::reference_count () const
{
  return d_ref_count;
}

inline 
obj_refcounted::obj_refcounted ()
  : d_ref_count(0)
{}

inline 
obj_refcounted::obj_refcounted (const obj_refcounted& rhs)
  : d_ref_count(0)
{}

inline 
obj_refcounted::~obj_refcounted ()
{}

inline obj_refcounted&
obj_refcounted::operator= (const obj_refcounted& rhs)
{
  return *this;
}

#endif

