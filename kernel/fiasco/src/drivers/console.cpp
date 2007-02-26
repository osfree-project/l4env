INTERFACE:

#include <stddef.h>

/**
 * @brief The abstract interface for a text I/O console.
 *
 * This abstract interface can be implemented for virtually every
 * text input or output device.
 */
class Console
{
public:
  /**
   * @brief Write a string of len chacters to the output.
   * @param str the string to write (no zero termination is needed)
   * @param len the number of chacters to write.
   *
   * This method must be implemented in every implementation, but
   * can simply do nothing for input only consoles.
   */
  virtual int write( char const *str, size_t len ) = 0;
  
  /**
   * @brief read a charcater from the input.
   * @param blocking if true getchar blocks til a charcater is available.
   *
   * This method must be implemented in every implementation, but
   * can simply return -1 for output only consoles.
   */
  virtual int getchar( bool blocking = true ) = 0;

  /**
   * @brief Is input available?
   *
   * This method can be implemented.
   * It must return -1 if no information is available, 
   * 1 if at least one character is avialable, and 0 if
   * no charachter is available.
   */
  virtual int char_avail() const;

  virtual ~Console();

public:

  /**
   * @brief Disables the stdout, stdin, and stderr console.
   */
  static void disable_all();

  /// stdout for libc glue.
  static Console *stdout;
  /// stderr for libc glue.
  static Console *stderr;
  /// stdin for libc glue.
  static Console *stdin;
};



IMPLEMENTATION:

Console *Console::stdout = 0;
Console *Console::stderr = 0;
Console *Console::stdin  = 0;

IMPLEMENT Console::~Console()
{}

IMPLEMENT
void Console::disable_all()
{
  stdout = 0;
  stderr = 0;
  stdin  = 0;
}

IMPLEMENT
int Console::char_avail() const
{
  //write("+",1);
  return -1; /* unknown */
}

