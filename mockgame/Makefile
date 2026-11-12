#---------------------------------------------------------------------------------
# souldew-mock.cxi – the mock game (test title 000400000F5D0000), built with makerom from its ELF.
#---------------------------------------------------------------------------------
.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET		:=	souldew-mock
BUILD		:=	build
SOURCES		:=	source ../common
INCLUDES	:=	source ../common
APP_TITLE	:=	souldew mock game
APP_DESCRIPTION	:=	test title for the plugin
APP_AUTHOR	:=	souldew

ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS	:=	-Wall -O2 -mword-relocations -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__
CXXFLAGS	:=	$(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS	:=	$(ARCH)
LDFLAGS	=	-specs=3dsx.specs $(ARCH) -Wl,--gc-sections -Wl,--undefined=sdk_gsp_register -Wl,--undefined=sdk_present
LIBS	:=	-lctru -lm
LIBDIRS	:=	$(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)
# only what the mock needs from common/
CPPFILES	:=	main.cpp pk6.cpp gfx.cpp fbblit.cpp fonts_data.cpp ui.cpp data_gen.cpp run.cpp json.cpp nuzlocke.cpp games.cpp
export LD	:=	$(CXX)
export OFILES	:=	$(CPPFILES:.cpp=.o) sdk_gsp.o
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
.PHONY: all clean
all:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" icon.png $(TARGET).smdh
	@makerom -f cxi -o $(TARGET).cxi -rsf mockgame.rsf -target t -elf $(TARGET).elf -icon $(TARGET).smdh -ignoresign
	@echo built ... $(TARGET).cxi
clean:
	@rm -rf build *.cxi *.smdh *.elf
else
DEPENDS	:=	$(OFILES:.o=.d)
$(OUTPUT).elf	:	$(OFILES)
-include $(DEPENDS)
endif
