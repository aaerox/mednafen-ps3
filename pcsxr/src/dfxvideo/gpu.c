//Lazy updates fix? Gone I guess

/***************************************************************************
                          gpu.c  -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#define _IN_GPU

#include <string.h>
#include <stdlib.h>

#include "externals.h"
#include "gpu.h"
#include "prim.h"
#include "stdint.h"
#include "psemu_plugin_defs.h"
#include "swap.h"

//From elsewhere
unsigned int		dwGPUVersion=0;
int					iGPUHeight=512;
int					iGPUHeightMask=511;
int					GlobalTextIL=0;
int					iTileCheat=0;
BOOL				bCheckMask;
unsigned short		sSetMask;
unsigned long		lSetMask;

////////////////////////////////////////////////////////////////////////
// memory image of the PSX vram 
////////////////////////////////////////////////////////////////////////
unsigned char		*psxVSecure;
unsigned char		*psxVub;
signed   char		*psxVsb;
unsigned short		*psxVuw;
unsigned short		*psxVuw_eom;
signed   short		*psxVsw;
uint32_t			*psxVul;
int32_t				*psxVsl;

////////////////////////////////////////////////////////////////////////
// GPU globals
////////////////////////////////////////////////////////////////////////

static long				lGPUdataRet;
long					lGPUstatusRet;
uint32_t				ulStatusControl[256];

//Command buffer data
static uint32_t			gpuDataM[256];
static unsigned char	gpuCommand = 0;
static long				gpuDataC = 0;
static long				gpuDataP = 0;

//Block transfer value
VRAMLoad_t				VRAMWrite;
VRAMLoad_t				VRAMRead;
DATAREGISTERMODES		DataWriteMode;
DATAREGISTERMODES		DataReadMode;

short					sDispWidths[8] = {256,320,512,640,368,384,512,640};
PSXDisplay_t			PSXDisplay;
PSXDisplay_t			PreviousPSXDisplay;
uint32_t				lGPUInfoVals[16];
static int				iFakePrimBusy=0;
uint32_t        		vBlank=0;


////////////////////////////////////////////////////////////////////////
// INIT, will be called after lib load... well, just do some var init...
////////////////////////////////////////////////////////////////////////

long CALLBACK laGPUinit()                                // GPU INIT
{
 memset(ulStatusControl,0,256*sizeof(uint32_t));  // init save state scontrol field

 psxVSecure = (unsigned char *)malloc((iGPUHeight*2)*1024 + (1024*1024)); // always alloc one extra MB for soft drawing funcs security
 if (!psxVSecure)
  return -1;

 //!!! ATTENTION !!!
 psxVub=psxVSecure + 512 * 1024;                           // security offset into double sized psx vram!

 psxVsb=(signed char *)psxVub;                         // different ways of accessing PSX VRAM
 psxVsw=(signed short *)psxVub;
 psxVsl=(int32_t *)psxVub;
 psxVuw=(unsigned short *)psxVub;
 psxVul=(uint32_t *)psxVub;

 psxVuw_eom=psxVuw+1024*iGPUHeight;                    // pre-calc of end of vram

 memset(psxVSecure,0x00,(iGPUHeight*2)*1024 + (1024*1024));
 memset(lGPUInfoVals,0x00,16*sizeof(uint32_t));

 PSXDisplay.RGB24        = FALSE;                      // init some stuff
 PSXDisplay.Interlaced   = FALSE;
 PSXDisplay.DrawOffset.x = 0;
 PSXDisplay.DrawOffset.y = 0;
 PSXDisplay.DisplayMode.x= 320;
 PSXDisplay.DisplayMode.y= 240;
 PreviousPSXDisplay.DisplayMode.x= 320;
 PreviousPSXDisplay.DisplayMode.y= 240;
 PSXDisplay.Disabled     = FALSE;
 PreviousPSXDisplay.Range.x0 =0;
 PreviousPSXDisplay.Range.y0 =0;
 PSXDisplay.Range.x0=0;
 PSXDisplay.Range.x1=0;
 PreviousPSXDisplay.DisplayModeNew.y=0;
 PSXDisplay.Double = 1;
 lGPUdataRet = 0x400;

 DataWriteMode = DR_NORMAL;

 // Reset transfer values, to prevent mis-transfer of data
 memset(&VRAMWrite, 0, sizeof(VRAMLoad_t));
 memset(&VRAMRead, 0, sizeof(VRAMLoad_t));
 
 // device initialised already !
 lGPUstatusRet = 0x14802000;
 GPUIsIdle;
 GPUIsReadyForCommands;
 bDoVSyncUpdate = TRUE;
 vBlank = 0;

 return 0;
}

////////////////////////////////////////////////////////////////////////
// Here starts all...
////////////////////////////////////////////////////////////////////////
long laGPUopen(unsigned long * disp,char * CapText,char * CfgFile)
{
 bDoVSyncUpdate = TRUE;

 return 0;
}

////////////////////////////////////////////////////////////////////////
// roughly emulated screen centering bits... not complete !!!
////////////////////////////////////////////////////////////////////////
void ChangeDispOffsetsX(void)                          // X CENTER
{
 long lx,l;

 if(!PSXDisplay.Range.x1) return;

 l=PreviousPSXDisplay.DisplayMode.x;

 l*=(long)PSXDisplay.Range.x1;
 l/=2560;lx=l;l&=0xfffffff8;

 if(l==PreviousPSXDisplay.Range.y1) return;            // abusing range.y1 for
 PreviousPSXDisplay.Range.y1=(short)l;                 // storing last x range and test

 if(lx>=PreviousPSXDisplay.DisplayMode.x)
  {
   PreviousPSXDisplay.Range.x1=
    (short)PreviousPSXDisplay.DisplayMode.x;
   PreviousPSXDisplay.Range.x0=0;
  }
 else
  {
   PreviousPSXDisplay.Range.x1=(short)l;

   PreviousPSXDisplay.Range.x0=
    (PSXDisplay.Range.x0-500)/8;

   if(PreviousPSXDisplay.Range.x0<0)
    PreviousPSXDisplay.Range.x0=0;

   if((PreviousPSXDisplay.Range.x0+lx)>
      PreviousPSXDisplay.DisplayMode.x)
    {
     PreviousPSXDisplay.Range.x0=
      (short)(PreviousPSXDisplay.DisplayMode.x-lx);
     PreviousPSXDisplay.Range.x0+=2; //???

     PreviousPSXDisplay.Range.x1+=(short)(lx-l);

     PreviousPSXDisplay.Range.x1-=2; // makes linux stretching easier
    }

   // some linux alignment security
   PreviousPSXDisplay.Range.x0=PreviousPSXDisplay.Range.x0>>1;
   PreviousPSXDisplay.Range.x0=PreviousPSXDisplay.Range.x0<<1;
   PreviousPSXDisplay.Range.x1=PreviousPSXDisplay.Range.x1>>1;
   PreviousPSXDisplay.Range.x1=PreviousPSXDisplay.Range.x1<<1;
  }

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////

void ChangeDispOffsetsY(void)                          // Y CENTER
{
 int iT,iO=PreviousPSXDisplay.Range.y0;
 int iOldYOffset=PreviousPSXDisplay.DisplayModeNew.y;

// new

 if((PreviousPSXDisplay.DisplayModeNew.x+PSXDisplay.DisplayModeNew.y)>iGPUHeight)
  {
   int dy1=iGPUHeight-PreviousPSXDisplay.DisplayModeNew.x;
   int dy2=(PreviousPSXDisplay.DisplayModeNew.x+PSXDisplay.DisplayModeNew.y)-iGPUHeight;

   if(dy1>=dy2)
    {
     PreviousPSXDisplay.DisplayModeNew.y=-dy2;
    }
   else
    {
     PSXDisplay.DisplayPosition.y=0;
     PreviousPSXDisplay.DisplayModeNew.y=-dy1;
    }
  }
 else PreviousPSXDisplay.DisplayModeNew.y=0;

// eon

 if(PreviousPSXDisplay.DisplayModeNew.y!=iOldYOffset) // if old offset!=new offset: recalc height
  {
   PSXDisplay.Height = PSXDisplay.Range.y1 - 
                       PSXDisplay.Range.y0 +
                       PreviousPSXDisplay.DisplayModeNew.y;
   PSXDisplay.DisplayModeNew.y=PSXDisplay.Height*PSXDisplay.Double;
  }

//

 if(PSXDisplay.PAL) iT=48; else iT=28;

 if(PSXDisplay.Range.y0>=iT)
  {
   PreviousPSXDisplay.Range.y0=
    (short)((PSXDisplay.Range.y0-iT-4)*PSXDisplay.Double);
   if(PreviousPSXDisplay.Range.y0<0)
    PreviousPSXDisplay.Range.y0=0;
   PSXDisplay.DisplayModeNew.y+=
    PreviousPSXDisplay.Range.y0;
  }
 else 
  PreviousPSXDisplay.Range.y0=0;
}

////////////////////////////////////////////////////////////////////////
// update lace is called evry VSync
////////////////////////////////////////////////////////////////////////

void CALLBACK laGPUupdateLace(void)                      // VSYNC
{
 //if(!(dwActFixes&1))
  //lGPUstatusRet^=0x80000000;                           // odd/even bit

 if(PSXDisplay.Interlaced)                             // interlaced mode?
  {
   lGPUstatusRet^=0x80000000;
  }

 bDoVSyncUpdate=FALSE;                                 // vsync done
}

////////////////////////////////////////////////////////////////////////
// process read request from GPU status register
////////////////////////////////////////////////////////////////////////


uint32_t CALLBACK laGPUreadStatus(void)             // READ STATUS
{
 if(dwActFixes&1)
  {
   static int iNumRead=0;                         // odd/even hack
   if((iNumRead++)==2)
    {
     iNumRead=0;
     lGPUstatusRet^=0x80000000;                   // interlaced bit toggle... we do it on every 3 read status... needed by some games (like ChronoCross) with old epsxe versions (1.5.2 and older)
    }
  }

 if(iFakePrimBusy)                                // 27.10.2007 - PETE : emulating some 'busy' while drawing... pfff
  {
   iFakePrimBusy--;

   if(iFakePrimBusy&1)                            // we do a busy-idle-busy-idle sequence after/while drawing prims
    {
     GPUIsBusy;
     GPUIsNotReadyForCommands;
    }
   else
    {
     GPUIsIdle;
     GPUIsReadyForCommands;
    }
  }
 return lGPUstatusRet | (vBlank ? 0x80000000 : 0 );
}

////////////////////////////////////////////////////////////////////////
// processes data send to GPU status register
// these are always single packet commands.
////////////////////////////////////////////////////////////////////////

void CALLBACK laGPUwriteStatus(uint32_t gdata)      // WRITE STATUS
{
 uint32_t lCommand=(gdata>>24)&0xff;

 ulStatusControl[lCommand]=gdata;                      // store command for freezing

 switch(lCommand)
  {
   //--------------------------------------------------//
   // reset gpu
   case 0x00:
    memset(lGPUInfoVals,0x00,16*sizeof(uint32_t));
    lGPUstatusRet=0x14802000;
    PSXDisplay.Disabled=1;
    DataWriteMode=DataReadMode=DR_NORMAL;
    PSXDisplay.DrawOffset.x=PSXDisplay.DrawOffset.y=0;
    drawX=drawY=0;drawW=drawH=0;
    sSetMask=0;lSetMask=0;bCheckMask=FALSE;
    usMirror=0;
    GlobalTextAddrX=0;GlobalTextAddrY=0;
    GlobalTextTP=0;GlobalTextABR=0;
    PSXDisplay.RGB24=FALSE;
    PSXDisplay.Interlaced=FALSE;
    bUsingTWin = FALSE;
    return;
   //--------------------------------------------------//
   // dis/enable display 
   case 0x03:  

    PreviousPSXDisplay.Disabled = PSXDisplay.Disabled;
    PSXDisplay.Disabled = (gdata & 1);

    if(PSXDisplay.Disabled) 
         lGPUstatusRet|=GPUSTATUS_DISPLAYDISABLED;
    else lGPUstatusRet&=~GPUSTATUS_DISPLAYDISABLED;
    return;

   //--------------------------------------------------//
   // setting transfer mode
   case 0x04:
    gdata &= 0x03;                                     // Only want the lower two bits

    DataWriteMode=DataReadMode=DR_NORMAL;
    if(gdata==0x02) DataWriteMode=DR_VRAMTRANSFER;
    if(gdata==0x03) DataReadMode =DR_VRAMTRANSFER;
    lGPUstatusRet&=~GPUSTATUS_DMABITS;                 // Clear the current settings of the DMA bits
    lGPUstatusRet|=(gdata << 29);                      // Set the DMA bits according to the received data

    return;
   //--------------------------------------------------//
   // setting display position
   case 0x05: 
    {
     PreviousPSXDisplay.DisplayPosition.x = PSXDisplay.DisplayPosition.x;
     PreviousPSXDisplay.DisplayPosition.y = PSXDisplay.DisplayPosition.y;

////////
/*
     PSXDisplay.DisplayPosition.y = (short)((gdata>>10)&0x3ff);
     if (PSXDisplay.DisplayPosition.y & 0x200) 
      PSXDisplay.DisplayPosition.y |= 0xfffffc00;
     if(PSXDisplay.DisplayPosition.y<0) 
      {
       PreviousPSXDisplay.DisplayModeNew.y=PSXDisplay.DisplayPosition.y/PSXDisplay.Double;
       PSXDisplay.DisplayPosition.y=0;
      }
     else PreviousPSXDisplay.DisplayModeNew.y=0;
*/

