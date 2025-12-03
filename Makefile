CC := gcc
DEFINES := -D_GNU_SOURCE
CFLAGS := -Wall -O3 -std=c17 $(DEFINES)
INCLUDE := -Itools/include

BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs

TOOLS_DIR := tools
WDC_DIR := wdc
NS_DIR := ns
CLIP_DIR := clip

TOOLS_SRCS := $(wildcard $(TOOLS_DIR)/*.c)
WDC_SRCS := $(wildcard $(WDC_DIR)/*.c)
NS_SRCS := $(wildcard $(NS_DIR)/*.c)
CLIP_SRCS := $(wildcard $(CLIP_DIR)/*.c)

TOOLS_OBJS := $(TOOLS_SRCS:$(TOOLS_DIR)/%.c=$(OBJDIR)/tools/%.o)
WDC_OBJS := $(WDC_SRCS:$(WDC_DIR)/%.c=$(OBJDIR)/wdc/%.o)
NS_OBJS := $(NS_SRCS:$(NS_DIR)/%.c=$(OBJDIR)/ns/%.o)
CLIP_OBJS := $(CLIP_SRCS:$(CLIP_DIR)/%.c=$(OBJDIR)/clip/%.o)

WDC_BIN := $(BUILDDIR)/wdc
NS_BIN := $(BUILDDIR)/ns
CLIP_BIN := $(BUILDDIR)/clip

TARGETS := $(WDC_BIN) $(NS_BIN) $(CLIP_BIN)

all:$(TARGETS)

$(WDC_BIN): $(WDC_OBJS) $(TOOLS_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(NS_BIN): $(NS_OBJS) $(TOOLS_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@ -lresolv

$(CLIP_BIN): $(CLIP_OBJS) $(TOOLS_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(OBJDIR)/tools/%.o: $(TOOLS_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR)/wdc/%.o: $(WDC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR)/ns/%.o: $(NS_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR)/clip/%.o: $(CLIP_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean