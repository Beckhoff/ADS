prefix ?= /usr
BINARY = build/adstool

$(BINARY): build
	ninja -C $(@D)

build:
	meson setup $@

install: $(BINARY)
	install --mode=755 -D $(BINARY) "$(DESTDIR)$(prefix)/bin/$(notdir $(BINARY))"

clean:
	rm -rf build
	rm -rf example/build
