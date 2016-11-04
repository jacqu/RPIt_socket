/*
 * rpit_socket_server : client of distant PC answering requests from
 * 											RPIt socket block.
 * 
 * Compile with : gcc -Wall -o rpit_socket_client -lpthread -lrt rpit_socket_client.c
 * 
 * JG, July 16 2016.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define RPIT_SOCKET_DISPLAY_MES

/* Check that these definitions are identical in server code */

#define RPIT_SOCKET_CON_N					10			// Nb of double sent (control)
#define RPIT_SOCKET_MES_N					10			// Nb of double returned (measurement)
#define RPIT_SOCKET_PORT					"31415"	// Port of the sever
#define RPIT_SOCKET_TIMEOUT				80000		// Server answering timeout in us
#define RPIT_SOCKET_SERVER_START	1000000	// Server startup time in us
#define RPIT_SOCKET_PERIOD				2000		// Period of the data update thread in us
#define RPIT_SIZEOF_IP            20      // Size of an IP address
#define RPIT_SOCKET_MAGIC					3141592	// Magic number
#define RPIT_SOCKET_MAX_INSTANCES	5				// Max number of client instances 

struct RPIt_socket_mes_struct	{
	unsigned int				magic;							// Magic number
	unsigned long long 	timestamp;					// Absolute server time in ns 
	double							mes[RPIT_SOCKET_MES_N];	// Measurements
};

struct RPIt_socket_con_struct	{
	unsigned int				magic;							// Magic number
	unsigned long long 	timestamp;					// Absolute client time in ns
	double							con[RPIT_SOCKET_CON_N];	// Control signals
};

struct RPIt_socket_instance_struct	{
	unsigned char									ip1;			// IP address of the server
	unsigned char									ip2;
	unsigned char									ip3;
	unsigned char									ip4;
	int														sfd;			// Socket file descriptor
	pthread_t											update_thread;
	pthread_mutex_t 							update_mutex;
	struct RPIt_socket_mes_struct	mes;			// Measurements
	struct RPIt_socket_con_struct	con;			// Control signals
};



struct RPIt_socket_instance_struct				instances[RPIT_SOCKET_MAX_INSTANCES];
unsigned int															nb_instances = 0;

unsigned char															exit_req = 0;

/*
 *	rpit_socket_client_ip2id : converts an ip address to instance ID
 */
int rpit_socket_client_ip2id( 	unsigned char ip1, 
																unsigned char ip2, 
																unsigned char ip3, 
																unsigned char ip4 )	{
	int	i;
	
	/* Scan the instance array looking for the given IP address */
	
	for ( i = 0; i < nb_instances; i++ )	{
		if (	( ip1 == instances[i].ip1 ) &&
					(	ip2 == instances[i].ip2 ) &&
					(	ip3 == instances[i].ip3 ) &&
					(	ip4 == instances[i].ip4 ) )
			return i;
	}
	
	/* IP address was not found */
																					
	return -1;
}

/* 
 * rpit_socket_client_update : thread communicating with the server
 */
void* rpit_socket_client_update( void* prt )	{
	struct RPIt_socket_instance_struct* instance = (struct RPIt_socket_instance_struct*)prt;
	ssize_t                   		nread;
	struct timespec           		before_time, after_time;
	unsigned long long        		period;
  struct RPIt_socket_mes_struct	local_mes;
  struct RPIt_socket_con_struct	local_con;
	#ifdef RPIT_SOCKET_DISPLAY_MES
	int                       		i;
	#endif
  
	while ( 1 )	{
		
		/* Check if exit is requested */
		
		if ( exit_req )
			break;
		
		
		/* Check if instance pointer is consistent */
		
		if ( !instance )
			continue;
		
		/* Wait for the thread refresh period */
		
		usleep( RPIT_SOCKET_PERIOD );
		
		/* Check if socket exists */
		
		if ( !instance->sfd )
			continue;
		
		/* Get time before sending request */
		
		clock_gettime( CLOCK_MONOTONIC, &before_time );
			
		/* Update control signals */
				
		pthread_mutex_lock( &instance->update_mutex );
		memcpy( &local_con, &instance->con, sizeof( struct RPIt_socket_con_struct ) );
		pthread_mutex_unlock( &instance->update_mutex );
		
		/* Update timestamp */
		
		local_con.timestamp = (unsigned long long)before_time.tv_sec * 1000000000
												+ (unsigned long long)before_time.tv_nsec;
		
		/* 
			Send control packet and read measurement packet from server.
		*/
		
		if (	write(	instance->sfd, 
									(char*)&local_con, 
									sizeof( struct RPIt_socket_con_struct ) ) != 
					sizeof( struct RPIt_socket_con_struct ) )
			fprintf( stderr, "rpit_socket_client_update: partial/failed write on %d.%d.%d.%d.\n",
																												instance->ip1,
																												instance->ip2,
																												instance->ip3,
																												instance->ip4 );
		
		nread = read( instance->sfd, (char*)&local_mes, sizeof( struct RPIt_socket_mes_struct ) );
		
		/* Get time after receiving response */
		
		clock_gettime( CLOCK_MONOTONIC, &after_time );
		
		/* Compute round-trip duration */
		
		period = (unsigned long long)after_time.tv_sec * 1000000000
					 + (unsigned long long)after_time.tv_nsec
				 - ( (unsigned long long)before_time.tv_sec * 1000000000
					 + (unsigned long long)before_time.tv_nsec );
		if ( period / 1000 > 2 * RPIT_SOCKET_PERIOD )
			fprintf( stderr, 
			"rpit_socket_client_update: laggy connection with %d.%d.%d.%d. Round-trip duration : %llu us\n", 
																												instance->ip1,
																												instance->ip2,
																												instance->ip3,
																												instance->ip4,
																												period / 1000 );
		
		if ( nread == -1 ) {
			fprintf( stderr, "rpit_socket_client_update: read error from %d.%d.%d.%d.\n",
																										instance->ip1,
																										instance->ip2,
																										instance->ip3,
																										instance->ip4 );
		}
		else
		{
			if ( nread != sizeof( struct RPIt_socket_mes_struct ) )
				fprintf( stderr, 
				"rpit_socket_client_update: received %zd bytes from %d.%d.%d.%d instead of %zd.\n", 
																							nread, 
																							instance->ip1,
																							instance->ip2,
																							instance->ip3,
																							instance->ip4,
																							sizeof( struct RPIt_socket_mes_struct ) );
			else
			{
				if ( local_mes.magic != RPIT_SOCKET_MAGIC )
					fprintf( stderr, 
				"rpit_socket_client_update: received bad magic number from %d.%d.%d.%d.\n",  
																							instance->ip1,
																							instance->ip2,
																							instance->ip3,
																							instance->ip4 );
				else
				{
					/* Update mes */
					
					pthread_mutex_lock( &instance->update_mutex );
					memcpy( &instance->mes, &local_mes, sizeof( struct RPIt_socket_mes_struct ) );
					pthread_mutex_unlock( &instance->update_mutex );
					
					/* Display measurements */
					
					#ifdef RPIT_SOCKET_DISPLAY_MES
					printf( "> Server IP: %d.%d.%d.%d\n",
									instance->ip1,
									instance->ip2,
									instance->ip3,
									instance->ip4 );
					printf( "> Timestamp : %llu\n", local_mes.timestamp );
					for ( i = 0; i < RPIT_SOCKET_MES_N; i++ )
						printf( "> mes[%d] = %e\n", i, local_mes.mes[i] );
					#endif
				}
			}
		}
	}
	
	return NULL;
}

