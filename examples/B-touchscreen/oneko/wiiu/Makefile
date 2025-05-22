#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

export PKG_CONFIG_PATH = /opt/devkitpro/portlibs/ppc/lib/pkgconfig:/opt/devkitpro/portlibs/wiiu/lib/pkgconfig/
# check out SDL and build to get  SDL touch support for WiiU
#export PKG_CONFIG_PATH = /project/SDL_wiiu_install/lib/pkgconfig/

TOPDIR ?= $(CURDIR)

include $(DEVKITPRO)/wut/share/wut_rules

#-------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#-------------------------------------------------------------------------------

TARGET		:=	sdl_oneko_wiiu
BUILD		:= 	build
SOURCES		:=	. \
				../../../rsyslog

SOURCE_FILES	:=	\
                    ../../../example-util/rsyslog-wiiu.c 

INCLUDES	:=	. \
			.. \
				../../../rsyslog \
				../../../example-util
				

#-------------------------------------------------------------------------------
# options for code generation
#-------------------------------------------------------------------------------

CFLAGS	:=	-O3 -ffunction-sections -fdata-sections \
			$(MACHDEP)

CFLAGS	+=	$(INCLUDE) -D__WIIU__ -D__WUT__ -DBIGENDIAN -DUSE_MBEDTLS

CXXFLAGS	:= $(CFLAGS)

ASFLAGS	:=	$(ARCH)
LDFLAGS	=	$(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)

LIBS	:=  -lwut 

#-------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level
# containing include and lib
#-------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(WUT_ROOT) 

#---------------------------------------------------------------------------------
# SDL libraries
#---------------------------------------------------------------------------------
# Minimal libraries to check if your environement is configured
#PKGCONF	:=	$(DEVKITPRO)/portlibs/ppc/bin/powerpc-eabi-pkg-config
PKGCONF	:=	$(DEVKITPRO)/portlibs/wiiu/bin/powerpc-eabi-pkg-config
SDL_CFLAGS	:=	$(shell $(PKGCONF) --cflags sdl2)
SDL_LDFLAGS	:=	$(shell $(PKGCONF) --libs sdl2)
CFLAGS		+=	$(SDL_CFLAGS)
CXXFLAGS	+=	$(SDL_CFLAGS)
LIBS		+=	$(SDL_LDFLAGS) 

#---------------------------------------------------------------------------------
# romfs - make sure you download, build & do a make install per instructions
#---------------------------------------------------------------------------------
# fonts and images in ROMFS will packaged into a romfs and read by the homebrew app
#ROMFS		:= ./romfs
#include $(PORTLIBS_PATH)/wiiu/share/romfs-wiiu.mk
#CFLAGS		+=	$(ROMFS_CFLAGS)
#CXXFLAGS	+=	$(ROMFS_CFLAGS)
#LIBS		+=	$(ROMFS_LIBS)
#OFILES		+=	$(ROMFS_TARGET)

#-------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#-------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
			$(foreach sf,$(SOURCE_FILES),$(CURDIR)/$(dir $(sf)))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c))) \
			$(foreach f,$(SOURCE_FILES),$(notdir $(f)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#-------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#-------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#-------------------------------------------------------------------------------
	export LD	:=	$(CC)
#-------------------------------------------------------------------------------
else
#-------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 		:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
.PHONY: $(BUILD) clean all dist

#-------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#-------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).rpx $(TARGET).elf $(TARGET).wuhb


#-------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
all	:	$(OUTPUT).wuhb

$(OUTPUT).wuhb	:	$(OUTPUT).rpx
$(OUTPUT).rpx	:	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#-------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#-------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#-------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------

