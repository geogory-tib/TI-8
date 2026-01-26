# ----------------------------
# Makefile Options
# ----------------------------

NAME = TI8
ICON = icon.png
DESCRIPTION = "CHIP8 EMU"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -O3

# ----------------------------

include $(shell cedev-config --makefile)
