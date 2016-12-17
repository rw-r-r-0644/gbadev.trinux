#	TinyLoad - a simple region free (original) game launcher in 4k

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http:#www.gnu.org/licenses/old-licenses/gpl-2.0.txt

# This code comes from the Twilight Hack
# Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>

# Slightly modified to imitate the homebrew channel stub


.set r0,0;		.set r1,1;		.set r2,2;		.set r3,3;		.set r4,4;
.set r5,5;		.set r6,6;		.set r7,7;		.set r8,8;		.set r9,9;
.set r10,10;	.set r11,11;	.set r12,12;	.set r13,13;	.set r14,14;
.set r15,15;	.set r16,16;	.set r17,17;	.set r18,18;	.set r19,19;
.set r20,20;	.set r21,21;	.set r22,22;	.set r23,23;	.set r24,24;
.set r25,25;	.set r26,26;	.set r27,27;	.set r28,28;	.set r29,29;
.set r30,30;	.set r31,31;

#include "hw.h" 	 

	.globl _start  
_start:

#	lis		r3,0x0d80		#HW_REG_BASE physical address
#	lwz 	r4,0xc4(r3)		#HW_GPIO1BDIR
#	ori		r4,r4,0x100
#	stw		r4,0xc4(r3)

#	lis		r3,0x0d80		#HW_REG_BASE physical address
#	lwz 	r4,0xc0(r3)		#HW_GPIO1BOUT
#	ori		r4,r4,0x100
#	stw		r4,0xc0(r3)
#	b		.
#	b		.


 
	mfspr r3,944
	oris r3,r3,0xc000
	mtspr 944,r3
	#hid5(944) |= 0xc0000000;  # enable HID5 and PIR (upir?)
	 
	# At this point, upir contains the core ID (0, 1, 2) that is currently
	# executing.
	 
    
  # Global init
	mfspr r3,1007
	cmpwi r3,0
	bne endif1
	#if (/*upir(1007) == 0*/ r3)
	 
  	
     
  	mfspr r3,947
		lis r4,0x4000
		not r4,r4
		and r3,r3,r4
		mtspr 947,r3
		#scr(947) &= ~0x40000000; # scr, car, and bcr are global SPRs
	 
		mfspr r3,947
		oris r3,r3,0x8000
		mtspr 947,r3
		#scr(947) |= 0x80000000;

		mfspr r3,948
		oris r3,r3,0xfc10
		mtspr 948,r3
		#car(948) |= 0xfc100000; # these bit assignments are unknown
   
		#lis r3,0x0800
#or
      #mfspr r3,949
		#oris r3,r3,0x0800
      
		#mtspr 949,r3
		#bcr(949) |= 0x08000000;
	 
	endif1:
	 


  # Per-core init
	# these registers and bits already exist in Broadway
	 
	lis r3,0x11
	ori r3,r3,0x0024
	mtspr 1008,r3
	#hid0(1008) = 0x110024; # enable BHT, BTIC, NHR, DPM
	 
	lis r3,0xf
	mtspr 920,r3
	#hid2(920) = 0xf0000; # enable cache and DMA errors
	 
	lis r3,0xb3b0
	mtspr 1011,r3
	#hid4(1011) = 0xb3b00000; # 64B fetch, depth 4, SBE, ST0, LPE, L2MUM, L2CFI

	# HID5 is new and unknown, probably mostly controls coherency and the
	# new L2 and core interface units
	mfspr r3,287
	andi. r3,r3,0xFFFF
  b endelse
  #if(/*pvr(287) & 0xFFFF*/ r3 == 0x101)
	 
		mfspr r3,944
		oris r3,r3,0x6FBD
		ori r3,r3,0x4300
		mtspr 944,r3
		#hid5(944) |= 0x6FBD4300;
	 
	b endelse
	endif2:
	#else
	 
		mfspr r3,944
		oris r3,r3,0x6FFD
		ori r3,r3,0xC000
		mtspr 944,r3
		#hid5(944) |= 0x6FFDC000;
	 
	endelse:
	 
	mfspr r3,920
	oris r3,r3,0xe000
	mtspr 920,r3
	#hid2(920) |= 0xe0000000; # LSQE, WPE, PSE
	 
	mfmsr r3
	ori r3,r3,0x2000
	mtmsr r3
	#msr |= 0x2000; # enable floating point
	 
	              
# boring TB, decrementer, mmu init omitted
	 
