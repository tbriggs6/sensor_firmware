# Host test: message struct layout for Zephyr/backend compatibility.
# Run from repo root: make -C tests/baseline
# Or from here: make (assumes REPO_ROOT=../..)

REPO_ROOT ?= ../..
MODULES   = $(REPO_ROOT)/modules
CFLAGS   ?= -g -Wall -I$(MODULES)/config -I$(MODULES)/messenger -I$(MODULES)/command

all: message_struct_test

message_struct_test: message_struct_test.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f message_struct_test message_struct_test.o

.PHONY: all clean