// new
     if(iGPUHeight==1024)
      {
       if(dwGPUVersion==2) 
            PSXDisplay.DisplayPosition.y = (short)((gdata>>12)&0x3ff);
       else PSXDisplay.DisplayPosition.y = (short)((gdata>>10)&0x3ff);
      }
     else PSXDisplay.DisplayPosition.y = (short)((gdata>>10)&0x1ff);

     // store the same val in some helper var, we need it on later compares
     PreviousPSXDisplay.DisplayModeNew.x=PSXDisplay.DisplayPosition.y;

     if((PSXDisplay.DisplayPosition.y+PSXDisplay.DisplayMode.y)>iGPUHeight)
      {
       int dy1=iGPUHeight-PSXDisplay.DisplayPosition.y;
       int dy2=(PSXDisplay.DisplayPosition.y+PSXDisplay.DisplayMode.y)-iGPUHeight;

       if(dy1>=dy2)
        {
         PreviousPSXDisplay.DisplayModeNew.y=-dy2;
        }
       else
        {
         PSXDisplay.DisplayPosition.y=0;
         PreviousPSXDisplay.DisplayModeNew.y=-dy1;
        }
      }
     else PreviousPSXDisplay.DisplayModeNew.y=0;
// eon

     PSXDisplay.DisplayPosition.x = (short)(gdata & 0x3ff);
     PSXDisplay.DisplayEnd.x=
      PSXDisplay.DisplayPosition.x+ PSXDisplay.DisplayMode.x;
     PSXDisplay.DisplayEnd.y=
      PSXDisplay.DisplayPosition.y+ PSXDisplay.DisplayMode.y + PreviousPSXDisplay.DisplayModeNew.y;
     PreviousPSXDisplay.DisplayEnd.x=
      PreviousPSXDisplay.DisplayPosition.x+ PSXDisplay.DisplayMode.x;
     PreviousPSXDisplay.DisplayEnd.y=
      PreviousPSXDisplay.DisplayPosition.y+ PSXDisplay.DisplayMode.y + PreviousPSXDisplay.DisplayModeNew.y;
 
     bDoVSyncUpdate=TRUE;

    }return;
   //--------------------------------------------------//
   // setting width
   case 0x06:

    PSXDisplay.Range.x0=(short)(gdata & 0x7ff);
    PSXDisplay.Range.x1=(short)((gdata>>12) & 0xfff);

    PSXDisplay.Range.x1-=PSXDisplay.Range.x0;

    ChangeDispOffsetsX();

    return;
   //--------------------------------------------------//
   // setting height
   case 0x07:
    {

     PSXDisplay.Range.y0=(short)(gdata & 0x3ff);
     PSXDisplay.Range.y1=(short)((gdata>>10) & 0x3ff);
                                      
     PreviousPSXDisplay.Height = PSXDisplay.Height;

     PSXDisplay.Height = PSXDisplay.Range.y1 - 
                         PSXDisplay.Range.y0 +
                         PreviousPSXDisplay.DisplayModeNew.y;

    }
   //--------------------------------------------------//
   // setting display infos
   case 0x08:

    PSXDisplay.DisplayModeNew.x =
     sDispWidths[(gdata & 0x03) | ((gdata & 0x40) >> 4)];

    if (gdata&0x04) PSXDisplay.Double=2;
    else            PSXDisplay.Double=1;

    PSXDisplay.DisplayModeNew.y = PSXDisplay.Height*PSXDisplay.Double;

    ChangeDispOffsetsY();

    PSXDisplay.PAL           = (gdata & 0x08)?TRUE:FALSE; // if 1 - PAL mode, else NTSC
    PSXDisplay.RGB24New      = (gdata & 0x10)?TRUE:FALSE; // if 1 - TrueColor
    PSXDisplay.InterlacedNew = (gdata & 0x20)?TRUE:FALSE; // if 1 - Interlace

    lGPUstatusRet&=~GPUSTATUS_WIDTHBITS;                   // Clear the width bits
    lGPUstatusRet|=
               (((gdata & 0x03) << 17) | 
               ((gdata & 0x40) << 10));                // Set the width bits

    if(PSXDisplay.InterlacedNew)
     {
      if(!PSXDisplay.Interlaced)
       {
        PreviousPSXDisplay.DisplayPosition.x = PSXDisplay.DisplayPosition.x;
        PreviousPSXDisplay.DisplayPosition.y = PSXDisplay.DisplayPosition.y;
       }
      lGPUstatusRet|=GPUSTATUS_INTERLACED;
     }
    else lGPUstatusRet&=~GPUSTATUS_INTERLACED;

    if (PSXDisplay.PAL)
         lGPUstatusRet|=GPUSTATUS_PAL;
    else lGPUstatusRet&=~GPUSTATUS_PAL;

    if (PSXDisplay.Double==2)
         lGPUstatusRet|=GPUSTATUS_DOUBLEHEIGHT;
    else lGPUstatusRet&=~GPUSTATUS_DOUBLEHEIGHT;

    if (PSXDisplay.RGB24New)
         lGPUstatusRet|=GPUSTATUS_RGB24;
    else lGPUstatusRet&=~GPUSTATUS_RGB24;

    return;
   //--------------------------------------------------//
   // ask about GPU version and other stuff
   case 0x10: 

    gdata&=0xff;

    switch(gdata) 
     {
      case 0x02:
       lGPUdataRet=lGPUInfoVals[INFO_TW];              // tw infos
       return;
      case 0x03:
       lGPUdataRet=lGPUInfoVals[INFO_DRAWSTART];       // draw start
       return;
      case 0x04:
       lGPUdataRet=lGPUInfoVals[INFO_DRAWEND];         // draw end
       return;
      case 0x05:
      case 0x06:
       lGPUdataRet=lGPUInfoVals[INFO_DRAWOFF];         // draw offset
       return;
      case 0x07:
       if(dwGPUVersion==2)
            lGPUdataRet=0x01;
       else lGPUdataRet=0x02;                          // gpu type
       return;
      case 0x08:
      case 0x0F:                                       // some bios addr?
       lGPUdataRet=0xBFC03720;
       return;
     }
    return;
   //--------------------------------------------------//
  }   
}

