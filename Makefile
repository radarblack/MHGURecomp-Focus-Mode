NAME := FocusMode
VERSION ?= 0.1.0
BUILD ?= release
OUT ?= build
DEBUG_LEVEL ?= 0
SRC := src/module.c src/host.c src/focus.c src/config.c src/platform.c src/math.c
COMMON := -std=c11 -Wall -Wextra -Wpedantic -fvisibility=hidden -Iinclude -Isrc -DFM_VERSION='"$(VERSION)"'
ifeq ($(BUILD),debug)
OPT := -O0 -g
else
OPT := -O2
endif
WINDOWS_CC ?= x86_64-w64-mingw32-gcc
LINUX_CC ?= gcc

.PHONY: all windows linux package-windows clean

all: windows

windows: $(OUT)/windows/$(NAME).dll

$(OUT)/windows/$(NAME).dll: $(SRC) src/*.h include/*.h
	@mkdir -p $(dir $@)
	$(WINDOWS_CC) $(OPT) $(COMMON) -D_WIN32 -shared -static-libgcc -o $@ $(SRC) -lm -luser32

linux: $(OUT)/linux/$(NAME).so

$(OUT)/linux/$(NAME).so: $(SRC) src/*.h include/*.h
	@mkdir -p $(dir $@)
	$(LINUX_CC) $(OPT) $(COMMON) -fPIC -shared -o $@ $(SRC) -ldl -lpthread -lm

package-windows: windows
	rm -rf $(OUT)/pkg-windows
	mkdir -p $(OUT)/pkg-windows/engine_modules/$(NAME)
	cp $(OUT)/windows/$(NAME).dll $(OUT)/pkg-windows/engine_modules/$(NAME)/
	cp config/$(NAME).ini $(OUT)/pkg-windows/engine_modules/$(NAME)/
	cd $(OUT)/pkg-windows && zip -qr ../$(NAME)-Windows-$(VERSION).zip .

clean:
	rm -rf $(OUT)
