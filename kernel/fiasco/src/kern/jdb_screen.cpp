INTERFACE:

class Jdb_screen
{
private:
  static unsigned int _height;
  static bool         _direct_enabled;
};

IMPLEMENTATION:

unsigned int Jdb_screen::_height         = 25; // default for native
bool         Jdb_screen::_direct_enabled = true;

PUBLIC static
void
Jdb_screen::set_height(unsigned int h)
{
  _height = h;
}

PUBLIC static inline
unsigned int
Jdb_screen::width()
{
  return 80;
}

PUBLIC static inline
unsigned int
Jdb_screen::height()
{
  return _height;
}

PUBLIC static inline
void
Jdb_screen::enable_direct(bool enable)
{
  _direct_enabled = enable;
}

PUBLIC static inline
bool
Jdb_screen::direct_enabled()
{
  return _direct_enabled;
}
