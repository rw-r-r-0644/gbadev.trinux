/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	PowerPC ELF file loading

Copyright (C) 2008, 2009        Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2009                      Andre Heider "dhewg" <dhewg@wiibrew.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "types.h"
#include "powerpc.h"
#include "hollywood.h"
#include "utils.h"
#include "start.h"
#include "gecko.h"
#include "ff.h"
#include "powerpc_elf.h"
#include "elf.h"
#include "memory.h"
#include "string.h"
#include "stubsb1.h"

extern u8 __mem2_area_start[];

#define PPC_MEM1_END    (0x017fffff)
#define PPC_MEM2_START  (0x10000000)
#define PPC_MEM2_END    ((u32) __mem2_area_start)

#define PHDR_MAX 10

typedef struct dol_t dol_t;
struct dol_t
{
	u32 offsetText[7];
	u32 offsetData[11];
	u32 addressText[7];
	u32 addressData[11];
	u32 sizeText[7];
	u32 sizeData[11];
	u32 addressBSS;
	u32 sizeBSS;
	u32 entrypt;
	u8 pad[0x1C];
};

static int _check_physaddr(u32 addr) {
	if ((addr >= PPC_MEM2_START) && (addr <= PPC_MEM2_END))
		return 2;

	if (addr < PPC_MEM1_END)
		return 1;

	return -1;
}

static int _check_physrange(u32 addr, u32 len) {
	switch (_check_physaddr(addr)) {
	case 1:
		if ((addr + len) < PPC_MEM1_END)
			return 1;
		break;
	case 2:
		if ((addr + len) < PPC_MEM2_END)
			return 2;
		break;
	}

	return -1;
}

static Elf32_Ehdr elfhdr;
static Elf32_Phdr phdrs[PHDR_MAX];

u32 virtualToPhysical(u32 virtualAddress)
{
	if ((virtualAddress & 0xC0000000) == 0xC0000000) return virtualAddress & ~0xC0000000;
	if ((virtualAddress & 0x80000000) == 0x80000000) return virtualAddress & ~0x80000000;
	return virtualAddress;
}

u32 makeRelativeBranch(u32 currAddr, u32 destAddr, bool linked)
{
	u32 ret = 0x48000000 | (( destAddr - currAddr ) & 0x3FFFFFC );
	if(linked)
		ret |= 1;
	return ret;
}

u32 makeAbsoluteBranch(u32 destAddr, bool linked)
{
	u32 ret = 0x48000002 | ( destAddr & 0x3FFFFFC );
	if(linked)
	ret |= 1;
	return ret;
}

///////////////////////////////// S T U B S /////////////////////////////////

//some needed init's and a jump to 0x1800 ???
//basically, MSR[ip]=0 + sync
const u32 stub_100_location = 0x100;
const u32 stub_100[] =
{
	/*0x100*/  0x38600000	//li r3,0
	/*0x104*/, 0x7C600124	//mtmsr r3
	/*0x108*/, 0x4C00012C	//isync
 	/*0x10c*/, 0x7c0004ac	//sync
							//eieio??
 	/*0x110*/, 0x48001802  //b 0x1800
};
const u32 stub_100_size = 5;



//another turn sensorbar on routine
const u32 stub_1800_2_size = 5;
const u32 stub_1800_2_location = 0x100;
const u32 stub_1800_2[] =
{
	/*0x1800*/  0x3c600d80	//lis r3,3456
	/*0x1804*/, 0x808300c0 	//lwz r4,192(r3)
	/*0x1808*/, 0x60840100 	//ori r4,r4,256
	/*0x180c*/, 0x908300c0	//stw r4,192(r3)
	/*0x1810*/, 0x48000000	//b 0x1810

};


//this one should flash the sensorbar
const u32 stub_1800_size = 26;
const u32 stub_1800_location = 0x100;
const u32 stub_1800[] =
{

	/*0x1800*/  0x38600005 //li   r3,5
	/*0x1804*/, 0x7c6903a6 //mtctr  r3
	
	//could this turn the sensorbar off?
	
	/*0x1808*/, 0x3c600d80 //lis r3,3456
	/*0x180c*/, 0x808300c0 //lwz r4,192(r3)
	/*0x1810*/, 0x3ca000ff //lis r5,255
	/*0x1814*/, 0x60a5feff //ori r5,r5,65279
	/*0x1818*/, 0x7c842838 //and r4,r4,r5
	/*0x181c*/, 0x908300c0 //stw r4,192(r3)
	
	//this will show up 2 times

	/*0x1820*/, 0x7c6c42e6 //mftbl  r3
	/*0x1824*/, 0x3ca001c9 //lis r5,457
	/*0x1828*/, 0x60a5c380 //ori r5,r5,50048
	/*0x182c*/, 0x7c8c42e6 //mftbl  r4
	/*0x1830*/, 0x7c832050 //subf r4,r3,r4
	/*0x1834*/, 0x7c042840 //cmplw  r4,r5
	/*0x1838*/, 0x4180fff4 //blt+ 0x12c ?
	
	//notice a resemblance with previous here?

	/*0x183c*/, 0x3c600d80 //lis r3,3456
	/*0x1840*/, 0x808300c0 //lwz r4,192(r3)
	/*0x1844*/, 0x60840100 //ori r4,r4,256
	/*0x1848*/, 0x908300c0 //stw r4,192(r3)
	
	//this is the second occurence
	
	/*0x184c*/, 0x7c6c42e6 //mftbl  r3
	/*0x1850*/, 0x3ca001c9 //lis r5,457
	/*0x1854*/, 0x60a5c380 //ori r5,r5,50048
	/*0x1858*/, 0x7c8c42e6 //mftbl  r4
	/*0x185c*/, 0x7c832050 //subf r4,r3,r4
	/*0x1860*/, 0x7c042840 //cmplw  r4,r5
	/*0x1864*/, 0x4180fff4 //blt+ 0x1858 ?



	/*0x1868*/, 0x4200ff98 //bdnz+  0x1800 ?
	/*0x186c*/, 0x48000000 // makeAbsoluteBranch(0x1870*/, false)

};