////////////////////////////////////////////////////////////////////////
// vram read/write helpers, needed by LEWPY's optimized vram read/write :)
////////////////////////////////////////////////////////////////////////

__inline void FinishedVRAMWrite(void)
{
 // Set register to NORMAL operation
 DataWriteMode = DR_NORMAL;
 // Reset transfer values, to prevent mis-transfer of data
 VRAMWrite.x = 0;
 VRAMWrite.y = 0;
 VRAMWrite.Width = 0;
 VRAMWrite.Height = 0;
 VRAMWrite.ColsRemaining = 0;
 VRAMWrite.RowsRemaining = 0;
}

__inline void FinishedVRAMRead(void)
{
 // Set register to NORMAL operation
 DataReadMode = DR_NORMAL;
 // Reset transfer values, to prevent mis-transfer of data
 VRAMRead.x = 0;
 VRAMRead.y = 0;
 VRAMRead.Width = 0;
 VRAMRead.Height = 0;
 VRAMRead.ColsRemaining = 0;
 VRAMRead.RowsRemaining = 0;

 // Indicate GPU is no longer ready for VRAM data in the STATUS REGISTER
 lGPUstatusRet&=~GPUSTATUS_READYFORVRAM;
}

////////////////////////////////////////////////////////////////////////
// core read from vram
////////////////////////////////////////////////////////////////////////

