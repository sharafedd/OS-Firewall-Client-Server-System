#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


#define BUFFERLENGTH 256

/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(0);
}

int formatChecker (const char* inp) {
    char str1[50], str2[50];
    int num1, num2;

    if (sscanf(inp, "%49s %d", str1, &num1) == 2)
        return 1;

    if (sscanf(inp, "%49s-%49s %d-%d", str1, str2, &num1, &num2) == 4)
        return 1;

    return 0;
}

int main(int argc, char *argv[]) {
    int sockfd, n;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int res;
    

    char buffer[BUFFERLENGTH];
    if (argc < 3) {
       fprintf (stderr, "usage %s hostname port\n", argv[0]);
       exit(1);
    } else if (argc < 4) {
       fprintf (stderr, "Illegal request\n");
       exit(1);
    }

    if (argv[3][1] != '\0') {
        fprintf (stderr, "Illegal request\n");
        exit(1);
    }

    char Req = argv[3][0];
    if (Req != 'A' && Req != 'C' && Req != 'D' && Req != 'L') {
        fprintf (stderr, "Illegal request\n");
        exit(1);
    }

   /* Obtain address(es) matching host/port */
   /* code taken from the manual page for getaddrinfo */
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    res = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (res != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
	exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */
    
    for (rp = result; rp != NULL; rp = rp->ai_next) {
	sockfd = socket(rp->ai_family, rp->ai_socktype,
			rp->ai_protocol);
	if (sockfd == -1)
	    continue;

	if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
	    break;                  /* Success */

	close(sockfd);
    }

    if (rp == NULL) {               /* No address succeeded */
	fprintf(stderr, "Could not connect\n");
	exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */


    // CODE !!

    if (argv[3][0] == 'A' || argv[3][0] == 'C' || argv[3][0] == 'D') {
        if (argc != 6) {
            fprintf (stderr, "ERROR: Not A Valid Request\n");
            exit(1);
        }
        char *IPs = argv[4];
        char *Ports = argv[5];
        snprintf(buffer, BUFFERLENGTH, "%c %s %s", Req, IPs, Ports);
    } else if (argv[3][0] == 'L' && argc == 4) {
        snprintf(buffer, BUFFERLENGTH, "%c ", Req);
    }

    /* send message */
    n = write (sockfd, buffer, BUFFERLENGTH - 1);
    if (n < 0) {
        error ("ERROR writing to socket");
    }

    /* wait for reply */
    n = read (sockfd, buffer, BUFFERLENGTH - 1);
    if (n < 0) 
        error ("ERROR reading from socket");
    if (strcmp (buffer, "start_loop") == 0) {
        n = read (sockfd, buffer, BUFFERLENGTH - 1);
        if (n < 0) {
            strerror(errno);
            error ("ERROR  5 reading from socket");
        }
        while (strcmp (buffer, "end_loop") != 0) {
            printf ("%s",buffer);
            n = sprintf (buffer, " ");
            n = read (sockfd, buffer, BUFFERLENGTH - 1);
            if (n < 0) 
                error ("ERROR reading from socket");
        }
    } else {
        printf ("%s",buffer);
    }
    
    

    close(sockfd);
    return 0;
}
