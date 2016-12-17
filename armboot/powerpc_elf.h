/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	PowerPC ELF file loading

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2009			Andre Heider "dhewg" <dhewg@wiibrew.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __POWERPC_ELF_H__
#define __POWERPC_ELF_H__

u32 virtualToPhysical(u32 virtualAddress);
u32 makeRelativeBranch(u32 currAddr, u32 destAddr, bool linked);
u32 makeAbsoluteBranch(u32 destAddr, bool linked);
void powerpc_jump_stub(u32 location, u32 entry);
void write_stub(u32 address, const u32 stub[], u32 size);
void powerpc_upload_array (const unsigned char* which, u32 where, const u32 len);
int powerpc_load_dol(const char *path, u32 *entry);
int powerpc_load_elf(const char* path);
int powerpc_boot_file(const char *path);
int powerpc_boot_mem(const u8 *addr, u32 len);

#endif