void CALLBACK laGPUreadDataMem(uint32_t * pMem, int iSize)
{
 int i;

 if(DataReadMode!=DR_VRAMTRANSFER) return;

 GPUIsBusy;

 // adjust read ptr, if necessary
 while(VRAMRead.ImagePtr>=psxVuw_eom)
  VRAMRead.ImagePtr-=iGPUHeight*1024;
 while(VRAMRead.ImagePtr<psxVuw)
  VRAMRead.ImagePtr+=iGPUHeight*1024;

 for(i=0;i<iSize;i++)
  {
   // do 2 seperate 16bit reads for compatibility (wrap issues)
   if ((VRAMRead.ColsRemaining > 0) && (VRAMRead.RowsRemaining > 0))
    {
     // lower 16 bit
     lGPUdataRet=(uint32_t)GETLE16(VRAMRead.ImagePtr);

     VRAMRead.ImagePtr++;
     if(VRAMRead.ImagePtr>=psxVuw_eom) VRAMRead.ImagePtr-=iGPUHeight*1024;
     VRAMRead.RowsRemaining --;

     if(VRAMRead.RowsRemaining<=0)
      {
       VRAMRead.RowsRemaining = VRAMRead.Width;
       VRAMRead.ColsRemaining--;
       VRAMRead.ImagePtr += 1024 - VRAMRead.Width;
       if(VRAMRead.ImagePtr>=psxVuw_eom) VRAMRead.ImagePtr-=iGPUHeight*1024;
      }

     // higher 16 bit (always, even if it's an odd width)
     lGPUdataRet|=(uint32_t)GETLE16(VRAMRead.ImagePtr)<<16;
     PUTLE32(pMem, lGPUdataRet); pMem++;

     if(VRAMRead.ColsRemaining <= 0)
      {FinishedVRAMRead();goto ENDREAD;}

     VRAMRead.ImagePtr++;
     if(VRAMRead.ImagePtr>=psxVuw_eom) VRAMRead.ImagePtr-=iGPUHeight*1024;
     VRAMRead.RowsRemaining--;
     if(VRAMRead.RowsRemaining<=0)
      {
       VRAMRead.RowsRemaining = VRAMRead.Width;
       VRAMRead.ColsRemaining--;
       VRAMRead.ImagePtr += 1024 - VRAMRead.Width;
       if(VRAMRead.ImagePtr>=psxVuw_eom) VRAMRead.ImagePtr-=iGPUHeight*1024;
      }
     if(VRAMRead.ColsRemaining <= 0)
      {FinishedVRAMRead();goto ENDREAD;}
    }
   else {FinishedVRAMRead();goto ENDREAD;}
  }

ENDREAD:
 GPUIsIdle;
}


