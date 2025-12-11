# Makefile
CC        := gcc
CFLAGS    := -Werror -Wall -Wextra -O0 -g -std=c17
BUILDDIR  := ../build
LIBDIR    := $(BUILDDIR)/lib
BINDIR    := $(BUILDDIR)/bin
INCLUDES  := -I../tools/include
CFLAGS    += $(INCLUDES)
LINKDIR   := -L$(BUILDDIR)/lib
SUBDIRS   := tools strl

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ \
		CC="$(CC)" \
		CFLAGS="$(CFLAGS)" \
		BUILDDIR="$(BUILDDIR)" \
		LINKDIR="$(LINKDIR)" \
		INCLUDES="$(INCLUDES)" \
		LIBDIR="$(LIBDIR)" \
	  	BINDIR="$(BINDIR)"

clean:
	@rm -rf build

.PHONY: all clean $(SUBDIRS)