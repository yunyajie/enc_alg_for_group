CC=g++
target_server=testserver
target_client=testclient
object_server=server/epoller.o server/main.o server/server.o conn/conn.o timer/heaptimer.o PH_Cipher/PH_Cipher.o PH_Cipher/PH_Member.o pool/sqlconnpool.o
object_client=client/client.o client/main.o 
object_common=buffer/buffer.o log/log.o
LDFLAGS=-lgmp -lgmpxx -pthread

all:$(target_server) $(target_client)

$(target_server):$(object_server) $(object_common)
	$(CC) -o $@ $^ $(LDFLAGS) -lmysqlclient

$(target_client):$(object_client) $(object_common)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o:%.c
	$(CC) -c $< -o $@

.PHONY:clean
clean:
	rm -f $(object_server) $(object_client) $(object_common) testserver testclient log/*.log *.log temp/*.log