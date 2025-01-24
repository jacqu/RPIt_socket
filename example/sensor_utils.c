#include "sensor_utils.h" 

double string_to_double ( char *str )
{
     double x; 
     errno = 0;    /* To distinguish success/failure after call */
     x = strtod(str, NULL); 
     if (errno != 0) {
	  flockfile( stderr );
	  fprintf( stderr, "strtod: string to double conversion error\n");
	  funlockfile( stderr );
	  x = NAN; 
     }

     return x; 
}

/* returns -1 if parsing failed, otherwise returns 0 */
int parse_total_station_msg( char *msg, double *x, double *y, double *z)
{
     int output = 0; 
     char delim = ','; 
     char *saveptr = NULL; 
     char *token; 
     uint cnt = 0; 

     size_t msg_len = strlen(msg) + 1;
     char *msg_cpy = NULL; // needed because strtok_r modifies its inputs
     msg_cpy = (char *) malloc( msg_len * sizeof( char ) ); 

     strcpy(msg_cpy, msg); 

     // First call
     token = strtok_r(msg_cpy, &delim, &saveptr); 

     while (token != NULL ) {
	  /* printf("token = %s\n", token);  */

	  if ( cnt == X_ID ) {
	       *x = string_to_double(token); 
	       if ( isnan(*x) ) output = -1; 

	  } else if ( cnt == Y_ID ) {
	       *y = string_to_double(token); 
	       if ( isnan(*y) ) output = -1; 

	  } else if ( cnt == Z_ID ) {
	       *z = string_to_double(token); 
	       if ( isnan(*z) ) output = -1; 

	  }
	  token = strtok_r(NULL, &delim, &saveptr); 
	  cnt ++; 
     }

     free(msg_cpy); 
     return output; 
}

void socket_configuration( int *sockfd )
{
     int status; 
     struct addrinfo hints; // used to hold socket configuration
     struct addrinfo *servinfo; // linked list returned by getaddrinfo
     struct addrinfo *p; // element used to iterate over servinfo

     memset(&hints, 0, sizeof hints);
     hints.ai_family = AF_UNSPEC; // IPv4 of IPv6
     hints.ai_socktype = SOCK_DGRAM; // UDP 
     hints.ai_flags = AI_PASSIVE; // use my IP

     if ((status = getaddrinfo(NULL, SENSOR_PORT, &hints, &servinfo)) != 0) {
	  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
	  exit( EXIT_FAILURE );
     }

     // loop through all the results and bind to the first we can
     for(p = servinfo; p != NULL; p = p->ai_next) {
	  if ((*sockfd = socket(p->ai_family, p->ai_socktype,
			       p->ai_protocol)) == -1) {
	       perror("listener: socket");
	       continue;
	  }

	  if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	       close(*sockfd);
	       perror("listener: bind");
	       continue;
	  }

	  break;
     }

     if (p == NULL) {
	  fprintf(stderr, "listener: failed to bind socket\n");
	  exit( EXIT_FAILURE );
     }

     freeaddrinfo(servinfo);
}
