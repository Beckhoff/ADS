prefix ?= /usr
WITH_TWINCAT_ROUTER ?= 1
TCADSDLL_INCLUDE ?= /usr/include
TCADSDLL_LIB ?= /usr/lib
NINJAFLAGS ?=
BINARY = build/adstool
TCBINARY = build/tcadstool
LIBRARY = build/libAdsLib.a
MANPAGE = doc/build/man/adstool.1

BINARIES = $(BINARY)
ifeq ($(WITH_TWINCAT_ROUTER),1)
BINARIES += $(TCBINARY)
MESON_ROUTER_OPTIONS = -Dtcadsdll_include="$(TCADSDLL_INCLUDE)" -Dtcadsdll_lib="$(TCADSDLL_LIB)"
else ifeq ($(WITH_TWINCAT_ROUTER),0)
MESON_ROUTER_OPTIONS = -Dtcadsdll_include= -Dtcadsdll_lib=
else
$(error WITH_TWINCAT_ROUTER must be 0 or 1)
endif

$(BINARIES): build
	ninja $(NINJAFLAGS) -C $(@D)

$(MANPAGE): $(DOCFILES)
	make -C doc/ man

build:
	meson setup $@ $(MESON_ROUTER_OPTIONS)

install: $(BINARIES) $(MANPAGE)
	install --mode=755 -D $(BINARY) "$(DESTDIR)$(prefix)/bin/$(notdir $(BINARY))"
ifeq ($(WITH_TWINCAT_ROUTER),1)
	install --mode=755 -D $(TCBINARY) "$(DESTDIR)$(prefix)/bin/$(notdir $(TCBINARY))"
endif
	install --mode=755 -D $(LIBRARY) "$(DESTDIR)$(prefix)/lib/$(notdir $(LIBRARY))"
	install --mode=755 -D $(MANPAGE) "$(DESTDIR)$(prefix)/share/man/man1/$(notdir $(MANPAGE))"
	find AdsLib -type f -name "*.h" -exec install --mode=644 -D {} "$(DESTDIR)$(prefix)/include/{}" \;

uncrustify:
	./tools/run-uncrustify.sh format

clean:
	rm -rf build
	rm -rf doc/build
	rm -rf example/build
