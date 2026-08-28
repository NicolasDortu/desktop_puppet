SRCS = $(wildcard src/*.c)

CFLAGS  ?=
LDLIBS   = -lraylib -lgdi32 -lwinmm
INCLUDES = -I src -I include -L lib

.PHONY: default release clean dist sync-assets

default: bin/main.exe sync-assets

# Distributable build: optimized, stripped, and -mwindows so the game (and
# every child process it spawns) runs without a console window popping up.
release: CFLAGS += -O2 -DNDEBUG -mwindows -s
release: clean bin/main.exe sync-assets

bin/main.exe: $(SRCS) bin/icon.o
	gcc $(CFLAGS) -o $@ $(SRCS) bin/icon.o $(INCLUDES) $(LDLIBS)

bin/icon.o: assets/icon.rc assets/icon.ico
	windres assets/icon.rc -o $@

# main.exe looks for assets/ next to itself (LoadAssetTexture/LoadAssetSound
# in renderer.c), so mirror the tracked assets/ folder into bin/ on every
# build. Always-copy instead of timestamp tracking: cheap (a few small
# files) and can't go stale if only an asset changed.
sync-assets:
	@mkdir -p bin/assets
	@cp -r assets/. bin/assets/

clean:
	rm -f bin/main.exe bin/icon.o
	rm -rf bin/assets

# Zip main.exe (renamed to a friendly name) + its assets into
# dist/DesktopPuppet.zip: both sit flat at the top of the folder, so a player
# just unzips and double-clicks -- ready for itch.io.
dist: release
	rm -rf dist
	mkdir -p dist/DesktopPuppet
	cp "bin/main.exe" "dist/DesktopPuppet/Desktop Buddy.exe"
	cp -r assets dist/DesktopPuppet/
	rm -f dist/DesktopPuppet/assets/icon.rc dist/DesktopPuppet/assets/icon.ico # build inputs; icon is embedded in the exe
	cp itch.toml dist/DesktopPuppet/
	powershell -NoProfile -Command "Compress-Archive -Path 'dist/DesktopPuppet' -DestinationPath 'dist/DesktopPuppet.zip' -Force"
