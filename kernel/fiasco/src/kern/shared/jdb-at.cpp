INTERFACE:

class Jdb_at_entry
{
  friend class Jdb_at;

public:
  Jdb_at_entry(void (*call)(Thread *t));

private:
  void		(*_call)(Thread *t);
  Jdb_at_entry	*_next;
};

class Jdb_at
{
public:
  void		add(Jdb_at_entry *entry);
  void		execute(Thread *t);

private:
  Jdb_at_entry	*_first;
};


IMPLEMENTATION[at]:


IMPLEMENT
void
Jdb_at::add(Jdb_at_entry *entry)
{
  if (!_first)
    _first = entry;
  else
    {
      Jdb_at_entry *j;
      for (j=_first; j->_next; j=j->_next)
	;
      j->_next = entry;
    }
}

IMPLEMENT
void
Jdb_at::execute(Thread *t)
{
  Jdb_at_entry *j;
  for (j=_first; j; j=j->_next)
    j->_call(t);
}

IMPLEMENT
Jdb_at_entry::Jdb_at_entry(void (*call)(Thread *))
  : _call(call), _next(0)
{}

