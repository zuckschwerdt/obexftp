/*********************************************************************
 *                
 * Filename:      crc.h
 * Version:       
 * Description:   CRC routines
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Mon Aug  4 20:40:53 1997
 * Modified at:   Sun May  2 20:25:23 1999
 * Modified by:   Dag Brattli <dagb@cs.uit.no>
 * 
 ********************************************************************/

#ifndef IRDA_CRC_H
#define IRDA_CRC_H

#include <inttypes.h>

#define INIT_FCS  0xffff   /* Initial FCS value */
#define GOOD_FCS  0xf0b8   /* Good final FCS value */

extern uint16_t const irda_crc16_table[];

/* Recompute the FCS with one more character appended. */
static inline uint16_t irda_fcs(uint16_t fcs, uint8_t c)
{
	return (((fcs) >> 8) ^ irda_crc16_table[((fcs) ^ (c)) & 0xff]);
}

/* Recompute the FCS with len bytes appended. */
unsigned short crc_calc( uint16_t fcs, uint8_t const *buf, int len);

#endif
