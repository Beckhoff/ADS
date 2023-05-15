prefix ?= /usr
BINARY = build/adstool
TCBINARY = build/tcadstool
LIBRARY = build/libAdsLib.a
MANPAGE = doc/build/man/adstool.1

$(BINARY) $(TCBINARY): build
	ninja -C $(@D)

$(MANPAGE): $(DOCFILES)
	make -C doc/ man

build:
	meson setup $@ -Dtcadsdll_include=/usr/include -Dtcadsdll_lib=/usr/lib

install: $(BINARY) $(TCBINARY) $(MANPAGE)
	install --mode=755 -D $(BINARY) "$(DESTDIR)$(prefix)/bin/$(notdir $(BINARY))"
	install --mode=755 -D $(TCBINARY) "$(DESTDIR)$(prefix)/bin/$(notdir $(TCBINARY))"
	install --mode=755 -D $(LIBRARY) "$(DESTDIR)$(prefix)/lib/$(notdir $(LIBRARY))"
	install --mode=755 -D $(MANPAGE) "$(DESTDIR)$(prefix)/share/man/man1/$(notdir $(MANPAGE))"
	find AdsLib -type f -name "*.h" -exec install --mode=644 -D {} "$(DESTDIR)$(prefix)/include/{}" \;

uncrustify:
	./tools/run-uncrustify.sh format

clean:
	rm -rf build
	rm -rf doc/build
	rm -rf example/build