/*
 * rpit_socket_client_add : add new connection to a server
 */
void rpit_socket_client_add( 	unsigned char ip1, 
															unsigned char ip2, 
															unsigned char ip3, 
															unsigned char ip4 )	{
																
	struct addrinfo 			hints;
	struct addrinfo 			*result, *rp;
	int 									s;
	struct timeval 				tv;
  char                  ip[RPIT_SIZEOF_IP];
  int										sfd;
  
  /* Compute IP address */
  
  snprintf( ip, RPIT_SIZEOF_IP, "%d.%d.%d.%d", ip1, ip2, ip3, ip4 );

	/* Obtain address(es) matching host/port */

	memset( &hints, 0, sizeof( struct addrinfo ) );
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	s = getaddrinfo( ip, RPIT_SOCKET_PORT, &hints, &result );
	if ( s != 0 ) {
		fprintf( stderr, "rpit_socket_client: function getaddrinfo returned: %s\n", gai_strerror( s ) );
		return;
	}
	
	/* 
		getaddrinfo() returns a list of address structures.
		Try each address until we successfully connect(2).
		If socket(2) (or connect(2)) fails, we (close the socket
		and) try the next address.
	*/

	for ( rp = result; rp != NULL; rp = rp->ai_next )	{
		sfd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
		if ( sfd == -1 )
			continue;

		if ( connect( sfd, rp->ai_addr, rp->ai_addrlen ) != -1 )
			break;									/* Success */

		close( sfd );
	}

	if ( rp == NULL )	{					/* No address succeeded */
		fprintf( stderr, "rpit_socket_client: could not connect. Aborting.\n" );
		return;
	}

	freeaddrinfo( result );			/* No longer needed */
	
	/* Set socket timeout */
	
	tv.tv_sec =		0;
	tv.tv_usec = 	RPIT_SOCKET_TIMEOUT;
	setsockopt( sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval) );
	
	/* Add connection to instances structure */
	
	if (nb_instances == RPIT_SOCKET_MAX_INSTANCES )
		return;
		
	/* Reset structure data */
	
	memset( &instances[nb_instances], 0, sizeof( struct RPIt_socket_instance_struct ) );
	
	/* Initialize connection data */
	
	instances[nb_instances].ip1 = ip1;
	instances[nb_instances].ip2 = ip2;
	instances[nb_instances].ip3 = ip3;
	instances[nb_instances].ip4 = ip4;
	instances[nb_instances].sfd = sfd;
	
	/* Initialize magic number in control structure */
	
	instances[nb_instances].con.magic = RPIT_SOCKET_MAGIC;
	
	/* Initialize mutex */
	
	pthread_mutex_init( &instances[nb_instances].update_mutex, NULL );
	
	/* Start one thread for each server */
	
	pthread_create( &instances[nb_instances].update_thread, 
									NULL, 
									rpit_socket_client_update, 
									(void*) &instances[nb_instances] );
	
	/* Increment instance counter */
  
  nb_instances++;
  
  /* Wait so that the periodic thread can update the measurement */
  
  usleep( RPIT_SOCKET_SERVER_START );
}
 
/* 
 * rpit_socket_client_close : close communication with the server
 */
void rpit_socket_client_close( void )	{
	
	int i;
	
	/* Request termination of the thread */
	
	exit_req = 1;

	/* Wait for all threads to terminate */
	
	for ( i = 0; i < nb_instances; i++ )
		pthread_join( instances[i].update_thread, NULL );
	
	/* Close each socket */
	
	for ( i = 0; i < nb_instances; i++ )	{
		close( instances[i].sfd );
		instances[i].sfd = 0;
	}
}

int main( void )	{
	
	rpit_socket_client_add( 127, 0, 0, 1 );
	rpit_socket_client_add( 130, 79, 73, 41 );
	
	sleep( 10 );
	
	rpit_socket_client_close(  );

return 0;
}

