/*
 * Some testing for l4util functions
 *
 * This probably only runs successful on little-endian and 32 bits archs
 *
 *
 * Extend it, if you feel like that!
 */

#include <stdio.h>

#include <l4/util/atomic.h>
#include <l4/util/bitops.h>

static int cmpxchg(void)
{
  l4_umword_t val = 345;

  if (!l4util_cmpxchg(&val, 345, 678) || val != 678)
    return 1;

  if (l4util_cmpxchg(&val, 123, 456))
    return 2;
  
  return 0;
}

static int cmpxchg_res(void)
{
  l4_umword_t val = 345;

  if (l4util_cmpxchg_res(&val, 345, 678) != 345 || val != 678)
    return 1;

  if (l4util_cmpxchg_res(&val, 123, 456) != 678)
    return 2;
  
  return 0;
}

static int set_bit(void)
{
  l4_umword_t val     = 3;
  l4_umword_t vals[5] = { 0, };

  l4util_set_bit(0, &val);
  if (val != 3)
    return 1;

  l4util_set_bit(4, &val);
  if (val != 19)
    return 2;

  l4util_set_bit(121, vals);
  if (vals[3] != 0x02000000)
    return 3;

  return 0;
}

static int clear_bit(void)
{
  l4_umword_t val = 16;
  l4_umword_t vals[9] = { 0xffccbbaa, 0xffccbbaa, 0xffccbbaa, 0xffccbbaa,
                          0xffccbbaa, 0xffccbbaa, 0xffccbbaa, 0xffccbbaa,
                          0xffccbbaa };

  l4util_clear_bit(1, &val);
  if (val != 16)
    return 1;

  l4util_clear_bit(4, &val);
  if (val != 0)
    return 2;

  l4util_clear_bit(245, vals);
  if (vals[7] != 0xffccbbaa)
    return 3;

  l4util_clear_bit(227, vals);
  if (vals[7] != 0xffccbba2)
    return 4;

  return 0;
}

static int test_bit(void)
{
  l4_umword_t val = 6;
  l4_umword_t vals[2] = { 0x34, 0x67 };

  if (!l4util_test_bit(2, &val))
    return 1;

  if (l4util_test_bit(17, &val))
    return 2;

  if (!l4util_test_bit(32, vals))
    return 3;

  return 0;
}

static int test_and_set_bit(void)
{
  l4_umword_t val = 8;

  if (l4util_test_and_set_bit(2, &val) != 0 || val != 12)
    return 1;

  return 0;
}

static int test_and_clear_bit(void)
{
  l4_umword_t val = 8;

  if (l4util_test_and_clear_bit(3, &val) != 1 || val != 0)
    return 1;

  return 0;
}

static int test_bit_scan_reverse(void)
{
  if (l4util_bsr(17) != 4)
    return 1;

  return 0;
}

static int test_bit_scan_forward(void)
{
  if (l4util_bsf(0xbc123000) != 12)
    return 1;

  return 0;
}

static int test_find_first_zero_bit(void)
{
  l4_umword_t val = 55;
  l4_umword_t vals[] = { 0xffffffff, 0xffffffff, 0xffffefff };

  if (l4util_find_first_zero_bit(&val, sizeof(val) * 8) != 3)
    return 1;

  if (l4util_find_first_zero_bit(vals, sizeof(vals) * 8) != 76)
    return 2;

  if (l4util_find_first_zero_bit(vals, 64) < 64)
    return 3;

  if (l4util_find_first_zero_bit(vals, 0) != 0)
    return 4;

  return 0;
}



static void test(const char *name, int (*t)(void))
{
  int ret;
  ret = t();
  printf("%30s  %.15s", name, ret ? "failed" : "OK");
  if (ret)
    printf(" (Test %d)\n", ret);
  printf("\n");
}

int main(void)
{

  printf("l4util function testing.\n");

  test("cmpxchg",             cmpxchg);
  test("cmpxchg_res",         cmpxchg_res);
  test("set_bit",             set_bit);
  test("clear_bit",           clear_bit);
  test("test_bit",            test_bit);
  test("test_and_set_bit",    test_and_set_bit);
  test("test_and_clear_bit",  test_and_clear_bit);
  test("bit_scan_reverse",    test_bit_scan_reverse);
  test("bit_scan_forward",    test_bit_scan_forward);
  test("find_first_zero_bit", test_find_first_zero_bit);
 
  return 0;
}
