CC := gcc
DEFINES := -D_GNU_SOURCE
CFLAGS := -Wall -O3 -std=c17 $(DEFINES)
INCLUDE := -Itools/include

BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs

TOOLS_DIR := tools
MEAS_DIR := meas

TOOLS_SRCS := $(wildcard $(TOOLS_DIR)/*.c)
MEAS_SRCS := $(wildcard $(MEAS_DIR)/*.c)

TOOLS_OBJS := $(TOOLS_SRCS:$(TOOLS_DIR)/%.c=$(OBJDIR)/tools/%.o)
MEAS_OBJS := $(MEAS_SRCS:$(MEAS_DIR)/%.c=$(OBJDIR)/meas/%.o)

MEAS_BIN := $(BUILDDIR)/meas

TARGETS := $(MEAS_BIN)

all:$(TARGETS)

$(MEAS_BIN): $(MEAS_OBJS) $(TOOLS_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(OBJDIR)/tools/%.o: $(TOOLS_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR)/meas/%.o: $(MEAS_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean