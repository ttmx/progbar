# progbar - progress bar
# See LICENSE file for copyright and license details.

include config.mk

SRC = drw.c progbar.c util.c
OBJ = $(SRC:.c=.o)

all: options progbar

options:
	@echo progbar build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

.c.o:
	$(CC) -c $(CFLAGS) $<

config.h:
	cp config.def.h $@

$(OBJ): arg.h config.h config.mk drw.h

progbar: progbar.o drw.o util.o
	$(CC) -o $@ progbar.o drw.o util.o $(LDFLAGS)

clean:
	rm -f progbar stest $(OBJ) progbar-$(VERSION).tar.gz *.rej *.orig

dist: clean
	mkdir -p progbar-$(VERSION)
	cp LICENSE Makefile README arg.h config.def.h config.mk progbar.1\
		drw.h util.h $(SRC)\
		progbar-$(VERSION)
	tar -cf progbar-$(VERSION).tar progbar-$(VERSION)
	gzip progbar-$(VERSION).tar
	rm -rf progbar-$(VERSION)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f progbar $(DESTDIR)$(PREFIX)/bin/progbar
	chmod 755 $(DESTDIR)$(PREFIX)/bin/progbar

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/progbar

.PHONY: all options clean dist install uninstall
