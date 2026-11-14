#---------------------------------------------------------------------------------
# souldew.3dsx – the launcher (libctru). Based on devkitPro's 3DS application template.
#   make        → souldew.3dsx (+ .smdh)
#   make demo   → souldew-demo.3dsx: walks through its screens by itself (emulator screenshots)
#   make cia    → souldew.cia: the same app for the HOME menu (makerom + bannertool, banner from cia/)
# romfs/souldew.3gx (copied there by build.sh) travels inside both: the launcher installs the plugin from it.
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
ROMFS		:=	romfs

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
ifneq ($(wildcard $(CURDIR)/$(ROMFS)/souldew.3gx),)
export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
export ROMFS_DEPS := $(CURDIR)/$(ROMFS)/souldew.3gx
endif
# the version for the .cia comes from common/screens.h (kVersion "X.Y.Z-…" → major X, minor Y, micro Z)
VERSION		:=	$(shell sed -n 's/.*kVersion = "\([^"]*\)".*/\1/p' ../common/screens.h)
VER_NUM		:=	$(firstword $(subst -, ,$(VERSION)))
VER_MAJOR	:=	$(word 1,$(subst ., ,$(VER_NUM)))
VER_MINOR	:=	$(word 2,$(subst ., ,$(VER_NUM)))
VER_MICRO	:=	$(word 3,$(subst ., ,$(VER_NUM)))
.PHONY: all demo cia clean
all:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
demo:
	@$(MAKE) --no-print-directory TARGET=souldew-demo BUILD=build-demo DEFINES=-DSOULDEW_DEMO=1 all
cia: all
	@test -f $(ROMFS)/souldew.3gx || { echo "romfs/souldew.3gx missing (build the plugin, copy it there)"; exit 1; }
	@bannertool makebanner -i cia/banner.png -a cia/banner.wav -o $(BUILD)/banner.bnr >/dev/null
	@bannertool makesmdh -s "$(APP_TITLE)" -l "$(APP_TITLE) - $(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i $(ICON) \
	   -f visible,allow3d,recordusage -o $(BUILD)/icon-cia.smdh >/dev/null
	@makerom -f cia -o $(TARGET).cia -target t -exefslogo -rsf cia/souldew.rsf -elf $(TARGET).elf \
	   -icon $(BUILD)/icon-cia.smdh -banner $(BUILD)/banner.bnr -DAPP_ROMFS=$(ROMFS) \
	   -major $(VER_MAJOR) -minor $(VER_MINOR) -micro $(VER_MICRO)
	@echo "built ... $(TARGET).cia ($(VERSION))"
clean:
	@rm -rf build build-demo *.3dsx *.smdh *.elf *.cia
else
DEPENDS	:=	$(OFILES:.o=.d)
$(OUTPUT).3dsx	:	$(OUTPUT).elf $(OUTPUT).smdh $(ROMFS_DEPS)
$(OUTPUT).elf	:	$(OFILES)
-include $(DEPENDS)
endif
