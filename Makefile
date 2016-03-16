BUNDLE = csynth.lv2
INSTALL_DIR = /home/jesse/.lv2

$(BUNDLE): manifest.ttl csynth.ttl csynth.so csynth_gui.so docs
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp manifest.ttl csynth.ttl csynth.so csynth_gui.so $(BUNDLE)
	cp -R presets $(BUNDLE)
	cp -R lib $(BUNDLE)

test: lib/*.h lib/*.cpp
	g++ -std=c++11 -Wall -Werror -fPIC lib/test.cpp -lm -o lib/runtest && lib/runtest

csynth.so: csynth.c csynth.h patch.h uris.h
	gcc -std=c99 -D_POSIX_C_SOURCE=2 -Werror -g -shared -fPIC -DPIC csynth.c -o csynth.so -lm # `pkg-config --cflags --libs lv2-plugin`

csynth_gui.so: csynth_gui.c csynth.h patch.h uris.h
	gcc -std=c99 -D_POSIX_C_SOURCE=2 -Werror -g -shared -fPIC -DPIC csynth_gui.c -o csynth_gui.so -lm `pkg-config --cflags --libs gtk+-2.0`

docs: lib/*.h extract-docs.sh
	./extract-docs.sh

install: $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE)

clean:
	rm -rf $(BUNDLE) *.so
	rm -f lib/run*
