#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>


#define SERVERIP "127.0.0.1"
#define SERVERPORT "4950"	
#define MSG_SIZE 1024
#define MES_PERIOD     5000  /* Sampling period of the measurement
			      * (us) */

#define RESET_MES_PERIOD 1000 /* Number of iterations before resetting
			      * the measurements*/
#define DELTA_X (0.1) 
#define DELTA_Y (0.2) 
#define DELTA_Z (0.3) 


/* char msg[] = "TS0004,5.9680,-2.7492,-0.0237,10.01.2025,14:38:34.54,8823770\n";  */
char msg[MSG_SIZE]; 

int sockfd;

void
sigint_handler (int signum)
{
     printf("Closing socket...");
     close(sockfd);
     exit(0);
}

void measure_time(struct timespec *current_time, uint64_t *timestamp)
{
     clock_gettime(CLOCK_MONOTONIC_RAW, current_time);
     *timestamp = (uint64_t) current_time->tv_sec * 1000000000 +
	  (uint64_t) current_time->tv_nsec; 
}

int main(int argc, char *argv[])
{
     
      /* SIGINT Handling */
     struct sigaction new_action;
     new_action.sa_handler = sigint_handler;
     sigemptyset (&new_action.sa_mask);
     new_action.sa_flags = 0;
     sigaction (SIGINT, &new_action, NULL);

     /* Time measurement */
     struct timespec current_time; 
     uint64_t timestamp; 
     measure_time(&current_time, &timestamp);

     /* Message initialization */
     double x = 1.0, y = 2.0, z = 3.0; 

     snprintf(msg, MSG_SIZE, "Pointid,%.3f,%.3f,%.3f,%ld", x, y, z, timestamp);

     /* Socket configuration */
     struct addrinfo hints, *servinfo, *p;
     int rv;
     int numbytes;

     memset(&hints, 0, sizeof hints);
     /* hints.ai_family = AF_INET6; // set to AF_INET to use IPv4 */
     hints.ai_family = AF_INET; // set to AF_INET to use IPv4
     hints.ai_socktype = SOCK_DGRAM;

     if ((rv = getaddrinfo(SERVERIP, SERVERPORT, &hints, &servinfo)) != 0) {
	  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	  return 1;
     }

     /* loop through all the results and make a socket */
     for(p = servinfo; p != NULL; p = p->ai_next) {
	  if ((sockfd = socket(p->ai_family, p->ai_socktype,
			       p->ai_protocol)) == -1) {
	       perror("talker: socket");
	       continue;
	  }

	  break;
     }

     if (p == NULL) {
	  fprintf(stderr, "talker: failed to create socket\n");
	  return 2;
     }

     freeaddrinfo(servinfo);

     uint cnt = 0; 
     while(1) {

	  measure_time(&current_time, &timestamp);
	  snprintf(msg, MSG_SIZE, "Pointid,%.3f,%.3f,%.3f,%ld", x, y, z, timestamp);

	  if ((numbytes = sendto(sockfd, msg, strlen(msg), 0,
				 p->ai_addr, p->ai_addrlen)) == -1) {
	       perror("talker: sendto");
	       exit(1);
	  }
	  /* printf("talker: sent %d bytes to %s\n", numbytes, SERVERPORT); */
	  /* printf("sizeof msg: %ld\n", sizeof msg);  */
	  /* printf("strlen(msg): %ld\n", strlen(msg));  */


	  if (cnt == RESET_MES_PERIOD ) {
	       cnt = 0;
	       x = 0.0, y = 0.0, z = 0.0; 
	  } else {
	       x = x + DELTA_X;
	       y = y + DELTA_Y;
	       z = z + DELTA_Z;
	       cnt++; 
	  }

	  usleep( MES_PERIOD );
     }

     return 0;
}
