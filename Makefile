build:
	gcc -o server server.c
	gcc -o client client.c

clean:
	rm -f server client
	rm -f client-*.log
