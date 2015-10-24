all:
	$(CC) dmz.c -o dmz
install:
	mv ./dmz /usr/local/bin/dmz
  
