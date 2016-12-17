/**************************************************************************************
***																					***
*** nswitch - Simple neek/realnand switcher to embed in a channel					***
***																					***
*** Copyright (C) 2011-2013  OverjoY												***
*** 																				***
*** This program is free software; you can redistribute it and/or					***
*** modify it under the terms of the GNU General Public License						***
*** as published by the Free Software Foundation version 2.							***
***																					***
*** This program is distributed in the hope that it will be useful,					***
*** but WITHOUT ANY WARRANTY; without even the implied warranty of					***
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the					***
*** GNU General Public License for more details.									***
***																					***
*** You should have received a copy of the GNU General Public License				***
*** along with this program; if not, write to the Free Software						***
*** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA. ***
***																					***
**************************************************************************************/

#include <gccore.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <ogc/machine/processor.h>

#define le32(i) (((((u32) i) & 0xFF) << 24) | ((((u32) i) & 0xFF00) << 8) | \
                ((((u32) i) & 0xFF0000) >> 8) | ((((u32) i) & 0xFF000000) >> 24))
#define MEM_REG_BASE 0xd8b4000
#define MEM_PROT (MEM_REG_BASE + 0x20a)

typedef struct {
	u32 hdrsize;
	u32 loadersize;
	u32 elfsize;
	u32 argument;
} ioshdr;

typedef struct _PR 
{
    u8 state;                            
    u8 chs_st[3];                        
    u8 type;                              
    u8 chs_e[3];                         
    u32 lba;                         
    u32 bc;                       
} __attribute__((__packed__)) _pr;

typedef struct _MBR
{
    u8 ca[446];               
    _pr part[4]; 
    u16 sig;                       
} __attribute__((__packed__)) _mbr;

static void disable_memory_protection() {
	write32(MEM_PROT, read32(MEM_PROT) & 0x0000FFFF);
}

int main() 
{	s32 ESHandle = IOS_Open("/dev/es", 0);
	bool neek = IOS_Ioctlv(ESHandle, 0xA2, 0, 0, NULL) == 0x666c6f77;
	IOS_Close(ESHandle);
	if(!neek)
	{	if ((read32(0xCD800038) | read32(0xCD80003C))==0) // if(no AHB access)
		{	SYS_ResetSystem( SYS_RETURNTOMENU, 0, 0 );
			return 0;
		}
		
		bool KernelFound = false;
		FILE *f = NULL;
		int retry = 0;
		
		while(retry < 10)
		{	if(__io_usbstorage.startup() && __io_usbstorage.isInserted())
				break;
			
			retry++;
			usleep(150000);
		}
		
		if(retry < 10)
		{	_mbr mbr;
			char buffer[4096];
			
			__io_usbstorage.readSectors(0, 1, &mbr);
			
			if(mbr.part[1].type != 0)
			{	__io_usbstorage.readSectors(le32(mbr.part[1].lba), 1, buffer);
				
				if(*((u16*)(buffer + 0x1FE)) == 0x55AA)
				{	if(memcmp(buffer + 0x36, "FAT", 3) == 0 || memcmp(buffer + 0x52, "FAT", 3) == 0)
					{	fatMount("usb", &__io_usbstorage, le32(mbr.part[1].lba), 8, 64);
						f = fopen("usb:/sneek/kernel.bin", "rb");
					}
				}
			}
		}
		
		if(!f)
			if(__io_wiisd.startup() || !__io_wiisd.isInserted())
				if(fatMount("sd", &__io_wiisd, 0, 8, 64))
					f = fopen("sd:/sneek/kernel.bin", "rb");
	
		if(f)
		{	fseek(f , 0 , SEEK_END);
			long fsize = ftell(f);
			rewind(f);
			fread((void *)0x91000000, 1, fsize, f);
			((ioshdr*)0x91000000)->argument = 0x42;
			DCFlushRange((void *)0x91000000, fsize);
			KernelFound = true;
		}

		fclose(f);
		fatUnmount("sd:");
		fatUnmount("usb:");
		__io_usbstorage.shutdown();
		__io_wiisd.shutdown();
		
		if(!KernelFound)
		{	SYS_ResetSystem( SYS_RETURNTOMENU, 0, 0 );
			return 0;
		}
		/** boot mini without BootMii IOS code by Crediar. **/
		disable_memory_protection();
		int i = 0x939F02F0;
		unsigned char ES_ImportBoot2[16] =
			{ 0x68, 0x4B, 0x2B, 0x06, 0xD1, 0x0C, 0x68, 0x8B, 0x2B, 0x00, 0xD1, 0x09, 0x68, 0xC8, 0x68, 0x42 };
		
		if( memcmp( (void*)(i), ES_ImportBoot2, sizeof(ES_ImportBoot2) ) != 0 )
			for( i = 0x939F0000; i < 0x939FE000; i+=4 )
				if( memcmp( (void*)(i), ES_ImportBoot2, sizeof(ES_ImportBoot2) ) == 0 )
					break;
		
		if(i >= 0x939FE000)
		{	SYS_ResetSystem( SYS_RETURNTOMENU, 0, 0 );
			return 0;
		}
		
		DCInvalidateRange( (void*)i, 0x20 );
		
		*(vu32*)(i+0x00)	= 0x48034904;	// LDR R0, 0x10, LDR R1, 0x14
		*(vu32*)(i+0x04)	= 0x477846C0;	// BX PC, NOP
		*(vu32*)(i+0x08)	= 0xE6000870;	// SYSCALL
		*(vu32*)(i+0x0C)	= 0xE12FFF1E;	// BLR
		*(vu32*)(i+0x10)	= 0x11000000;	// offset
		*(vu32*)(i+0x14)	= 0x0000FF01;	// version

		DCFlushRange( (void*)i, 0x20 );
		__IOS_ShutdownSubsystems();
		
		s32 fd = IOS_Open( "/dev/es", 0 );
		
		u8 *buffer = (u8*)memalign( 32, 0x100 );
		memset( buffer, 0, 0x100 );
		
		IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL );
		return 0;
	}
	else
	{	SYS_ResetSystem(SYS_RESTART, 0, 0);
		return 0;
	}
}
