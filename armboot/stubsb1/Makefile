
include $(DEVKITPPC)/wii_rules

#PREFIX = powerpc-gekko-
PREFIX = powerpc-eabi-
#PREFIX = /home/megazig/Wii/bootmii-utils/bin/powerpc-elf-

AR = $(PREFIX)ar
AS = $(PREFIX)as
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
RANLIB = $(PREFIX)ranlib
STRIP = $(PREFIX)strip
#RAW2C = c:/devkitpro/devkitppc/bin/raw2c
RAW2C = $(DEVKITARM)/bin/raw2c
COPY = cp

MACHDEP = -mcpu=750 -mno-eabi -mhard-float -DTINY -DDEBUG
#CFLAGS = $(MACHDEP) -Os -Wall -pipe -ffunction-sections -finline-functions-called-once -mno-sdata --combine -fwhole-program -ffreestanding
CFLAGS = $(MACHDEP) -O1 -Werror -Wall -pipe -ffunction-sections -finline-functions-called-once -mno-sdata
LDFLAGS = $(MACHDEP) -n -nostartfiles -nostdlib -Wl,-T,openstub.ld -L.
ASFLAGS = -D_LANGUAGE_ASSEMBLY -DHW_RVL -DTINY

TARGET_LINKED = tinyload.elf
TARGET = stubsb1.bin
TARGET_C = stubsb1.c
TARGET_H = stubsb1.h
TARGET_O = stubsb1.o

CFILES = ios.c utils.c cache.c usbgecko.c main.c
#OBJS = crt0.o _all.o
OBJS = stub_sb_on.o

DEPDIR = .deps

LIBS = 

all: $(TARGET_C) $(OBJS) 

%.o: %.s
	@echo " ASSEMBLE    $<"
	@$(CC) $(CFLAGS) $(DEFINES) $(ASFLAGS) -c $< -o $@

%.o: %.S
	@echo " ASSEMBLE    $<"
	@$(CC) $(CFLAGS) $(DEFINES) $(ASFLAGS) -c $< -o $@

%.o: %.c
	@echo " COMPILE     $<"
	@$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

#_all.o: $(CFILES)
#	@echo " COMPILE ALL "
#	@mkdir -p $(DEPDIR)
#	@$(CC) $(CFLAGS) $(DEFINES) -Wp,-MMD,$(DEPDIR)/$(*F).d,-MQ,"$@",-MP -c $(CFILES) -o $@

$(TARGET_LINKED): $(OBJS)
	@echo " LINK        $@"
	@$(CC) -g -o $@ $(LDFLAGS) $(OBJS) $(LIBS)

$(TARGET): $(TARGET_LINKED)
	@echo " OBJCOPY     $@"
	@$(OBJCOPY) -O binary $< $@

$(TARGET_C): $(TARGET)
	@echo " raw2c     $<"
	@$(RAW2C) $<
	@$(COPY)  $(TARGET_C) ./../$(TARGET_C)
	@$(COPY)  $(TARGET_H) ./../$(TARGET_H)
clean:
	rm -rf $(TARGET_LINKED) $(TARGET) $(OBJS) $(DEPDIR) $(TARGET_C) $(TARGET_H)
