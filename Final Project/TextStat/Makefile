# textstat - build, test and install
NAME     := textstat

CC       ?= cc
CFLAGS   ?= -O2
CPPFLAGS += -Iinclude -MMD -MP
CSTD     := -std=c11
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes
PREFIX   ?= /usr/local
BUILD    ?= build

LIB_OBJ  := $(BUILD)/textstat.o
MAIN_OBJ := $(BUILD)/main.o
TEST_BIN := $(BUILD)/test_textstat

COMPILE  = $(CC) $(CPPFLAGS) $(CSTD) $(WARNINGS) $(CFLAGS)

.PHONY: all test sanitize install uninstall clean help
.DEFAULT_GOAL := all

all: $(BUILD)/$(NAME)  ## build the textstat binary

$(BUILD)/$(NAME): $(MAIN_OBJ) $(LIB_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(COMPILE) -c $< -o $@

$(TEST_BIN): tests/test_textstat.c $(LIB_OBJ) | $(BUILD)
	$(COMPILE) $< $(LIB_OBJ) $(LDFLAGS) -o $@

$(BUILD):
	mkdir -p $@

test: $(TEST_BIN)  ## build and run the unit tests
	./$(TEST_BIN)

sanitize:  ## run the tests under AddressSanitizer + UBSan
	$(MAKE) BUILD=build-sanitize \
	    CFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" \
	    LDFLAGS="-fsanitize=address,undefined" test

install: all  ## install to $(PREFIX)/bin (honours DESTDIR)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(BUILD)/$(NAME) $(DESTDIR)$(PREFIX)/bin/$(NAME)

uninstall:  ## remove the installed binary
	rm -f $(DESTDIR)$(PREFIX)/bin/$(NAME)

clean:  ## remove build artifacts
	rm -rf build build-sanitize

help:  ## list available targets
	@grep -E '^[a-z]+:.*##' $(MAKEFILE_LIST) | sed -E 's/:.*## /\t/'

-include $(wildcard $(BUILD)/*.d)
