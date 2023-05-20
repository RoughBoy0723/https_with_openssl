CC = gcc
CFLAGS = -lssl -lcrypto -pthread

SFILES = OpenSSL_HTTPS.c

SERVER = server

all : $(CLIENT) $(SERVER)

$(SERVER) : $(SFILES)
	$(CC) -o $(SERVER)	$(SFILES)	$(CFLAGS)
curl:
	curl	-k	https://localhost:5555/index.html	--output	-
wire:
	tshark	-i	lo	-f	"tcp	port	5555"	-a	duration:60	-w	tls.pcapng
clean : 
	rm -f *.o core core.*
