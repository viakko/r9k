# Makefile
CC        := gcc
CFLAGS    := -Werror -Wall -Wextra -O3 -std=c17
BUILDDIR  := ../build
LIBDIR    := $(BUILDDIR)/lib
BINDIR    := $(BUILDDIR)/bin
INCLUDES  := -I../include -I../tools/include
CFLAGS    += $(INCLUDES)
LINKDIR   := -L$(BUILDDIR)/lib
SUBDIRS   := tools strc url

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