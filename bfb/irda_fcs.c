/* Taken with renamed variables/function from
 * "A Painless Guide to CRC Error Detection Algorithms", chapter 18.
 *
 * This code is in the public domain.
 */

#include "irda_fcs.h"

/* For the values see IrPHY specification, chapter 5.3.1 */
#define FCS_INIT  0xFFFF
#define FCS_XOROT 0xFFFF
extern unsigned short crctable[256];

unsigned short irda_fcs (unsigned char *blk_adr, unsigned long blk_len)
{
  unsigned short fcs = FCS_INIT;
  while (blk_len--)
    fcs = crctable[(fcs ^ *blk_adr++) & 0xFFL] ^ (fcs >> 8);
  return fcs ^ FCS_XOROT;
}
