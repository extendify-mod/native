%::
	$(MAKE) $(MAKEFLAGS) -C build $(MAKECMDGOALS)

.PHONY: all
all:
	$(MAKE) $(MAKEFLAGS) -C build
.PHONY: configure_debug
configure_debug:
	cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug

.PHONY: configure
configure: configure_debug

.PHONY: configure_release
configure_release:
	cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
	