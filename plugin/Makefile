#---------------------------------------------------------------------------------
# souldew.3gx – the Luma3DS plugin (CTRPluginFramework). Based on CTRPF's TestPlugin Makefile.
#   make            → souldew.3gx
#   make test       → souldew-test.3gx (also runs in the mock game, opens souldew's screens by itself once)
#---------------------------------------------------------------------------------
.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

export TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

CTRPFLIB	?=	$(DEVKITPRO)/libctrpf
NAME		?=	souldew
INCLUDES	:=	source ../common
SOURCES		:=	source ../common
PSF		?=	$(NAME).plgInfo

ARCH		:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS		:=	-mword-relocations -ffunction-sections -fdata-sections -fno-strict-aliasing \
			-O2 -fomit-frame-pointer -Wall $(ARCH) $(INCLUDE) -D__3DS__ -DNDEBUG=1 $(DEFINES)
CXXFLAGS	:=	$(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS		:=	$(ARCH)
LDFLAGS		:=	-T $(TOPDIR)/3gx.ld $(ARCH) -Os -Wl,--strip-discarded,--strip-debug,--gc-sections,--section-start,.text=0x07000100
LIBS		:=	-lctrpf -lctru -lm
LIBDIRS		:=	$(CTRPFLIB) $(CTRULIB) $(PORTLIBS)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export LD	:=	$(CXX)
export OFILES	:=	$(CPPFILES:.cpp=.o)
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(DEVKITPRO)/libctru/include/3ds
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L $(dir)/lib)
.PHONY: all test clean

all:
	@mkdir -p build-release
	@$(MAKE) BUILD=build-release OUTPUT=$(CURDIR)/souldew.3gx NAME=souldew --no-print-directory -C build-release -f $(CURDIR)/Makefile

test:
	@mkdir -p build-test
	@$(MAKE) BUILD=build-test OUTPUT=$(CURDIR)/souldew-test.3gx NAME=souldew-test DEFINES=-DSOULDEW_TEST=1 \
		--no-print-directory -C build-test -f $(CURDIR)/Makefile
	@python3 ../tools/allow_homebrew.py souldew-test.3gx

clean:
	@rm -rf build-release build-test *.elf *.3gx
else
DEPENDS	:=	$(OFILES:.o=.d)
$(OUTPUT) : $(basename $(OUTPUT)).elf
$(basename $(OUTPUT)).elf : $(OFILES)
%.3gx: %.elf
	@echo creating $(notdir $@)
	@3gxtool -s $^ $(TOPDIR)/$(PSF) $@
-include $(DEPENDS)
endif