////////////////////////////////////////////////////////////////////////

uint32_t CALLBACK laGPUreadData(void)
{
 uint32_t l;
 laGPUreadDataMem(&l,1);
 return lGPUdataRet;
}

////////////////////////////////////////////////////////////////////////
// processes data send to GPU data register
// extra table entries for fixing polyline troubles
////////////////////////////////////////////////////////////////////////

const unsigned char primTableCX[256] =
{
    // 00
    0,0,3,0,0,0,0,0,
    // 08
    0,0,0,0,0,0,0,0,
    // 10
    0,0,0,0,0,0,0,0,
    // 18
    0,0,0,0,0,0,0,0,
    // 20
    4,4,4,4,7,7,7,7,
    // 28
    5,5,5,5,9,9,9,9,
    // 30
    6,6,6,6,9,9,9,9,
    // 38
    8,8,8,8,12,12,12,12,
    // 40
    3,3,3,3,0,0,0,0,
    // 48
//  5,5,5,5,6,6,6,6,    // FLINE
    254,254,254,254,254,254,254,254,
    // 50
    4,4,4,4,0,0,0,0,
    // 58
//  7,7,7,7,9,9,9,9,    // GLINE
    255,255,255,255,255,255,255,255,
    // 60
    3,3,3,3,4,4,4,4,    
    // 68
    2,2,2,2,3,3,3,3,    // 3=SPRITE1???
    // 70
    2,2,2,2,3,3,3,3,
    // 78
    2,2,2,2,3,3,3,3,
    // 80
    4,0,0,0,0,0,0,0,
    // 88
    0,0,0,0,0,0,0,0,
    // 90
    0,0,0,0,0,0,0,0,
    // 98
    0,0,0,0,0,0,0,0,
    // a0
    3,0,0,0,0,0,0,0,
    // a8
    0,0,0,0,0,0,0,0,
    // b0
    0,0,0,0,0,0,0,0,
    // b8
    0,0,0,0,0,0,0,0,
    // c0
    3,0,0,0,0,0,0,0,
    // c8
    0,0,0,0,0,0,0,0,
    // d0
    0,0,0,0,0,0,0,0,
    // d8
    0,0,0,0,0,0,0,0,
    // e0
    0,1,1,1,1,1,1,0,
    // e8
    0,0,0,0,0,0,0,0,
    // f0
    0,0,0,0,0,0,0,0,
    // f8
    0,0,0,0,0,0,0,0
};