const u32 stub_1800_1_512[] =
{

//check_pvr_hi r4, 0x7001
    /*0x1800*/  0x7c9f42a6 //mfpvr   r4
    /*0x1804*/, 0x5484843e //rlwinm  r4,r4,16,16,31
    /*0x1808*/, 0x28047001 //cmplwi  r4,28673
//bne __real_start:
    /*0x180c*/, 0x408200b4 //bne-    0x18c0  0x408200f4 original

    /*0x1810*/, 0x7c79faa6 //mfl2cr  r3
    /*0x1814*/, 0x5463003e //rotlwi  r3,r3,0	rlwinm..?
    /*0x1818*/, 0x7c79fba6 //mtl2cr  r3
    /*0x181c*/, 0x7c0004ac //sync    
//spr_oris r3, l2cr, 0x0020		/* L2 global invalidate */
    /*0x1820*/, 0x7c79faa6 //mfl2cr  r3
    /*0x1824*/, 0x64630020 //oris    r3,r3,32
    /*0x1828*/, 0x7c79fba6 //mtl2cr  r3
//spr_check_bit r3, l2cr, 31, 0	/* L2IP */
//local_0:
    /*0x182c*/, 0x7c79faa6 //mfl2cr  r3
    /*0x1830*/, 0x546307fe //clrlwi  r3,r3,31

    /*0x1834*/, 0x2c030000 //cmpwi   r3,0
    /*0x1838*/, 0x4082fff4 //bne+    0x182c	local_0:
//spr_clear_bit r3, l2cr, 10		/* clear L2I */
    /*0x183c*/, 0x7c79faa6 //mfl2cr  r3
    /*0x1840*/, 0x546302d2 //rlwinm  r3,r3,0,11,9
    /*0x1844*/, 0x7c79fba6 //mtl2cr  r3

//spr_check_bit r3, l2cr, 31, 0	/* clear L2I */
//local_1:
    /*0x1848*/, 0x7c79faa6 //mfl2cr  r3
    /*0x184c*/, 0x546307fe //clrlwi  r3,r3,31
    /*0x1850*/, 0x2c030000 //cmpwi   r3,0
    /*0x1854*/, 0x4082fff4 //bne+    0x1848	local_1:

    /*0x1858*/, 0x48000004 //b       0x185c	start:	0x48000008 original

//original there is a 0x00000000 located here

//spr_clear_bit r3, hid5, 0		/* disable HID5 */
//start:
    /*0x185c*/, 0x7c70eaa6 //mfspr   r3,944	Ox3b0 hid5?
    /*0x1860*/, 0x5463007e //clrlwi  r3,r3,1
    /*0x1864*/, 0x7c70eba6 //mtspr   944,r3

    /*0x1868*/, 0x60000000 //nop
    /*0x186c*/, 0x7c0004ac //sync    
    /*0x1870*/, 0x60000000 //nop
    /*0x1874*/, 0x60000000 //nop
    /*0x1878*/, 0x60000000 //nop

//spr_oris r3, bcr, 0x1000
    /*0x187c*/, 0x7c75eaa6 //mfspr   r3,949
    /*0x1880*/, 0x64631000 //oris    r3,r3,4096
    /*0x1884*/, 0x7c75eba6 //mtspr   949,r3

    /*0x1888*/, 0x388000ff //li      r4,255
//delay loop?
//local_2:
    /*0x188c*/, 0x3884ffff //addi    r4,r4,-1
    /*0x1890*/, 0x2c040000 //cmpwi   r4,0
    /*0x1894*/, 0x4082fff8 //bne+    0x188c	local_2:

    /*0x1898*/, 0x60000000 //nop

//set_srr0_phys r3, __real_start
    /*0x189c*/, 0x3c600000 //lis     r3,0
    /*0x18a0*/, 0x606318c0 //ori     r3,r3,6336
    /*0x18a4*/, 0x5463007e //clrlwi  r3,r3,1
    /*0x18a8*/, 0x7c7a03a6 //mtsrr0  r3

    /*0x18ac*/, 0x38800000 //li      r4,0
    /*0x18b0*/, 0x7c9b03a6 //mtsrr1  r4
    /*0x18b4*/, 0x4c000064 //rfi
//.align 4
    /*0x18b8*/, 0x60000000 //nop
    /*0x18bc*/, 0x60000000 //nop

// in original code this starts at 0x01330200
// previous area is filled with 68 0x00 bytes

//__real_start:
//spr_set r4, hid0, 0x00110C64	/* DPM, NHR, ICFI, DCFI, DCFA, BTIC, and BHT */
    /*0x18c0*/, 0x3c800011 //lis     r4,17
    /*0x18c4*/, 0x60840c64 //ori     r4,r4,3172	0x0c64		   0x38840c64 original
    /*0x18c8*/, 0x7c90fba6 //mtspr   1008,r4		0x3f0 = hid0?
//msr_set r4, 0x2000				/* FP */
    /*0x18cc*/, 0x3c800000 //lis     r4,0
    /*0x18d0*/, 0x60842000 //ori     r4,r4,8192	0x2000		   0x38842000 original
    /*0x18d4*/, 0x7c800124 //mtmsr   r4
//spr_oris r4, hid4, 0x0200		/* enable SBE */
    /*0x18d8*/, 0x7c93faa6 //mfspr   r4,1011		 0x3f3 = hid4?
    /*0x18dc*/, 0x64840200 //oris    r4,r4,512	 0x548401ca original	
    /*0x18e0*/, 0x7c93fba6 //mtspr   1011,r4
//spr_ori r4, hid0, 0xC000		/* ICE, DCE */
    /*0x18e4*/, 0x7c90faa6 //mfspr   r4,1008		0x3f0 = hid0?  0x7c70faa6 original
    /*0x18e8*/, 0x6084c000 //ori     r4,r4,49152				   0x6054c000 original
    /*0x18ec*/, 0x7c90fba6 //mtspr   1008,r4
    /*0x18f0*/, 0x4c00012c //isync

//.irp b,0u,0l,1u,1l,2u,2l,3u,3l,4u,4l,5u,5l,6u,6l,7u,7l
    /*0x18f4*/, 0x38800000 //li      r4,0
    /*0x18f8*/, 0x7c9883a6 //mtdbatu 0,r4			mov to .. bat0u
    /*0x18fc*/, 0x7c9983a6 //mtdbatl 0,r4			mov to .. bat0l	 not in original?
    /*0x1900*/, 0x7c9a83a6 //mtdbatu 1,r4
    /*0x1904*/, 0x7c9b83a6 //mtdbatl 1,r4			not in original?
    /*0x1908*/, 0x7c9c83a6 //mtdbatu 2,r4
    /*0x190c*/, 0x7c9d83a6 //mtdbatl 2,r4
    /*0x1910*/, 0x7c9e83a6 //mtdbatu 3,r4
    /*0x1914*/, 0x7c9f83a6 //mtdbatl 3,r4
//mtsr sr\sr, r4
    /*0x1918*/, 0x7c988ba6 //mtspr   568,r4		move to spr 0x0238
    /*0x191c*/, 0x7c998ba6 //mtspr   569,r4		0x0239
    /*0x1920*/, 0x7c9a8ba6 //mtspr   570,r4
    /*0x1924*/, 0x7c9b8ba6 //mtspr   571,r4
    /*0x1928*/, 0x7c9c8ba6 //mtspr   572,r4
    /*0x192c*/, 0x7c9d8ba6 //mtspr   573,r4
    /*0x1930*/, 0x7c9e8ba6 //mtspr   574,r4
    /*0x1934*/, 0x7c9f8ba6 //mtspr   575,r4
//.irp b,0u,0l,1u,1l,2u,2l,3u,3l,4u,4l,5u,5l,6u,6l,7u,7l
    /*0x1938*/, 0x7c9083a6 //mtibatu 0,r4
    /*0x193c*/, 0x7c9183a6 //mtibatl 0,r4
    /*0x1940*/, 0x7c9283a6 //mtibatu 1,r4
    /*0x1944*/, 0x7c9383a6 //mtibatl 1,r4
    /*0x1948*/, 0x7c9483a6 //mtibatu 2,r4
    /*0x194c*/, 0x7c9583a6 //mtibatl 2,r4
    /*0x1950*/, 0x7c9683a6 //mtibatu 3,r4
    /*0x1954*/, 0x7c9783a6 //mtibatl 3,r4
//mtspr ibat\b, r4
    /*0x1958*/, 0x7c908ba6 //mtspr   560,r4
    /*0x195c*/, 0x7c918ba6 //mtspr   561,r4
    /*0x1960*/, 0x7c928ba6 //mtspr   562,r4
    /*0x1964*/, 0x7c938ba6 //mtspr   563,r4
    /*0x1968*/, 0x7c948ba6 //mtspr   564,r4
    /*0x196c*/, 0x7c958ba6 //mtspr   565,r4
    /*0x1970*/, 0x7c968ba6 //mtspr   566,r4
    /*0x1974*/, 0x7c978ba6 //mtspr   567,r4

    /*0x1978*/, 0x4c00012c //isync

//.irp sr,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
//			mtsr sr\sr, r4
    /*0x197c*/, 0x3c808000 //lis     r4,-32768
    /*0x1980*/, 0x60840000 //ori     r4,r4,0		original 0x38840000

    /*0x1984*/, 0x7c8001a4 //mtsr    0,r4			move to spr sr?
    /*0x1988*/, 0x7c8101a4 //mtsr    1,r4
    /*0x198c*/, 0x7c8201a4 //mtsr    2,r4
    /*0x1990*/, 0x7c8301a4 //mtsr    3,r4
    /*0x1994*/, 0x7c8401a4 //mtsr    4,r4
    /*0x1998*/, 0x7c8501a4 //mtsr    5,r4
    /*0x199c*/, 0x7c8601a4 //mtsr    6,r4
    /*0x19a0*/, 0x7c8701a4 //mtsr    7,r4
    /*0x19a4*/, 0x7c8801a4 //mtsr    8,r4
    /*0x19a8*/, 0x7c8901a4 //mtsr    9,r4
    /*0x19ac*/, 0x7c8a01a4 //mtsr    10,r4
    /*0x19b0*/, 0x7c8b01a4 //mtsr    11,r4
    /*0x19b4*/, 0x7c8c01a4 //mtsr    12,r4
    /*0x19b8*/, 0x7c8d01a4 //mtsr    13,r4
    /*0x19bc*/, 0x7c8e01a4 //mtsr    14,r4
    /*0x19c0*/, 0x7c8f01a4 //mtsr    15,r4

//set_bats r4, bat0l, 0x00000002		/* 0x00000000-0x10000000 : RW/RW Cached   */
    /*0x19c4*/, 0x3c800000 //lis     r4,0
    /*0x19c8*/, 0x60840002 //ori     r4,r4,2		original 0x38840002
    /*0x19cc*/, 0x7c9183a6 //mtibatl 0,r4			original not there
    /*0x19d0*/, 0x7c9983a6 //mtdbatl 0,r4			original not there
//set_bats r3, bat0u, 0x80001FFF		/* 0x80000000-0x90000000 :  1/ 1 :        */
    /*0x19d4*/, 0x3c608000 //lis     r3,-32768
    /*0x19d8*/, 0x60631fff //ori     r3,r3,8191	original 0x38631fff
    /*0x19dc*/, 0x7c7083a6 //mtibatu 0,r3			original 0x7c9983a6
    /*0x19e0*/, 0x7c7883a6 //mtdbatu 0,r3

    /*0x19e4*/, 0x4c00012c //isync
//set_bat r4, dbat1l, 0x0000002A		/* 0x00000000-0x10000000 : RW/RW Uncached */
    /*0x19e8*/, 0x3c800000 //lis     r4,0
    /*0x19ec*/, 0x6084002a //ori     r4,r4,42
    /*0x19f0*/, 0x7c9b83a6 //mtdbatl 1,r4
//set_bat r3, dbat1u, 0xC0001FFF		/* 0xC0000000-0xD0000000 :  1/ 1 : I/G    */
    /*0x19f4*/, 0x3c60c000 //lis     r3,-16384
    /*0x19f8*/, 0x60631fff //ori     r3,r3,8191
    /*0x19fc*/, 0x7c7a83a6 //mtdbatu 1,r3

    /*0x1a00*/, 0x4c00012c //isync
//set_bats r4, bat4l, 0x10000002		/* 0x10000000-0x20000000 : RW/RW Cached   */
    /*0x1a04*/, 0x3c801000 //lis     r4,4096
    /*0x1a08*/, 0x60840002 //ori     r4,r4,2
    /*0x1a0c*/, 0x7c918ba6 //mtspr   561,r4
    /*0x1a10*/, 0x7c998ba6 //mtspr   569,r4

//set_bats r3, bat4u, 0x90001FFF		/* 0x90000000-0xA0000000 :  1/ 1 :        */
    /*0x1a14*/, 0x3c609000 //lis     r3,-28672
    /*0x1a18*/, 0x60631fff //ori     r3,r3,8191
    /*0x1a1c*/, 0x7c708ba6 //mtspr   560,r3
    /*0x1a20*/, 0x7c788ba6 //mtspr   568,r3

    /*0x1a24*/, 0x4c00012c //isync
//set_bat r4, dbat5l, 0x1000002A		/* 0x10000000-0x20000000 : RW/RW Uncached */
    /*0x1a28*/, 0x3c801000 //lis     r4,4096
    /*0x1a2c*/, 0x6084002a //ori     r4,r4,42
    /*0x1a30*/, 0x7c9b8ba6 //mtspr   571,r4
//set_bat r3, dbat5u, 0xD0001FFF		/* 0xD0000000-0xE0000000 :  1/ 1 : I/G    */
    /*0x1a34*/, 0x3c60d000 //lis     r3,-12288
    /*0x1a38*/, 0x60631fff //ori     r3,r3,8191
    /*0x1a3c*/, 0x7c7a8ba6 //mtspr   570,r3

    /*0x1a40*/, 0x4c00012c //isync

    /*0x1a44*/, 0x38600000 //li      r3,0								original 0x3c600000
    /*0x1a48*/, 0x38800000 //li      r4,0
    /*0x1a4c*/, 0x908300f4 //stw     r4,244(r3)		BI2 0xf4


//set_srr0 r3, __KernelInit
//no instruction at 0x1a50 in megazig code so...?
    /*0x1a50*/, 0x60000000 //nop										original 0x3c608133
    /*0x1a54*/,	0x60633400 //ori     r3,r3,13312		/* 0x3400 */	original 0x60630400
    /*0x1a58*/, 0x7c7a03a6 //mtsrr0  r3

//so loading entry where original loaded kernel...
//this is from Maxternal I assume

//    write32(0x1a50, 0x3c600000 | entry >> 16 ); //lis     r3,entry@h
//    write32(0x1a54, 0x60630000 | (entry & 0xffff) ); //ori     r3,r3,entry@l
//    write32(0x1a58, 0x7c7a03a6); //mtsrr0  r3

//msr_to_srr1_ori r4, 0x30			/* enable DR|IR */
    /*0x1a5c*/, 0x7c8000a6 //mfmsr   r4
    /*0x1a60*/, 0x60840030 //ori     r4,r4,48		0x30
    /*0x1a64*/, 0x7c9b03a6 //mtsrr1  r4
    /*0x1a68*/, 0x4c000064 //rfi
};
const u32 stub_1800_1_512_size = sizeof(stub_1800_1_512) / 4;
const u32 stub_1800_1_512_location = 0x100;

