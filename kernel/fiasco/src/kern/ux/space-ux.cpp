IMPLEMENTATION[ux]:

PUBLIC inline
pid_t
Space::pid() const		// returns host pid number
{
  return _mem_space.pid();
}

