/*
File Logger - Quick and easy file logging for Wii homebrew
Copyright (C) 2013 JoostinOnline
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdarg.h>

#include "types.h"
#include "ff.h"
#include "vsprintf.h"
#include "filelog.h"

#ifdef ENABLE_LOG
static FIL __log_file;
static bool __log_initialized = false;
#endif
bool Log_Init(const char *filename) {
#ifdef ENABLE_LOG
	if (__log_initialized) return true;
	if(f_open(&__log_file, filename, FA_WRITE|FA_CREATE_ALWAYS) == FR_OK)
		__log_initialized = true;
	return __log_initialized;
#else
	return false;
#endif
}
#ifdef ENABLE_LOG
void Log_Deinit(void) {
	if (__log_initialized) f_close(&__log_file);
	__log_initialized = false;
	return;
}

void Log(const char *format, ...) {	
	if (!__log_initialized) return;
	
	va_list args;
	va_start(args, format);
	char buffer[256] = {0};
	vsnprintf(buffer, sizeof(buffer)-1, format, args);
	f_puts(buffer, &__log_file);
	va_end(args);
	
	f_sync(&__log_file);
	return;
}
#endif
