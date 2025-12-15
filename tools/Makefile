# tools/Makefile
MODULE      := tools
SRCS        := $(shell find ./ -type f -name '*.c' | sed 's|^\./||')
OBJDIR      := $(BUILDDIR)/obj/$(MODULE)
OBJS        := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
LIB         := $(LIBDIR)/libtools.a
AR          := ar
ARFLAGS     := rcs

all: $(LIB)

$(LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

-include $(OBJS:.o=.d)

.PHONY: all clean