void powerpc_jump_stub(u32 location, u32 entry)
{
//	u32 i;
	//u32 location = 0x01330100;
//	set32(HW_EXICTRL, EXICTRL_ENABLE_EXI);

	// lis r3, entry@h
	write32(location + 4 * 0, 0x3c600000 | entry >> 16);
	// ori r3, r3, entry@l
	write32(location + 4 * 1, 0x60630000 | (entry & 0xffff));
	// mtsrr0 r3
	write32(location + 4 * 2, 0x7c7a03a6);
	// li r3, 0
	write32(location + 4 * 3, 0x38600000);
	// mtsrr1 r3
	write32(location + 4 * 4, 0x7c7b03a6);
	// rfi
	write32(location + 4 * 5, 0x4c000064);

//	for (i = 6; i < 0x10; ++i)
//		write32(EXI_BOOT_BASE + 4 * i, 0);

//	set32(HW_DIFLAGS, DIFLAGS_BOOT_CODE);
//	set32(HW_AHBPROT, 0xFFFFFFFF);

//	gecko_printf("disabling EXI now...\n");
//	clear32(HW_EXICTRL, EXICTRL_ENABLE_EXI);
}

const u32 memory_watcher_size = 6;
const u32 memory_watcher_location = 0x1330100;
const u32 memory_watcher_stub[] =
{
//	write32(0x1330100, 0x3c600000); // lis r3,0
//	write32(0x1330104, 0x90831800); // stw r4,(0x1800)r3
//	write32(0x1330108, 0x48000000); // infinite loop

//	write32(0x1330100, 0x3c600133); // lis r3,0x0133
	/*0x1330100*/  0x48000005 // branch 1 instruction ahead and link, loading the address of the next instruction (0x1330104) into lr
	/*0x1330104*/, 0x7c6802a6 // mflr r3
	/*0x1330108*/, 0x90830014 // stw r4,(0x0014)r3
 	/*0x133010C*/, 0x7c0004ac // sync
	/*0x1330110*/, 0x48000000 // infinite loop
	/*0x1330114*/, 0x00000000
	/*0x1330118*/, 0xAAAAAAAA // flag location
};

