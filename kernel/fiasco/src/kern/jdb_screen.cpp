INTERFACE:

class Jdb_screen
{
public:
  static unsigned int height();
  static void         set_height(unsigned int);

private:
  static unsigned int _height;
};

IMPLEMENTATION:

unsigned int Jdb_screen::_height = 25; // default for native

IMPLEMENT
void
Jdb_screen::set_height(unsigned int h)
{
  _height = h;
}

IMPLEMENT inline
unsigned int
Jdb_screen::height()
{
  return _height;
}
