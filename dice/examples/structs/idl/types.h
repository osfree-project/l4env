#ifndef __TYPES_H__
#define __TYPES_H__

struct A
{
  char a;
  char b;
  unsigned int c; // is dword-aligned
};

struct B {
  char a;
  char b;
  unsigned int c; // is not dword-aligned
} __attribute__((packed));

#endif // !__TYPES_H__