//////////////////////////////// END STUBS ////////////////////////

void write_stub(u32 address, const u32 stub[], u32 size)
{	u32 i;
	for(i = 0; i < size; i++)
		write32(address + 4 * i, stub[i]);
}

int powerpc_load_dol(const char *path, u32 *entry)
{
	u32 read;
	FIL fd;
	FRESULT fres;
	dol_t dol_hdr;
	gecko_printf("Loading DOL file: %s .\n", path);
	fres = f_open(&fd, path, FA_READ);
	if (fres != FR_OK)
		return -fres;

	fres = f_read(&fd, &dol_hdr, sizeof(dol_t), &read);
	if (fres != FR_OK)
		return -fres;

	u32 end = 0;
	int ii;

	/* TEXT SECTIONS */
	for (ii = 0; ii < 7; ii++)
	{
		if (!dol_hdr.sizeText[ii])
			continue;
		fres = f_lseek(&fd, dol_hdr.offsetText[ii]);
		if (fres != FR_OK)
			return -fres;
		u32 phys = virtualToPhysical(dol_hdr.addressText[ii]);
		fres = f_read(&fd, (void*)phys, dol_hdr.sizeText[ii], &read);
		if (fres != FR_OK)
			return -fres;
		if (phys + dol_hdr.sizeText[ii] > end)
			end = phys + dol_hdr.sizeText[ii];
		gecko_printf("Text section of size %08x loaded from offset %08x to memory %08x.\n", dol_hdr.sizeText[ii], dol_hdr.offsetText[ii], phys);
		gecko_printf("Memory area starts with %08x and ends with %08x (at address %08x)\n", read32(phys), read32((phys+(dol_hdr.sizeText[ii] - 1)) & ~3),(phys+(dol_hdr.sizeText[ii] - 1)) & ~3);
	}

	/* DATA SECTIONS */
	for (ii = 0; ii < 11; ii++)
	{
		if (!dol_hdr.sizeData[ii])
			continue;
		fres = f_lseek(&fd, dol_hdr.offsetData[ii]);
		if (fres != FR_OK)
			return -fres;
		u32 phys = virtualToPhysical(dol_hdr.addressData[ii]);
		fres = f_read(&fd, (void*)phys, dol_hdr.sizeData[ii], &read);
		if (fres != FR_OK)
			return -fres;
		if (phys + dol_hdr.sizeData[ii] > end)
			end = phys + dol_hdr.sizeData[ii];
		gecko_printf("Data section of size %08x loaded from offset %08x to memory %08x.\n", dol_hdr.sizeData[ii], dol_hdr.offsetData[ii], phys);
		gecko_printf("Memory area starts with %08x and ends with %08x (at address %08x)\n", read32(phys), read32((phys+(dol_hdr.sizeData[ii] - 1)) & ~3),(phys+(dol_hdr.sizeData[ii] - 1)) & ~3);
	}
  *entry = dol_hdr.entrypt;
	return 0;
}

