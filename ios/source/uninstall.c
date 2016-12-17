/*-------------------------------------------------------------
 
uninstall.c -- title uninstallation
 
Copyright (C) 2008 tona
Unless other credit specified
 
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.
 
Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:
 
1.The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.
 
2.Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.
 
3.This notice may not be removed or altered from any source
distribution.
 
-------------------------------------------------------------*/

#include <stdio.h>
#include <gccore.h>

#include "uninstall.h"

/* Many additional functions appear in the original version
   of this file from AnyTitle Deleter.  They are not needed
   for this app and require many additional files to work,
   so they have been removed.*/

s32 Uninstall_DeleteTitle(u32 title_u, u32 title_l)
{
	s32 ret;
	char filepath[256];
	sprintf(filepath, "/title/%08x/%08x",  title_u, title_l);
	
	/* Remove title */
	printf("\t\t- Deleting title file %s...", filepath);
	fflush(stdout);

	ret = ISFS_Delete(filepath);
	if (ret < 0)
		printf("\n\tError! ISFS_Delete(ret = %d)\n", ret);
	else
		printf(" OK!\n");

	return ret;
}

s32 Uninstall_DeleteTicket(u32 title_u, u32 title_l)
{
	s32 ret;

	char filepath[256];
	sprintf(filepath, "/ticket/%08x/%08x.tik", title_u, title_l);
	
	/* Delete ticket */
	printf("\t\t- Deleting ticket file %s...", filepath);
	fflush(stdout);

	ret = ISFS_Delete(filepath);
	if (ret < 0)
		printf("\n\tTicket delete failed (No ticket?) %d\n", ret);
	else
		printf(" OK!\n");
	return ret;
}