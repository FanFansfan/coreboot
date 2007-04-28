/*
 *
 * Copyright (C) 200X FIXME
 * Copyright (C) 2007 coresystems GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 *
 */


#include <arch/types.h>
#include <arch/io.h>
#include <console/console.h>
#include <tables.h>
#include <mc146818rtc.h>
#include <string.h>
#include <lar.h>

/*
 * Why is this not a function? I assume the two io 
 * operations are less overhead than a function call.
 */

#define CMOS_READ(addr) ({ \
outb((addr),RTC_PORT(0)); \
inb(RTC_PORT(1)); \
})

#define CMOS_WRITE(val, addr) ({ \
outb((addr),RTC_PORT(0)); \
outb((val),RTC_PORT(1)); \
})

/* control registers - Moto names
 */
#define RTC_REG_A		10
#define RTC_REG_B		11
#define RTC_REG_C		12
#define RTC_REG_D		13


/**********************************************************************
 * register details
 **********************************************************************/
#define RTC_FREQ_SELECT	RTC_REG_A

/* update-in-progress  - set to "1" 244 microsecs before RTC goes off the bus,
 * reset after update (may take 1.984ms @ 32768Hz RefClock) is complete,
 * totalling to a max high interval of 2.228 ms.
 */
# define RTC_UIP		0x80
# define RTC_DIV_CTL		0x70
   /* divider control: refclock values 4.194 / 1.049 MHz / 32.768 kHz */
#  define RTC_REF_CLCK_4MHZ	0x00
#  define RTC_REF_CLCK_1MHZ	0x10
#  define RTC_REF_CLCK_32KHZ	0x20
   /* 2 values for divider stage reset, others for "testing purposes only" */
#  define RTC_DIV_RESET1	0x60
#  define RTC_DIV_RESET2	0x70
  /* Periodic intr. / Square wave rate select. 0=none, 1=32.8kHz,... 15=2Hz */
# define RTC_RATE_SELECT 	0x0F
#  define RTC_RATE_NONE		0x00
#  define RTC_RATE_32786HZ	0x01
#  define RTC_RATE_16384HZ	0x02
#  define RTC_RATE_8192HZ	0x03
#  define RTC_RATE_4096HZ	0x04
#  define RTC_RATE_2048HZ	0x05
#  define RTC_RATE_1024HZ	0x06
#  define RTC_RATE_512HZ	0x07
#  define RTC_RATE_256HZ	0x08
#  define RTC_RATE_128HZ	0x09
#  define RTC_RATE_64HZ		0x0a
#  define RTC_RATE_32HZ		0x0b
#  define RTC_RATE_16HZ		0x0c
#  define RTC_RATE_8HZ		0x0d
#  define RTC_RATE_4HZ		0x0e
#  define RTC_RATE_2HZ		0x0f

/**********************************************************************/
#define RTC_CONTROL	RTC_REG_B
# define RTC_SET 0x80		/* disable updates for clock setting */
# define RTC_PIE 0x40		/* periodic interrupt enable */
# define RTC_AIE 0x20		/* alarm interrupt enable */
# define RTC_UIE 0x10		/* update-finished interrupt enable */
# define RTC_SQWE 0x08		/* enable square-wave output */
# define RTC_DM_BINARY 0x04	/* all time/date values are BCD if clear */
# define RTC_24H 0x02		/* 24 hour mode - else hours bit 7 means pm */
# define RTC_DST_EN 0x01	/* auto switch DST - works f. USA only */

/**********************************************************************/
#define RTC_INTR_FLAGS	RTC_REG_C
/* caution - cleared by read */
# define RTC_IRQF 0x80		/* any of the following 3 is active */
# define RTC_PF 0x40
# define RTC_AF 0x20
# define RTC_UF 0x10

/**********************************************************************/
#define RTC_VALID	RTC_REG_D
# define RTC_VRT 0x80		/* valid RAM and time */
/**********************************************************************/

#if defined(CONFIG_OPTION_TABLE) && (CONFIG_OPTION_TABLE == 1)

static int rtc_checksum_valid(int range_start, int range_end, int cks_loc)
{
	int i;
	unsigned sum, old_sum;
	sum = 0;
	for(i = range_start; i <= range_end; i++) {
		sum += CMOS_READ(i);
	}
	sum = (~sum)&0x0ffff;
	old_sum = ((CMOS_READ(cks_loc)<<8) | CMOS_READ(cks_loc+1))&0x0ffff;
	return sum == old_sum;
}

