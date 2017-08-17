DESTDIR = /usr
INSTALL_DIR = $(DESTDIR)/sbin/

GLIBC += -L.. -I.. $(shell pkg-config --cflags --libs glib-2.0)

CFLAGS += -O2 -g -Wextra -Wstrict-prototypes -Wno-sign-compare \
		-Wimplicit-function-declaration -Wno-unused-parameter \
		-Wno-missing-field-initializers $(GLIBC) 

PROGRAMS += server client

.PHONY:all
all: $(PROGRAMS)

server: server.o 
	$(CC) $^ -o $@  $(GLIBC)
client: client.o 
	$(CC) $^ -o $@  $(GLIBC)

-include server.d

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) -MF $*.d -MT $*.o $*.c

.PHONY: install
install: $(PROGRAMS)
	install -d -m 755 $(INSTALL_DIR)
	install -m 755 $(PROGRAMS) $(INSTALL_DIR)

.PHONY: uninstall
uninstall:
	rm -f $(INSTALL_DIR)/$(PROGRAMS)

.PHONY: clean
clean:
	rm -f *.[od] $(PROGRAMS)
