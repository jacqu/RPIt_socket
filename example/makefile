all: rpit_socket_server sensor_stream

CFLAGS= -lc -lrt -Wall -pedantic-errors -lpthread 

sensor_utils.o: sensor_utils.c sensor_utils.h 
	gcc -c -o $@ $< $(CFLAGS)

rpit_socket_server.o: rpit_socket_server.c sensor_utils.c sensor_utils.h
	gcc -c -o $@  $< $(CFLAGS)

rpit_socket_server: rpit_socket_server.o sensor_utils.o 
	gcc  -o $@ rpit_socket_server.o sensor_utils.o

rpit_socket_client: rpit_socket_client.c
	gcc -o $@ $< $(CFLAGS)

sensor_stream: sensor_stream.c
	gcc -o $@ $< $(CFLAGS)

clean:
	rm rpit_socket_server rpit_socket_server.o sensor_utils.o sensor_stream
