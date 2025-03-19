init:
	gcc -o rtse rtse.c

install:
ifdef INSTALL_DIR
	cp rtse $(INSTALL_DIR)
else
	cp rtse /bin
endif

uninstall:
	rm /bin/rtse

clean:
	rm rtse