void CALLBACK laGPUwriteDataMem(uint32_t * pMem, int iSize)
{
 unsigned char command;
 uint32_t gdata=0;
 int i=0;
 GPUIsBusy;
 GPUIsNotReadyForCommands;

STARTVRAM:

 if(DataWriteMode==DR_VRAMTRANSFER)
  {
   BOOL bFinished=FALSE;

   // make sure we are in vram
   while(VRAMWrite.ImagePtr>=psxVuw_eom)
    VRAMWrite.ImagePtr-=iGPUHeight*1024;
   while(VRAMWrite.ImagePtr<psxVuw)
    VRAMWrite.ImagePtr+=iGPUHeight*1024;

   // now do the loop
   while(VRAMWrite.ColsRemaining>0)
    {
     while(VRAMWrite.RowsRemaining>0)
      {
       if(i>=iSize) {goto ENDVRAM;}
       i++;

       gdata=GETLE32(pMem); pMem++;

       PUTLE16(VRAMWrite.ImagePtr, (unsigned short)gdata); VRAMWrite.ImagePtr++;
       if(VRAMWrite.ImagePtr>=psxVuw_eom) VRAMWrite.ImagePtr-=iGPUHeight*1024;
       VRAMWrite.RowsRemaining --;

       if(VRAMWrite.RowsRemaining <= 0)
        {
         VRAMWrite.ColsRemaining--;
         if (VRAMWrite.ColsRemaining <= 0)             // last pixel is odd width
          {
           gdata=(gdata&0xFFFF)|(((uint32_t)GETLE16(VRAMWrite.ImagePtr))<<16);
           FinishedVRAMWrite();
           bDoVSyncUpdate=TRUE;
           goto ENDVRAM;
          }
         VRAMWrite.RowsRemaining = VRAMWrite.Width;
         VRAMWrite.ImagePtr += 1024 - VRAMWrite.Width;
        }

       PUTLE16(VRAMWrite.ImagePtr, (unsigned short)(gdata>>16)); VRAMWrite.ImagePtr++;
       if(VRAMWrite.ImagePtr>=psxVuw_eom) VRAMWrite.ImagePtr-=iGPUHeight*1024;
       VRAMWrite.RowsRemaining --;
      }

     VRAMWrite.RowsRemaining = VRAMWrite.Width;
     VRAMWrite.ColsRemaining--;
     VRAMWrite.ImagePtr += 1024 - VRAMWrite.Width;
     bFinished=TRUE;
    }

   FinishedVRAMWrite();
   if(bFinished) bDoVSyncUpdate=TRUE;
  }

ENDVRAM:

 if(DataWriteMode==DR_NORMAL)
  {
   void (* *primFunc)(unsigned char *);
   primFunc=primTableJ;

   for(;i<iSize;)
    {
     if(DataWriteMode==DR_VRAMTRANSFER) goto STARTVRAM;

     gdata=GETLE32(pMem); pMem++; i++;
 
     if(gpuDataC == 0)
      {
       command = (unsigned char)((gdata>>24) & 0xff);
 
//if(command>=0xb0 && command<0xc0) auxprintf("b0 %x!!!!!!!!!\n",command);

       if(primTableCX[command])
        {
         gpuDataC = primTableCX[command];
         gpuCommand = command;
         PUTLE32(&gpuDataM[0], gdata);
         gpuDataP = 1;
        }
       else continue;
      }
     else
      {
       PUTLE32(&gpuDataM[gpuDataP], gdata);
       if(gpuDataC>128)
        {
         if((gpuDataC==254 && gpuDataP>=3) ||
            (gpuDataC==255 && gpuDataP>=4 && !(gpuDataP&1)))
          {
           if((gpuDataM[gpuDataP] & 0xF000F000) == 0x50005000)
            gpuDataP=gpuDataC-1;
          }
        }
       gpuDataP++;
      }
 
     if(gpuDataP == gpuDataC)
      {
       gpuDataC=gpuDataP=0;
       primFunc[gpuCommand]((unsigned char *)gpuDataM);
       if(dwEmuFixes&0x0001 || dwActFixes&0x0400)      // hack for emulating "gpu busy" in some games
        iFakePrimBusy=4;
      }
    } 
  }

 lGPUdataRet=gdata;

 GPUIsReadyForCommands;
 GPUIsIdle;                
}