static void rtc_set_checksum(int range_start, int range_end, int cks_loc)
{
	int i;
	unsigned sum;
	sum = 0;
	for(i = range_start; i <= range_end; i++) {
		sum += CMOS_READ(i);
	}
	sum = ~(sum & 0x0ffff);
	CMOS_WRITE(((sum >> 8) & 0x0ff), cks_loc);
	CMOS_WRITE(((sum >> 0) & 0x0ff), cks_loc+1);
}

#endif

#define RTC_CONTROL_DEFAULT (RTC_24H)
#define RTC_FREQ_SELECT_DEFAULT (RTC_REF_CLCK_32KHZ | RTC_RATE_1024HZ)

void rtc_init(int invalid)
{
#if defined(CONFIG_OPTION_TABLE) && (CONFIG_OPTION_TABLE == 1)
	unsigned char x;
	int cmos_invalid, checksum_invalid;
#endif

	printk(BIOS_DEBUG, "Initializing realtime clock.\n");

#if defined(CONFIG_OPTION_TABLE) && (CONFIG_OPTION_TABLE == 1)

	/* See if there has been a CMOS power problem. */
	x = CMOS_READ(RTC_VALID);
	cmos_invalid = !(x & RTC_VRT);

	/* See if there is a CMOS checksum error */
	checksum_invalid = !rtc_checksum_valid(PC_CKS_RANGE_START,
			PC_CKS_RANGE_END,PC_CKS_LOC);

	if (invalid || cmos_invalid || checksum_invalid) {
		printk(BIOS_WARNING, "RTC:%s%s%s zeroing cmos\n",
			invalid?" Clear requested":"", 
			cmos_invalid?" Power Problem":"",
			checksum_invalid?" Checksum invalid":"");
#if 0
		CMOS_WRITE(0, 0x01);
		CMOS_WRITE(0, 0x03);
		CMOS_WRITE(0, 0x05);
		for(i = 10; i < 48; i++) {
			CMOS_WRITE(0, i);
		}
		
		if (cmos_invalid) {
			/* Now setup a default date of Sat 1 January 2000 */
			CMOS_WRITE(0, 0x00); /* seconds */
			CMOS_WRITE(0, 0x02); /* minutes */
			CMOS_WRITE(1, 0x04); /* hours */
			CMOS_WRITE(7, 0x06); /* day of week */
			CMOS_WRITE(1, 0x07); /* day of month */
			CMOS_WRITE(1, 0x08); /* month */
			CMOS_WRITE(0, 0x09); /* year */
		}
#endif
	}
#endif

	/* Setup the real time clock */
	CMOS_WRITE(RTC_CONTROL_DEFAULT, RTC_CONTROL);
	/* Setup the frequency it operates at */
	CMOS_WRITE(RTC_FREQ_SELECT_DEFAULT, RTC_FREQ_SELECT);

#if defined(CONFIG_OPTION_TABLE) && (CONFIG_OPTION_TABLE == 1)
	/* See if there is a LB CMOS checksum error */
	checksum_invalid = !rtc_checksum_valid(LB_CKS_RANGE_START,
			LB_CKS_RANGE_END,LB_CKS_LOC);
	if(checksum_invalid)
		printk(BIOS_WARNING, "Invalid LinuxBIOS CMOS checksum.\n");

	/* Make certain we have a valid checksum */
	rtc_set_checksum(PC_CKS_RANGE_START,
                        PC_CKS_RANGE_END,PC_CKS_LOC);
#endif

	/* Clear any pending interrupts */
	(void) CMOS_READ(RTC_INTR_FLAGS);
}


/* This routine returns the value of the requested bits
	input bit = bit count from the beginning of the cmos image
	      length = number of bits to include in the value
	      ret = a character pointer to where the value is to be returned
	output the value placed in ret
	      returns 0 = successful, -1 = an error occurred
*/
static int get_cmos_value(unsigned long bit, unsigned long length, void *vret)
{
	unsigned char *ret;
	unsigned long byte,byte_bit;
	unsigned long i;
	unsigned char uchar;

	/* The table is checked when it is built to ensure all 
		values are valid. */
	ret = vret;
	byte=bit/8;	/* find the byte where the data starts */
	byte_bit=bit%8; /* find the bit in the byte where the data starts */
	if(length<9) {	/* one byte or less */
		uchar = CMOS_READ(byte); /* load the byte */
		uchar >>= byte_bit;	/* shift the bits to byte align */
		/* clear unspecified bits */
		ret[0] = uchar & ((1 << length) -1);
	}
	else {	/* more that one byte so transfer the whole bytes */
		for(i=0;length;i++,length-=8,byte++) {
			/* load the byte */
			ret[i]=CMOS_READ(byte);
		}
	}
	return 0;
}

