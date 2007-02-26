/*
 * Copyright (C) 2006 Michael Roitzsch <mroi@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 */

#include "process.h"

#if SIDEBAND_COMPRESSION && (SIDEBAND_WRITE || SIDEBAND_READ)
/* sideband data is compressed using Fibonacci coding,
 * see http://en.wikipedia.org/wiki/Fibonacci_coding for details */
static const uint8_t fibonacci[] = {
  1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233
};
#endif


#if SIDEBAND_WRITE

int slice_start(void)
{
  while (1) {
    /* peek ahead to get the start code */
    if (fread(proc.sideband.buf, 1, 4, proc.sideband.from) != 4)
      printf("could not read NALU start code\n");
    fseek(proc.sideband.from, -4, SEEK_CUR);
    if (proc.sideband.buf[0] == 0 && proc.sideband.buf[1] == 0 && proc.sideband.buf[2] == 1)
      break;
    else
      /* not at a nalu start, align */
      copy_nalu();
  }
  return ((proc.sideband.buf[3] & 0x1F) > 0 && (proc.sideband.buf[3] & 0x1F) < 6);
}

void copy_nalu(void)
{
  int i, bytes;
  /* skip our own NALUs, if the file has already been preprocessed */
  const int skip =
    (proc.sideband.buf[0] == 0) &&
    (proc.sideband.buf[1] == 0) &&
    (proc.sideband.buf[2] == 1) &&
    ((proc.sideband.buf[3] & 0x1F) == NAL_SIDEBAND_DATA);
  
  /* the start bytes are alredy in the buffer, take them from there and skip them in the file */
  if (!skip)
    fwrite(proc.sideband.buf, 1, 4, proc.sideband.to);
  fseek(proc.sideband.from, 4, SEEK_CUR);
  do {
    bytes = fread(proc.sideband.buf, 1, sizeof(proc.sideband.buf), proc.sideband.from);
    for (i = 0; i < bytes - 2; i++)
      if (proc.sideband.buf[i] == 0 && proc.sideband.buf[i+1] == 0 && proc.sideband.buf[i+2] == 1)
	break;
    if (!skip)
      fwrite(proc.sideband.buf, 1, i, proc.sideband.to);
    fseek(proc.sideband.from, i - bytes, SEEK_CUR);
  } while (i == bytes - 2);
}

void nalu_write_start(void)
{
  const uint8_t custom[4] = { 0x00, 0x00, 0x01, NAL_SIDEBAND_DATA };
  
  fwrite(custom, 1, 4, proc.sideband.to);
  proc.sideband.history[2] = 0xFF;
#if SIDEBAND_COMPRESSION
  proc.sideband.remain = 0;
  proc.sideband.remain_bits = 0;
#endif
}

static void nalu_write_byte(uint8_t value)
{
  const uint8_t fill = 0x03;
  
  proc.sideband.history[0] = proc.sideband.history[1];
  proc.sideband.history[1] = proc.sideband.history[2];
  proc.sideband.history[2] = value;
  /* the three byte sequences 0x000001, 0x000002 and 0x000003 are special in H.264 NAL;
   * if they do appear regularly, they must be disguised by inserting an extra 0x03 */
  if (proc.sideband.history[0] == 0 && proc.sideband.history[1] == 0 && proc.sideband.history[2] < 4) {
    fwrite(&fill, 1, 1, proc.sideband.to);
    proc.sideband.history[1] = 0xFF;
  }
  fwrite(&value, 1, 1, proc.sideband.to);
}

void nalu_write_end(void)
{
#if SIDEBAND_COMPRESSION
  uint32_t coded = proc.sideband.remain;
  int bits = proc.sideband.remain_bits;
  /* flush all remaining bits */
  for (; bits > 0; bits -= 8, coded <<= 8)
    nalu_write_byte((uint8_t)(coded >> 24));
#endif
}

void nalu_write_uint8(uint8_t value)
{
#if SIDEBAND_COMPRESSION
  nalu_write_int8((int8_t)value);
#else
  nalu_write_byte(value);
#endif
}

void nalu_write_uint16(uint16_t value)
{
  /* big endian stream */
  nalu_write_uint8((value >> 8) & 0xFF);
  nalu_write_uint8((value >> 0) & 0xFF);
}

void nalu_write_uint24(uint32_t value)
{
  /* big endian stream */
  nalu_write_uint8((value >> 16) & 0xFF);
  nalu_write_uint8((value >>  8) & 0xFF);
  nalu_write_uint8((value >>  0) & 0xFF);
}

void nalu_write_uint32(uint32_t value)
{
  /* big endian stream */
  nalu_write_uint8((value >> 24) & 0xFF);
  nalu_write_uint8((value >> 16) & 0xFF);
  nalu_write_uint8((value >>  8) & 0xFF);
  nalu_write_uint8((value >>  0) & 0xFF);
}

