CC := gcc
CFLAGS := -Wall -O3 -std=c17
INCLUDE := -Itools/include

BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs

TOOLS_DIR := tools
LEN_DIR := len

TOOLS_SRCS := $(wildcard $(TOOLS_DIR)/*.c)
LEN_SRCS := $(wildcard $(LEN_DIR)/*.c)

TOOLS_OBJS := $(TOOLS_SRCS:$(TOOLS_DIR)/%.c=$(OBJDIR)/tools/%.o)
LEN_OBJS := $(LEN_SRCS:$(LEN_DIR)/%.c=$(OBJDIR)/len/%.o)

LEN_BIN := $(BUILDDIR)/len

TARGETS := $(LEN_BIN)

all:$(TARGETS)

$(LEN_BIN): $(LEN_OBJS) $(TOOLS_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(OBJDIR)/tools/%.o: $(TOOLS_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR)/len/%.o: $(LEN_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean