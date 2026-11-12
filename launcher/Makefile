#---------------------------------------------------------------------------------
# souldew.3dsx – the launcher (libctru). Based on devkitPro's 3DS application template.
#   make        → souldew.3dsx (+ .smdh)
#   make demo   → souldew-demo.3dsx: walks through its screens by itself (emulator screenshots)
#---------------------------------------------------------------------------------
.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET		?=	souldew
BUILD		?=	build
SOURCES		:=	source ../common
INCLUDES	:=	source ../common
APP_TITLE	:=	souldew
APP_DESCRIPTION	:=	Nuzlocke tracker
APP_AUTHOR	:=	souldew
ICON		:=	icon.png

ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS	:=	-Wall -O2 -mword-relocations -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__ $(DEFINES)
CXXFLAGS	:=	$(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS	:=	$(ARCH)
LDFLAGS	=	-specs=3dsx.specs $(ARCH) -Wl,--gc-sections
LIBS	:=	-lctru -lm
LIBDIRS	:=	$(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export LD	:=	$(CXX)
export OFILES	:=	$(CPPFILES:.cpp=.o)
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export APP_ICON	:=	$(TOPDIR)/$(ICON)
export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
.PHONY: all demo clean
all:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
demo:
	@$(MAKE) --no-print-directory TARGET=souldew-demo BUILD=build-demo DEFINES=-DSOULDEW_DEMO=1 all
clean:
	@rm -rf build build-demo *.3dsx *.smdh *.elf
else
DEPENDS	:=	$(OFILES:.o=.d)
$(OUTPUT).3dsx	:	$(OUTPUT).elf $(OUTPUT).smdh
$(OUTPUT).elf	:	$(OFILES)
-include $(DEPENDS)
endif