int powerpc_load_elf(const char* path)
{
	u32 read;
	FIL fd;
	FRESULT fres;
	
	fres = f_open(&fd, path, FA_READ);
	if (fres != FR_OK)
		return -fres;

	fres = f_read(&fd, &elfhdr, sizeof(elfhdr), &read);

	if (fres != FR_OK)
		return -fres;

	if (read != sizeof(elfhdr))
		return -100;

	if (memcmp("\x7F" "ELF\x01\x02\x01\x00\x00",elfhdr.e_ident,9)) {
		gecko_printf("Invalid ELF header! 0x%02x 0x%02x 0x%02x 0x%02x\n",elfhdr.e_ident[0], elfhdr.e_ident[1], elfhdr.e_ident[2], elfhdr.e_ident[3]);
		return -101;
	}

	if (_check_physaddr(elfhdr.e_entry) < 0) {
		gecko_printf("Invalid entry point! 0x%08x\n", elfhdr.e_entry);
		return -102;
	}

	if (elfhdr.e_phoff == 0 || elfhdr.e_phnum == 0) {
		gecko_printf("ELF has no program headers!\n");
		return -103;
	}

	if (elfhdr.e_phnum > PHDR_MAX) {
		gecko_printf("ELF has too many (%d) program headers!\n", elfhdr.e_phnum);
		return -104;
	}

	fres = f_lseek(&fd, elfhdr.e_phoff);
	if (fres != FR_OK)
		return -fres;

	fres = f_read(&fd, phdrs, sizeof(phdrs[0])*elfhdr.e_phnum, &read);
	if (fres != FR_OK)
		return -fres;

	if (read != sizeof(phdrs[0])*elfhdr.e_phnum)
		return -105;

	u16 count = elfhdr.e_phnum;
	Elf32_Phdr *phdr = phdrs;
	//powerpc_hang();
	while (count--) {
		if (phdr->p_type != PT_LOAD) {
			gecko_printf("Skipping PHDR of type %d\n", phdr->p_type);
		} else {
			if (_check_physrange(phdr->p_paddr, phdr->p_memsz) < 0) {
				gecko_printf("PHDR out of bounds [0x%08x...0x%08x]\n",
								phdr->p_paddr, phdr->p_paddr + phdr->p_memsz);
				return -106;
			}

			void *dst = (void *) phdr->p_paddr;

			gecko_printf("LOAD 0x%x @0x%08x [0x%x]\n", phdr->p_offset, phdr->p_paddr, phdr->p_filesz);
			fres = f_lseek(&fd, phdr->p_offset);
			if (fres != FR_OK)
				return -fres;
			fres = f_read(&fd, dst, phdr->p_filesz, &read);
			if (fres != FR_OK)
				return -fres;
			if (read != phdr->p_filesz)
				return -107;
		}
		phdr++;
	}

	dc_flushall();

	gecko_printf("ELF load done. Entry point: %08x\n", elfhdr.e_entry);
	//*entry = elfhdr.e_entry;
	return 0;
}