////////////////////////////////////////////////////////////////////////

void CALLBACK laGPUwriteData(uint32_t gdata)
{
 PUTLE32(&gdata, gdata);
 laGPUwriteDataMem(&gdata,1);
}

////////////////////////////////////////////////////////////////////////
// this functions will be removed soon (or 'soonish')... not really needed, but some emus want them
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// call config dlg
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// sets all kind of act fixes
////////////////////////////////////////////////////////////////////////

void SetFixes(void)
 {
  if(dwActFixes&0x02) sDispWidths[4]=384;
  else                sDispWidths[4]=368;
 }

////////////////////////////////////////////////////////////////////////
// process gpu commands
////////////////////////////////////////////////////////////////////////

unsigned long lUsedAddr[3];

__inline BOOL CheckForEndlessLoop(unsigned long laddr)
{
 if(laddr==lUsedAddr[1]) return TRUE;
 if(laddr==lUsedAddr[2]) return TRUE;

 if(laddr<lUsedAddr[0]) lUsedAddr[1]=laddr;
 else                   lUsedAddr[2]=laddr;
 lUsedAddr[0]=laddr;
 return FALSE;
}

long CALLBACK laGPUdmaChain(uint32_t * baseAddrL, uint32_t addr)
{
 uint32_t dmaMem;
 unsigned char * baseAddrB;
 short count;unsigned int DMACommandCounter = 0;

 GPUIsBusy;

 lUsedAddr[0]=lUsedAddr[1]=lUsedAddr[2]=0xffffff;

 baseAddrB = (unsigned char*) baseAddrL;

 do
  {
   if(iGPUHeight==512) addr&=0x1FFFFC;
   if(DMACommandCounter++ > 2000000) break;
   if(CheckForEndlessLoop(addr)) break;

   count = baseAddrB[addr+3];

   dmaMem=addr+4;

   if(count>0) laGPUwriteDataMem(&baseAddrL[dmaMem>>2],count);

   addr = GETLE32(&baseAddrL[addr>>2])&0xffffff;
  }
 while (addr != 0xffffff);

 GPUIsIdle;

 return 0;
}


typedef struct GPUFREEZETAG
{
 uint32_t ulFreezeVersion;      // should be always 1 for now (set by main emu)
 uint32_t ulStatus;             // current gpu status
 uint32_t ulControl[256];       // latest control register values
 unsigned char psxVRam[1024*1024*2]; // current VRam image (full 2 MB for ZN)
} GPUFreeze_t;

//TRIVIAL STUBS
void laGPUgetScreenPic(unsigned char * pMem){memset(pMem, 0, 128 * 96 * 3);}
void CALLBACK laGPUshowScreenPic(unsigned char * pMem){}
void CALLBACK laGPUsetfix(uint32_t dwFixBits){dwEmuFixes=dwFixBits;}
void CALLBACK laGPUvBlank( int val ){vBlank = val;}
void CALLBACK laGPUmakeSnapshot(void){}
long CALLBACK laGPUclose(){return 0;}
long CALLBACK laGPUshutdown(){free(psxVSecure);return 0;}
void CALLBACK laGPUcursor(int iPlayer,int x,int y){}
long CALLBACK laGPUfreeze(uint32_t ulGetFreezeData,GPUFreeze_t * pF){}
long CALLBACK laGPUtest(void){return 0;}
void CALLBACK laGPUabout(void){return;}
void CALLBACK laGPUsetMode(unsigned long gdata){}
long CALLBACK laGPUgetMode(void){ long iT=0; if(DataWriteMode==DR_VRAMTRANSFER) iT|=0x1; if(DataReadMode ==DR_VRAMTRANSFER) iT|=0x2; return iT;}
long CALLBACK laGPUconfigure(void){return 0;}
void CALLBACK lsGPUdisplayText(char * pText){}

