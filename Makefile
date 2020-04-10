CFLAGS ?= -Wall -Wextra -pedantic -std=c11
CPPFLAGS = -MMD -D_POSIX_C_SOURCE=200809L

BIN = signal_test

OBJS = $(BIN).o signal_lut.o signal_pipe.o
DEPS = $(OBJS:.o=.d)

all: $(BIN)

$(BIN): $(OBJS)

-include $(DEPS)

clean:
	$(RM) $(BIN) $(OBJS) $(DEPS)

.PHONY: clean all