int powerpc_boot_file(const char *path)
{
	int fres = 0; 
	//FIL fd;
	//u32 decryptionEndAddress, entry;
	
	gecko_printf("powerpc_load_elf returned %d .\n", fres = powerpc_load_elf(path));
	//fres = powerpc_load_dol("/bootmii/00000003.app", &entry);
	//decryptionEndAddress = ( 0x1330100 + read32(0x133008c + read32(0x1330008)) -1 ) & ~3; 
	//gecko_printf("powerpc_load_dol returned %d .\n", fres);
	if(fres) return fres;
	gecko_printf("0xd8005A0 register value is %08x.\n", read32(0xd8005A0));
	if((read32(0xd8005A0) & 0xFFFF0000) != 0xCAFE0000)
	{	gecko_printf("Running old Wii code.\n");
		powerpc_upload_oldstub(elfhdr.e_entry);
		powerpc_reset();
		gecko_printf("PPC booted!\n");
		return 0;
	}gecko_printf("Running Wii U code.\n");
	powerpc_upload_oldstub(0x1800);
 	write_stub(0x1800, (u32*)stubsb1, stubsb1_size/4);
	powerpc_jump_stub(0x1800+stubsb1_size, elfhdr.e_entry);
	dc_flushall();
	//this is where the end of our entry point loading stub will be
	u32 oldValue = read32(0x1330108);

    //set32(HW_GPIO1OWNER, HW_GPIO1_SENSE);
	set32(HW_DIFLAGS,DIFLAGS_BOOT_CODE);
	set32(HW_AHBPROT, 0xFFFFFFFF);
	gecko_printf("Resetting PPC. End on-screen debug output.\n\n");
	gecko_enable(0);

	//reboot ppc side
	clear32(HW_RESETS, 0x30);
	udelay(100);
	set32(HW_RESETS, 0x20);
	udelay(100);
	set32(HW_RESETS, 0x10);

	// do race attack here
	do
	{	dc_invalidaterange((void*)0x1330100,32);
		//ahb_flush_from(AHB_1);
	}while(oldValue == read32(0x1330108));

	write32(0x1330100, 0x38802000); // li r4, 0x2000
	write32(0x1330104, 0x7c800124); // mtmsr r4
	write32(0x1330108, 0x48001802); // b 0x1800
	dc_flushrange((void*)0x1330100,32);
	udelay(100000);
	set32(HW_EXICTRL, EXICTRL_ENABLE_EXI);
	return fres;

}


