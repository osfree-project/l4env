/* -*- c++ -*- */
#ifndef L4_X_PAGE_H__
#define L4_X_PAGE_H__

#include <l4/sys/l4int.h>
#include <l4/sys/consts.h>

class Page
{
public:
  Page() : state(1) {}
  bool is_reserved() const { return state == 1; }
  bool is_free() const { return state == 0; }
  char owner() const { return state; }

  void reserve() { state = 1; }
  void free() { state = 0; }
  void change_owner( char owner ) { state = owner; }
private:
  char state;
} __attribute__((packed));

class SuperPage
{
public:
  SuperPage() : free_pages(0), state(1) {}

  bool is_reserved() const { return state == 1; }
  bool is_free() const { return state == 0; }
  char owner() const { return state; }

  void reserve() { state = 1; }
  void free() { free_pages = L4_SUPERPAGESIZE/L4_PAGESIZE; state = 0; }
  void change_owner( char owner ) { state = owner; }
  void inc_free() 
  { 
    if((++free_pages) == (L4_SUPERPAGESIZE/L4_PAGESIZE))
      state = 0;
  }

  void dec_free() { --free_pages; }

  l4_uint16_t nr_free() const { return free_pages; }

private:
  l4_uint16_t free_pages;
  char state;
  char padding;
} __attribute__((packed));


#endif /* L4_X_PAGE_H__ */
