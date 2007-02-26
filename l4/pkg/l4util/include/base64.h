/*! 
 * \file l4util/include/ARCH-x86/base64.h
 * \brief   base 64 encoding and decoding functions adapted from  Bob Trower 08/04/01
 * \ingroup utils
 *
 * \date    04/26/2002
 * \author  Joerg Nothnagel <jn6@os.inf.tu-dresden.de>
 */

#ifndef B64_EN_DECODE
#define B64_EN_DECODE


/*!
 * \brief base-64-encode string \a infile
 * \ingroup utils_internal
 *
 * 
 * \param infile string to be encoded
 * \param in_size length of \a infile 
 * \retval outfile the base-64-encoded representation of \infile
 *
 *  base-64-encode string \a infile adding padding as per spec
 */
void base64_encode( const char *infile, unsigned int in_size, char **outfile);
 
/*!
 * \brief decode base-64-encoded string \a infile
 * \ingroup utils_internal
 *
 * 
 * \param infile string to be decoded
 * \param in_size length of \a infile 
 * \retval outfile the decoded representation of \infile
 *
 *  base-64-decode string \a infile discarding padding, line breaks and noise
 */
void base64_decode(const char *infile, unsigned int in_size, char **outfile);

#endif //B64_EN_DECODE