int powerpc_boot_mem(const u8 *addr, u32 len)
{
	if (len < sizeof(Elf32_Ehdr))
		return -100;

	Elf32_Ehdr *ehdr = (Elf32_Ehdr *) addr;

	if (memcmp("\x7F" "ELF\x01\x02\x01\x00\x00", ehdr->e_ident, 9)) {
		gecko_printf("Invalid ELF header! 0x%02x 0x%02x 0x%02x 0x%02x\n",
					ehdr->e_ident[0], ehdr->e_ident[1],
					ehdr->e_ident[2], ehdr->e_ident[3]);
		return -101;
	}

	if (_check_physaddr(ehdr->e_entry) < 0) {
		gecko_printf("Invalid entry point! 0x%08x\n", ehdr->e_entry);
		return -102;
	}

	if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
		gecko_printf("ELF has no program headers!\n");
		return -103;
	}

	if (ehdr->e_phnum > PHDR_MAX) {
		gecko_printf("ELF has too many (%d) program headers!\n",
					ehdr->e_phnum);
		return -104;
	}

	u16 count = ehdr->e_phnum;
	if (len < ehdr->e_phoff + count * sizeof(Elf32_Phdr))
		return -105;

	Elf32_Phdr *phdr = (Elf32_Phdr *) &addr[ehdr->e_phoff];

	// TODO: add more checks here
	// - loaded ELF overwrites itself?

	powerpc_hang();

	while (count--) {
		if (phdr->p_type != PT_LOAD) {
			gecko_printf("Skipping PHDR of type %d\n", phdr->p_type);
		} else {
			if (_check_physrange(phdr->p_paddr, phdr->p_memsz) < 0) {
				gecko_printf("PHDR out of bounds [0x%08x...0x%08x]\n",
								phdr->p_paddr, phdr->p_paddr + phdr->p_memsz);
				return -106;
			}

			gecko_printf("LOAD 0x%x @0x%08x [0x%x]\n", phdr->p_offset, phdr->p_paddr, phdr->p_filesz);
			memcpy((void *) phdr->p_paddr, &addr[phdr->p_offset],
				phdr->p_filesz);
		}
		phdr++;
	}

	dc_flushall();

	gecko_printf("ELF load done, booting PPC...\n");
	//powerpc_upload_oldstub(ehdr->e_entry);
	powerpc_reset();
	gecko_printf("PPC booted!\n");

	return 0;
}