void nalu_write_int8(int8_t value)
{
#if SIDEBAND_COMPRESSION
  /* map to positive values greater zero */
  unsigned normalized =
    ((value <  0) ? (2 * -(int)value + 1) :
    ((value == 0) ? 1 :
    ((value >  0) ? (2 * (int)value) : 0)));
  /* create the coded number in the higher bits in reverse */
  uint32_t coded;
  int i, bits;
  
  /* search for the highest contribution */
  for (i = sizeof(fibonacci) / sizeof(fibonacci[0]) - 1; i >= 0; i--)
    if (fibonacci[i] <= normalized) break;
  /* bit count is one more than i because i starts counting with zero
   * and maybe an additional one more because of the one bit already in coded */
  if (i == sizeof(fibonacci) / sizeof(fibonacci[0]) - 1) {
    /* we can omit the end marker if the number is of maximal bitlength */
    coded = 0;
    bits = i + 1;
  } else {
    coded = (1 << 31);
    bits = i + 2;
  }
  for (; i >= 0; i--) {
    coded >>= 1;
    if (fibonacci[i] <= normalized) {
      coded |= (1 << 31);
      normalized -= fibonacci[i];
    }
  }
  assert(normalized == 0);
  /* merge with not yet written bits */
  bits += proc.sideband.remain_bits;
  coded >>= proc.sideband.remain_bits;
  coded |= proc.sideband.remain;
  /* write all completed bytes */
  for (; bits >= 8; bits -= 8, coded <<= 8)
    nalu_write_byte((uint8_t)(coded >> 24));
  /* remember the remainder for later */
  proc.sideband.remain_bits = bits;
  proc.sideband.remain = coded;
#else
  nalu_write_uint8((uint8_t)value);
#endif
}

void nalu_write_int16(int16_t value)
{
  nalu_write_uint16((uint16_t)value);
}

void nalu_write_float(float value)
{
  uint32_t out;
  float *in = (float *)(void *)&out;
  assert(sizeof(float) == sizeof(uint32_t));
  *in = value;
  nalu_write_uint32(out);
}

#endif

#if SIDEBAND_READ

void nalu_read_start(uint8_t *nalu)
{
  proc.sideband.nalu = nalu;
#if SIDEBAND_COMPRESSION
  proc.sideband.bits = 8;
#endif
}

uint8_t nalu_read_uint8(void)
{
#if SIDEBAND_COMPRESSION
  return (uint8_t)nalu_read_int8();
#else
  return *(proc.sideband.nalu++);
#endif
}

uint16_t nalu_read_uint16(void)
{
  /* big endian stream */
  uint16_t value = 0;
  value |= ((uint16_t)nalu_read_uint8() << 8);
  value |= ((uint16_t)nalu_read_uint8() << 0);
  return value;
}

uint32_t nalu_read_uint24(void)
{
  /* big endian stream */
  uint32_t value = 0;
  value |= ((uint32_t)nalu_read_uint8() << 16);
  value |= ((uint32_t)nalu_read_uint8() <<  8);
  value |= ((uint32_t)nalu_read_uint8() <<  0);
  return value;
}

uint32_t nalu_read_uint32(void)
{
  /* big endian stream */
  uint32_t value = 0;
  value |= ((uint32_t)nalu_read_uint8() << 24);
  value |= ((uint32_t)nalu_read_uint8() << 16);
  value |= ((uint32_t)nalu_read_uint8() <<  8);
  value |= ((uint32_t)nalu_read_uint8() <<  0);
  return value;
}

int8_t nalu_read_int8(void)
{
#if SIDEBAND_COMPRESSION
  unsigned coded = 0;
  int i, bit, last_bit = 0;
  
  for (i = 0; i < sizeof(fibonacci) / sizeof(fibonacci[0]); i++) {
    bit = *proc.sideband.nalu & (1 << 7);
    *proc.sideband.nalu <<= 1;
    if (!(--proc.sideband.bits)) {
      proc.sideband.nalu++;
      proc.sideband.bits = 8;
    }
    if (bit && last_bit) break;
    if (bit) coded += fibonacci[i];
    last_bit = bit;
  }
  /* map to positive and negative integers with zero */
  if (coded & 1)
    return (int8_t)-((coded - 1) / 2);
  else
    return (int8_t)(coded / 2);
#else
  return (int8_t)nalu_read_uint8();
#endif
}

int16_t nalu_read_int16(void)
{
  return (int16_t)nalu_read_uint16();
}

float nalu_read_float(void)
{
  uint32_t in;
  float *out = (float *)(void *)&in;
  assert(sizeof(float) == sizeof(uint32_t));
  in = nalu_read_uint32();
  return *out;
}

#endif
