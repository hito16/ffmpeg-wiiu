.SUFFIXES:
#---------------------------------------------------------------------------
# = Dissecting a WUT compatible make file =
# We ultimately want to port code onto the WIIU.  This is a dumbed down makefile
# so we can see the imported and implied rules.
# Don't actually use this for your own makefile.
#
# For porting the current strucuture is challenging, specifically
# when the source dirs have contain serveral files with their
# own "main()" instances.
# 

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)

#-------------------------------------------------------------------------------
# Notice the imported variables
#-------------------------------------------------------------------------------

include $(DEVKITPRO)/wut/share/wut_rules
$(info # ---- initial wut_root compiller variable values   ----)
$(info included CC		= $(CC))
$(info included CFLAGS		= $(CFLAGS))
$(info # ---- initial wut_root linker variable values   ----)
$(info included LD		= $(LD))
$(info included LDFLAGS		= $(LDFLAGS))
$(info # ---- initial wut_root  variable values   ----)
$(info included WUT_ROOT		= $(WUT_ROOT))
$(info included PORTLIBS		= $(PORTLIBS))
$(info included PORTLIBS_PATH	= $(PORTLIBS_PATH))
$(info included ARCH			= $(ARCH))
$(info included MACHDEP		= $(MACHDEP))
$(info included RPXSPECS		= $(RPXSPECS))

export LD	:=	$(CC)
#-------------------------------------------------------------------------------
# Constraints for your Makefile due to wut_rules
#-------------------------------------------------------------------------------
# import brings in this rule for the TARGET
# %.elf:
#	@echo linking ... $(notdir $@)
#	$(ADD_COMPILE_COMMAND) end
#	$(SILENTCMD)$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@ $(ERROR_FILTER)
#	$(SILENTCMD)$(NM) -CSn $@ > $(notdir $*.lst) $(ERROR_FILTER)
#
# instead of a freeform Makefile, you MUST adhere to the following
#   OFILES (not OBJECTS) %.o
#   LIBS                 -lwut  or `pkg-config --libs-only-l sdl2`
#   LIBPATHS             -L$(DEVKITPRO)/wut/lib
#                              or  `pkg-config --libs-only-L sdl2`
#   LIBDIRS   (optional)  directory above a /lib subdir but YOU must concatenate
#   ex.     export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
#
#
#  Additionally, do not redefine this
# %.o: %.c
#	$(SILENTMSG) $(notdir $<)
#	$(ADD_COMPILE_COMMAND) add $(CC) "$(CPPFLAGS) $(CFLAGS) -c $< -o $@" $<
#	$(SILENTCMD)$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CPPFLAGS) $(CFLAGS) -c $< -o $@ $(ERROR_FILTER)
#
#  Alternatively, you could reimplement the logic in wut_rules, 
#  but future improvements to WUT could leave your code behind
#-------------------------------------------------------------------------------

# Output executable name
TARGET = sdl-portable-app-wiiu

# Explicit Source file example
#  Say we have multiple CLIs sources in one folder, each with a main()
#  We can't just include *.c and compile them. Rather we need to specify
#  a single main() and its dependencies.
SOURCES := sdlmain.c sdltriangle.c sdlanimate.c

# Compiler flags
#CFLAGS = -Wall -g 
CFLAGS	:=	-O3 -ffunction-sections -fdata-sections \
			$(MACHDEP)

CFLAGS	+=	$(INCLUDE) -I$(WUT_ROOT)/include \
				-D__WIIU__ -D__WUT__ -DBIGENDIAN -DUSE_MBEDTLS

CXXFLAGS	:= $(CFLAGS)

ASFLAGS	:=	$(ARCH)
#LDFLAGS	=	$(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)
#-----------------------------------------------------------------------------
# a linker flag generate a detailed map of the program's memory layout after linking 
#-----------------------------------------------------------------------------
LDFLAGS	=	$(ARCH) $(RPXSPECS) -Wl,-Map,$(TARGET).map

LIBPATHS	:=  -L$(WUT_ROOT)/lib
LIBS	:=	-lwut

#-------------------------------------------------------------------------------
# portlibs and 3rd party packages
#   notice how you can include multiple PKG_CONFIG_PATH's.  There are at 
#   least 2 paths (ppc and wiiu).  With porting you may end up adding your own.
#-------------------------------------------------------------------------------
# Linker flags
PKG_CONFIG_PATH = $(DEVKITPRO)/portlibs/ppc/lib/pkgconfig:$(DEVKITPRO)/portlibs/wiiu/lib/pkgconfig
export PKG_CONFIG_PATH
PKGCONF_WIIU	:= /usr/bin/pkg-config
CFLAGS		+=	$(shell $(PKGCONF_WIIU) --cflags SDL2_image SDL2_ttf sdl2)
CXXFLAGS	+=	$(shell $(PKGCONF_WIIU) --cflags SDL2_image SDL2_ttf sdl2)
LIBPATHS	+=	$(shell $(PKGCONF_WIIU) --libs-only-L SDL2_image SDL2_ttf sdl2) \
				$(shell $(PKGCONF_WIIU) --libs-only-L harfbuzz freetype2)	
LIBS		+=	$(shell $(PKGCONF_WIIU) --libs-only-l SDL2_image SDL2_ttf sdl2) \
				$(shell $(PKGCONF_WIIU) --libs-only-l harfbuzz freetype2)

#-------------------------------------------------------------------------------
# romfs
#    all fonts, images and such go into the ROMFS subfolder and are 
#    packaged with your homebrew app.
# notice OFILES : be sure to += or otherwise include this in further
#    references to typical makefile OBJECTS variable
#-------------------------------------------------------------------------------
ROMFS		:= ./romfs
include $(PORTLIBS_PATH)/wiiu/share/romfs-wiiu.mk
CFLAGS		+=	$(ROMFS_CFLAGS)
CXXFLAGS	+=	$(ROMFS_CFLAGS)
LIBS		+=	$(ROMFS_LIBS)
OFILES		+=	$(ROMFS_TARGET)

#-------------------------------------------------------------------------------
#  Rename / merge variables to be compliant with build rules
#-------------------------------------------------------------------------------
OFILES += $(SOURCES:.c=.o)
OUTPUT	= $(TARGET)

# not sure if this is needed
DEPENDS	:=	$(OFILES:.o=.d)
$(info DEPENDS $(DEPENDS))

.PHONY: $(BUILD) clean all 

#-------------------------------------------------------------------------------
# Final values, as needed by 
#	$(SILENTCMD)$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@ $(ERROR_FILTER)
#	$(SILENTCMD)$(NM) -CSn $@ > $(notdir $*.lst) $(ERROR_FILTER)
#-------------------------------------------------------------------------------
$(info # ---- final compiller variable values   ----)
$(info included CC		= $(CC))
$(info included CFLAGS		= $(CFLAGS))
$(info # ---- final linker variable values   ----)
$(info included LD		= $(LD))
$(info included LDFLAGS	= $(LDFLAGS))
$(info included OFILES		= $(OFILES))
$(info included LIBPATHS	= $(LIBPATHS))
$(info included LIBS		= $(LIBS))


#-------------------------------------------------------------------------------
#  Homebrew executable build rules
#-------------------------------------------------------------------------------
# Remember, we already have at least two imported rules you must leave alone
# %.elf:
#   and 
# %.o: %.c
#-------------------------------------------------------------------------------

all: $(OUTPUT).wuhb

$(OUTPUT).wuhb	:	$(OUTPUT).rpx
$(OUTPUT).rpx	:	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)


# Clean rule: Remove the executable
clean:
	rm -f $(TARGET).rpx $(TARGET).elf $(TARGET).wuhb  $(TARGET).map $(TARGET).lst
	rm -f *.o




