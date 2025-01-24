#ifndef SENSOR_UTILS_H
#define SENSOR_UTILS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define X_ID 1
#define Y_ID 2
#define Z_ID 3

#define SENSOR_IP "127.0.0.1"
#define SENSOR_PORT "4950"	// the port users will be connecting to
#define MAXBUFLEN 1024
#define DELIM ',' // character used to delimit the various fields of the message


double string_to_double ( char *str );
int parse_total_station_msg( char *msg, double *x, double *y, double *z);
void socket_configuration( int *sockfd );

#endif /* SENSOR_UTILS_H */
