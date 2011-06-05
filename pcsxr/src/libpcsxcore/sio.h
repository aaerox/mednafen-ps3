/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/


#ifndef _SIO_H_
#define _SIO_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "r3000a.h"
#include "psxcommon.h"
#include "psxmem.h"
#include "plugins.h"
#include "psemu_plugin_defs.h"

#define BUFFER_SIZE 0x1010
#define DONGLE_SIZE 0x40 * 0x1000
#define MCD_SIZE	(1024 * 8 * 16)

typedef	struct
{
	uint8_t				Buffer[BUFFER_SIZE];

	uint16_t			Status;	//This had a default, its gone now, but should probably come back
	uint16_t			Mode;
	uint16_t			Control;
	uint16_t			Baud;

	uint32_t			bufcount;
	uint32_t			parp;
	uint32_t			mcdst, rdwr;
	uint8_t				adrH, adrL;
	uint32_t			padst;
	uint32_t			gsdonglest;
	int8_t				Mcd1Data[MCD_SIZE], Mcd2Data[MCD_SIZE];
	uint32_t			DongleBank;
	uint8_t				DongleData[DONGLE_SIZE];
	int32_t				DongleInit;
}	PSXSIO_DataDef;
extern PSXSIO_DataDef PSXSIO_Data;

//Earmarked for removal
#define Mcd1Data	PSXSIO_Data.Mcd1Data
#define Mcd2Data	PSXSIO_Data.Mcd2Data

void sioWrite8(unsigned char value);

static inline void			PSXSIO_WriteStatus				(uint16_t aValue)	{}
static inline void			PSXSIO_WriteMode				(uint16_t aValue)	{PSXSIO_Data.Mode = aValue;}
static inline void			PSXSIO_WriteBaud				(uint16_t aValue)	{PSXSIO_Data.Baud = aValue;}
void						PSXSIO_WriteControl				(uint16_t aValue);


unsigned char sioRead8();
static inline uint16_t		PSXSIO_ReadStatus				()		{return PSXSIO_Data.Status;}
static inline uint16_t		PSXSIO_ReadMode					()		{return PSXSIO_Data.Mode;}
static inline uint16_t		PSXSIO_ReadControl				()		{return PSXSIO_Data.Control;}
static inline uint16_t		PSXSIO_ReadBaud					()		{return PSXSIO_Data.Baud;}

#if 0
Read Status:
	// wait for IRQ first
	if( psxRegs.interrupt & (1 << PSXINT_SIO) )
	{
		hard &= ~TX_RDY;
		hard &= ~RX_RDY;
		hard &= ~TX_EMPTY;
	}
#endif


void sioInterrupt();
int sioFreeze(gzFile f, int Mode);

void LoadMcd(int mcd, char *str);
void LoadMcds(char *mcd1, char *mcd2);
void SaveMcd(char *mcd, char *data, uint32_t adr, int size);
void CreateMcd(char *mcd);
void ConvertMcd(char *mcd, char *data);

typedef struct {
	char Title[48 + 1]; // Title in ASCII
	char sTitle[48 * 2 + 1]; // Title in Shift-JIS
	char ID[12 + 1];
	char Name[16 + 1];
	int IconCount;
	short Icon[16 * 16 * 3];
	unsigned char Flags;
} McdBlock;

void GetMcdBlockInfo(int mcd, int block, McdBlock *info);

#ifdef __cplusplus
}
#endif
#endif