struct cmos_option_table *get_option_table(void)
{
	struct mem_file result, archive;
	int ret;

	// FIXME - i want to be dynamic.
	archive.len=(CONFIG_LINUXBIOS_ROMSIZE_KB-16)*1024;
	archive.start=(void *)(0UL-(CONFIG_LINUXBIOS_ROMSIZE_KB*1024)); 

	ret = find_file(&archive, "normal/option_table", &result);
	if (ret) {
		printk(BIOS_ERR, "No such file '%s'.\n",
				"normal/option_table");
		return (struct cmos_option_table *)NULL;
	}
	return (struct cmos_option_table *) result.start;
}

int get_option(void *dest, char *name)
{
	// struct cmos_option_table *option_table;
	struct cmos_option_table *ct;
	struct cmos_entries *ce;
	size_t namelen;
	int found=0;
	/* Figure out how long name is */
	namelen = strnlen(name, CMOS_MAX_NAME_LENGTH);
	
	/* find the requested entry record */
	ct=get_option_table();
	ce=(struct cmos_entries*)((unsigned char *)ct + ct->header_length);
	for(;ce->tag==LB_TAG_OPTION;
		ce=(struct cmos_entries*)((unsigned char *)ce + ce->size)) {
		if (memcmp(ce->name, name, namelen) == 0) {
			found=1;
			break;
		}
	}
	if(!found) {
		printk(BIOS_ERR, "ERROR: No cmos option '%s'\n", name);
		return(-2);
	}
	
	if(get_cmos_value(ce->bit, ce->length, dest))
		return(-3);
	if(!rtc_checksum_valid(LB_CKS_RANGE_START,
			LB_CKS_RANGE_END,LB_CKS_LOC))
		return(-4);
#if defined(CONFIG_OPTION_TABLE) && (CONFIG_OPTION_TABLE == 1)
	return(0);
#else
	return -2;
#endif
}

/* -------------------------------------------------------------- */

// FIXME: Early starts here

#ifndef MAX_REBOOT_CNT
#define MAX_REBOOT_CNT 2
#endif

static int cmos_error(void)
{
	unsigned char reg_d;
	/* See if the cmos error condition has been flagged */
	reg_d = CMOS_READ(RTC_REG_D);
	return (reg_d & RTC_VRT) == 0;
}

static int cmos_chksum_valid(void)
{
	unsigned char addr;
	unsigned long sum, old_sum;
	sum = 0;
	/* Comput the cmos checksum */
	for(addr = LB_CKS_RANGE_START; addr <= LB_CKS_RANGE_END; addr++) {
		sum += CMOS_READ(addr);
	}
	sum = (sum & 0xffff) ^ 0xffff;

	/* Read the stored checksum */
	old_sum = CMOS_READ(LB_CKS_LOC) << 8;
	old_sum |=  CMOS_READ(LB_CKS_LOC+1);

	return sum == old_sum;
}


int last_boot_normal(void)
{
	unsigned char byte;
	byte = CMOS_READ(RTC_BOOT_BYTE);
	return (byte & (1 << 1));
}

static int do_normal_boot(void)
{
	unsigned char byte;

	if (cmos_error() || !cmos_chksum_valid()) {
		unsigned char byte;
		/* There are no impossible values, no cheksums so just
		 * trust whatever value we have in the the cmos,
		 * but clear the fallback bit.
		 */
		byte = CMOS_READ(RTC_BOOT_BYTE);
		byte &= 0x0c;
		byte |= MAX_REBOOT_CNT << 4;
		CMOS_WRITE(byte, RTC_BOOT_BYTE);
	}

	/* The RTC_BOOT_BYTE is now o.k. see where to go. */
	byte = CMOS_READ(RTC_BOOT_BYTE);
	
	/* Are we in normal mode? */
	if (byte & 1) {
		byte &= 0x0f; /* yes, clear the boot count */
	}

	/* Properly set the last boot flag */
	byte &= 0xfc;
	if ((byte >> 4) < MAX_REBOOT_CNT) {
		byte |= (1<<1);
	}

	/* Are we already at the max count? */
	if ((byte >> 4) < MAX_REBOOT_CNT) {
		byte += 1 << 4; /* No, add 1 to the count */
	}
	else {
		byte &= 0xfc;	/* Yes, put in fallback mode */
	}

	/* Save the boot byte */
	CMOS_WRITE(byte, RTC_BOOT_BYTE);

	return (byte & (1<<1));
}


