ser: auth.c  ser_main.c server.c
	gcc -o ser auth.c ser_main.c server.c

clean:
	rm ser
