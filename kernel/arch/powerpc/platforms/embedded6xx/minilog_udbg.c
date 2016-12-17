#include <asm/udbg.h>

#include "minilog_udbg.h"

#define MINILOG_COMMINICATION_AREA 0x2fe0

void minilog_udbg_putc(char ch)
{	// wait/make sure MINI has already picked up the last char
	while(*(char*)MINILOG_COMMINICATION_AREA)
		asm("li 4,0x2fe0;dcbi 0,4":::"4"); // invalidate
	*(char*)MINILOG_COMMINICATION_AREA = ch;
		asm("li 4,0x2fe0;dcbf 0,4":::"4"); // flush
}

void __init minilog_udbg_init(void)
{
	udbg_putc = minilog_udbg_putc;
	/*
	 * At this point USB Gecko already has a BAT setup that enables I/O
	 * to the EXI hardware.
	 *
	 * The BAT uses a virtual address range reserved at the fixmap.
	 * This must match the virtual address configured in
	 * head_32.S:setup_usbgecko_bat().
	 *
	 * Here it prepares again the same BAT for MMU_init.
	 * This allows udbg I/O to continue working after the MMU is
	 * turned on for real.
	 * It is safe to continue using the same virtual address as it is
	 * a reserved fixmap area.
	 *
	 *
	 *	* * *
	 *
	 * We may need to do something similar for the bit of memory we
	 * use to communicate with the MINI mod.
	 *
	 *	* * *
	 *
	 */
	 
/*	void __iomem *early_debug_area = (void __iomem *)__fix_to_virt(FIX_EARLY_DEBUG_BASE);
	setbat(1, (unsigned long)early_debug_area,
	       MINILOG_COMMINICATION_AREA, 128*1024, PAGE_KERNEL_NCG);
*/
}

#ifdef CONFIG_PPC_EARLY_DEBUG_MINILOG

/*
 * USB Gecko early debug support initialization for udbg.
 */
void __init udbg_init_early_minilog(void)
{	minilog_udbg_init();
}

#endif /* CONFIG_PPC_EARLY_DEBUG_MINILOG */
