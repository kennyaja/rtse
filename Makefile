init:
	gcc -o rtse-server rtse-server.c
	gcc -o rtse rtse.c

localinstall:
	cp rtse ~/.local/bin/
	cp rtse-server ~/.local/bin/

install:
	cp rtse /bin/
	cp rtse-server /bin/

localuninstall:
	rm ~/.local/bin/rtse ~/.local/bin/rtse-server

uninstall:
	rm /bin/rtse /bin/rtse-server

clean:
	rm rtse rtse-server

