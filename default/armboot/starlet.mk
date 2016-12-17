ifeq ($(strip $(DEVKITARM)),)
$(error "Set DEVKITARM in your environment.")
endif


#ifeq ($(strip $(WIIDEV)),)
#$(error "Set WIIDEV in your environment.")
#endif

PREFIX = $(DEVKITARM)/bin/arm-eabi-
#PREFIX = $(WIIDEV)/bin/arm-eabi-

CFLAGS = -mbig-endian -mcpu=arm926ej-s
CFLAGS += -fomit-frame-pointer -ffunction-sections
CFLAGS += -Wall -Wextra -Os -pipe
ASFLAGS =
LDFLAGS = -mbig-endian -n -nostartfiles -nodefaultlibs -Wl,-gc-sections -L$DEVKITARM/lib

