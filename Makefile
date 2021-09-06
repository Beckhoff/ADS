prefix ?= /usr
BINARY = build/adstool
MANPAGE = doc/build/man/adstool.1

$(BINARY): build
	ninja -C $(@D)

$(MANPAGE): $(DOCFILES)
	make -C doc/ man

build:
	meson setup $@

install: $(BINARY) $(MANPAGE)
	install --mode=755 -D $(BINARY) "$(DESTDIR)$(prefix)/bin/$(notdir $(BINARY))"
	install --mode=755 -D $(MANPAGE) "$(DESTDIR)$(prefix)/share/man/man1/$(notdir $(MANPAGE))"

clean:
	rm -rf build
	rm -rf doc/build
	rm -rf example/build
