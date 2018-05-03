# Makefile for oob-ipmid
.DEFAULT_GOAL := all

BINS := oob-ipmid

ALL_CFLAGS += -fPIC -Wall $(CFLAGS)

OBJECTS := app.o ipmi.o oob-ipmid.o

all: $(BINS) 

%.o : %.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(BINS) : $(OBJECTS)
	$(CC) $(ALL_CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install:
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(BINS) $(DESTDIR)$(BINDIR)