# floating point reg init
	 
	mfspr r3,1008
	ori r3,r3,0xc00
	mtspr 1008,r3
	#hid0(1008) |= 0xc00; # flash invalidate ICache, DCache
	 
	mfspr r3,1008
	lis r4,0x10
	not r4,r4
	and r3,r3,r4
	mtspr 1008,r3
	#hid0(1008) &= ~0x100000; # disable DPM
	 
	li r4,0
	mtspr 1017,0
	#l2cr(1017) = new_l2cr(?) = 0
	 
	mfspr r3,287
	andi. r3,r3,0xFFFF
	cmpwi r3,0x100
	bne endif3
	#if (/*pvr(287) & 0xffff*/ r3 == 0x100)
	 
		ori r4,r4,0x8
		#new_l2cr(?) |= 0x8;
	 
	endif3:
	 
	mfspr r3,944
	oris r3,r3,0x0100
	mtspr 944,r3
	#hid5(944) |= 0x01000000;
	 
	mfspr r3,1007
	cmpwi r3,1
	bne endif4
	#if (/*core*/ r3 == 1)
	 
		oris r4,r4,0x2000
		#new_l2cr(r4) |= 0x20000000; # probably has something to do with the
								# extra L2 for core1
	endif4:
	 
	mtspr 1017,r4
	#l2cr(1017) = new_l2cr(r4);
	 
	oris r4, r4, 0x20
	#new_l2cr(r4) |= 0x200000; # L2 global invalidate
	 
	mtspr 1017,r4
	#l2cr(1017) = new_l2cr(r4);
	 
	loopstart:
	mfspr r3,1017
	andi. r3,r3,1
	bne loopstart
	#while (/*l2cr(1017) & 1*/ r3); # wait for global invalidate to finish
	 
	mfspr r3,1017
	lis r4,0x20
	not r4,r4
	and r3,r3,r4
	mtspr 1017,r3
	#l2cr(1017) &= ~0x200000; # clear L2 invalidate
	 
	mfspr r3,1017
	oris r3,r3,0x8000
	mtspr 1017,r3
	#l2cr(1017) |= 0x80000000; # L2 enable
	 
	mfspr r3,1008
	oris r3,r3,0x10
	mtspr 1008,r3
	#hid0(1008) |= 0x100000; # enable DPM
	 
	mfspr r3,1008
	ori r3,r3,0xc000
	mtspr 1008,r3
	#hid0(1008) |= 0xc000; # enable DCache+ICache
	
	#self note : 15-12,11-8,7-4,3-0
	
	
# optional: enable locked L1 cache as usual
# boring standard GPR init
	li      r0,0
# I think these 2 (r1 and r2) have other uses (stack pointer and something else)
# so I havent put thought into what to initialize them to yet.
	li      r3,0
	li      r4,0
	li      r5,0
	li      r6,0
	li      r7,0
	li      r8,0
	li      r9,0
	li      r10,0
	li      r11,0
	li      r12,0
	li      r14,0
	li      r15,0
	li      r16,0
	li      r17,0
	li      r18,0
	li      r19,0
	li      r20,0
	li      r21,0
	li      r22,0
	li      r23,0
	li      r24,0
	li      r25,0
	li      r26,0
	li      r27,0
	li      r28,0
	li      r29,0
	li      r30,0
	li      r31,0
   
  # Core is now initialized. Check core ID (upir) and jump to wherever
	mfspr r3,1007
	cmpwi r3,0
 #  #if (/*core0*/ r3 == 0)
bne flagloop
		# To kickstart the other cores (from core 0):
		# core 1
		mfspr r3,947
		oris r3,r3,0x0020
		mtspr 947,r3
		#scr(947) |= 0x00200000;
  	# core 2
		mfspr r3,947
		oris r3,r3,0x0040
		mtspr 947,r3
		#scr(947) |= 0x00400000;
b stubend
# do
	flagloop:
   b flagloop
# (wait for some flag set from core 1 when initialized)
# (wait for some flag set from core 2 when initialized)
# while
	 
# jump to code start
	 
	ifcore1 :

# set a flag for the main core,
	 
		# spin in a loop waiting for a vector, or whatever
	  core1loop0:
		b core1loop1
	  core1loop1:
		b core1loop0
	 
	ifcore2 :
 	 
# set a flag for the main core,
	 
		# spin in a loop waiting for a vector, or whatever
	  core2loop0:
		b core2loop1
	  core2loop1:
		b core2loop0
	
	stubend:
	# There's probably an easier way to do this but I'd just put them in two separate loops
	# I've also found that it won't let you write to an address that is currently executing
	# so I made the loop 2 instructions long
	
	#The idea would be to temporarily place a linked branch instruction
	#over the top of the first "b core1loop" and "b core2loop" instructions
	#when you want to use those cores and then first thing to do when the core
	#jumps to the new instructions would be put the old "b core1loop" and "b core2loop"
	#instruction back in place. That way the return instruction ("blr") from the custom code would
	#just pop back to the SECOND "b core1loop" / "b core2loop" instruction which would
	#put the core back in the original loop.
	
	 
	# Note: the Cafe OS kernel actually then uses core 2 as the main core
	# after starting all three. This is probably unimportant.
