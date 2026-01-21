# ----------------------------
# Makefile Options
# ----------------------------

NAME = TI-8
ICON = icon.png
DESCRIPTION = "CHIP8 EMU"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
