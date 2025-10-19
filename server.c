/* A threaded server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define BUFFERLENGTH 256

#define THREAD_IN_USE 0
#define THREAD_FINISHED 1
#define THREAD_AVAILABLE 2
#define THREADS_ALLOCATED 10

/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(1);
};

struct threadArgs_t {
    int newsockfd;
    int threadIndex;
};

// Defining Queries Structure
struct queries {
char *ip;
int port;
}; 

// Defining Rules Structure
struct rules {
char *ip1;
char *ip2;
int port1;
int port2;
struct queries* listHead;
int queries_number;
};      

// List of Rules
struct rules* lines = NULL;
int lines_number = 0;

// function to check ip
int checkip (const char *str) {
  int group = 0;
  int num_count = 0;
  int group_count = 0;
    while (*str != '\0') {
      if (isdigit((unsigned char) *str)){
        group = group * 10 + (*str - '0');
        if (group > 255) {
          return 1;
        }
        num_count++;
        if (num_count > 3) {
          return 1;
        }
      } else if (*str == '.') {
        if (num_count == 0) {
          return 1;
        }
        group = 0;
        num_count = 0;
        group_count++;
        if (group_count > 3) {
          return 1;
        }
      } else {
        return 1;
      }
      str++;
    }

    if (group_count != 3) {
      return 1;
    }
    return 0;
}

int sortip (char *str1, char *str2) {
  int octet1[4], octet2[4];
  sscanf(str1, "%d.%d.%d.%d", &octet1[0], &octet1[1], &octet1[2], &octet1[3]);
  sscanf(str2, "%d.%d.%d.%d", &octet2[0], &octet2[1], &octet2[2], &octet2[3]);
  int i = 0;
  while (i < 4) {
    if (octet1[i] < octet2[i]) {return -1;}
    else if (octet1[i] > octet2[i]) {return 1;}
    i++;
  }
  return -1;
}

int isExecuted = 0;
int returnValue = 0; /* not used; need something to keep compiler happy */
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER; /* the lock used for processing */

/* this is only necessary for proper termination of threads - you should not need to access this part in your code */
struct threadInfo_t {
    pthread_t pthreadInfo;
    pthread_attr_t attributes;
    int status;
};
struct threadInfo_t *serverThreads = NULL;
int noOfThreads = 0;
pthread_rwlock_t threadLock =  PTHREAD_RWLOCK_INITIALIZER;
pthread_cond_t threadCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t threadEndLock = PTHREAD_MUTEX_INITIALIZER;

/* For each connection, this function is called in a separate thread. You need to modify this function. */
void *processRequest (void *args) {
  pthread_mutex_lock (&mut);

    struct threadArgs_t *threadArgs;
  char buffer[BUFFERLENGTH];
  int n;

  threadArgs = (struct threadArgs_t *) args;
  bzero (buffer, BUFFERLENGTH);
  n = read (threadArgs->newsockfd, buffer, BUFFERLENGTH -1);
  if (n < 0) 
    error ("ERROR reading from socket");

  char *space = strchr(buffer, ' ');
  *space = '\0';

  if (buffer[0] == 'L') {
    if (lines_number == 0) {
      n = sprintf (buffer, "No rules\n");
      n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
      if (n < 0) 
        error ("ERROR writing to socket\n");
    } else {
      n = sprintf (buffer, "start_loop");
      n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
      for (int i = 0; i < lines_number; i++) {
        if (lines[i].port2 == 0) {
          n = sprintf (buffer, "Rule: %s %d\n", lines[i].ip1, lines[i].port1);
          n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
          if (n < 0) 
            error ("ERROR writing to socket\n");
          if (lines[i].listHead != NULL) {
            for (int y = 0; y < lines[i].queries_number; y++) {
              n = sprintf (buffer, "Query: %s %d\n", lines[i].listHead[y].ip, lines[i].listHead[y].port);
              n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
              if (n < 0) 
                error ("ERROR writing to socket\n");
            }
          }
        } else {
          n = sprintf (buffer, "Rule: %s-%s %d-%d\n", lines[i].ip1, lines[i].ip2, lines[i].port1, lines[i].port2);
          n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
          if (n < 0) 
            error ("ERROR writing to socket\n");
          if (lines[i].listHead != NULL) {
            if (lines[i].listHead != NULL) {
              for (int y = 0; y < lines[i].queries_number; y++) {
                n = sprintf (buffer, "Query: %s %d\n", lines[i].listHead[y].ip, lines[i].listHead[y].port);
                n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
                if (n < 0) 
                  error ("ERROR writing to socket\n");
              }
            }
          }
        }
      }
      n = sprintf (buffer, "end_loop");
      n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
      if (n < 0) 
        error ("ERROR writing to socket\n");
    }
  } else {
      char *space2 = strchr(space + 1, ' ');
      *space2 = '\0';
      char *tiret = strchr(space + 1, '-');
      char *tiret2 = strchr(space2 + 1, '-');
      int num = 0;
      int num2 = 0;
      int noMatch;
      switch (buffer[0]) {
        case 'A':
          if (tiret != NULL && tiret2 != NULL) {
            *tiret = '\0';
            *tiret2 = '\0';
            if (checkip(space + 1) == 0 && checkip(tiret + 1) == 0 && sscanf(space2 + 1, "%d", &num) == 1 && sscanf(tiret2 + 1, "%d",
&num2) == 1) {
              if (sortip(space + 1, tiret + 1) == -1 && num < num2) {
                if (num <= 65535 && num >= 0 && num2 <= 65535 && num2 >= 0) {
                  lines = (struct rules*)realloc(lines, (lines_number+1) *sizeof(struct rules));
                  if (lines == NULL) {
                  printf("Memory Allocation Error.\n");
                  }
                  lines[lines_number].ip1 = strdup(space + 1);
                  lines[lines_number].ip2 = strdup(tiret + 1);
                  lines[lines_number].port1 = num;
                  lines[lines_number].port2 = num2;
                  lines[lines_number].listHead = NULL;
                  lines[lines_number].queries_number = 0;
                  lines_number++;

                  n = sprintf (buffer, "Rule added\n");
                  n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
                  if (n < 0) 
                    error ("ERROR writing to socket\n");
                } else {
                    n = sprintf (buffer, "Invalid rule\n");
                    n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
                    if (n < 0) 
                      error ("ERROR writing to socket\n");
                }
              } else {
                n = sprintf (buffer, "Invalid rule\n");
                n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
                if (n < 0) 
                  error ("ERROR writing to socket\n");
              }
            } else {
              n = sprintf (buffer, "Invalid rule\n");
              n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
              if (n < 0) 
                error ("ERROR writing to socket\n");
            }
          } else if (tiret == NULL && tiret2 == NULL) {
            if (checkip(space + 1) == 0 && sscanf(space2 + 1, "%d", &num) == 1) {
              if (num <= 65535 && num >= 0) {
                lines = (struct rules*)realloc(lines, (lines_number+1) *sizeof(struct rules));
                if (lines == NULL) {
                printf("Memory Allocation Error.\n");
                }
                lines[lines_number].ip1 = strdup(space + 1);
                lines[lines_number].ip2 = NULL;
                lines[lines_number].port1 = num;
                lines[lines_number].port2 = 0;
                lines[lines_number].listHead = NULL;
                lines[lines_number].queries_number = 0;
                lines_number++;

                n = sprintf (buffer, "Rule added\n");
                n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
                  if (n < 0) 
                error ("ERROR writing to socket\n");
              } else {
                n = sprintf (buffer, "Invalid rule\n");
                n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
                if (n < 0) 
                  error ("ERROR writing to socket\n");
              }
            } else {
              n = sprintf (buffer, "Invalid rule\n");
              n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
              if (n < 0) 
                error ("ERROR writing to socket\n");
            }
          } else {
            n = sprintf (buffer, "Invalid rule\n");
            n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
            if (n < 0) 
              error ("ERROR writing to socket\n");
          }
          break;
        case 'C':
          noMatch = lines_number;
          if (checkip(space + 1) == 0 && sscanf(space2 + 1, "%d", &num) == 1) {
            if (num <= 65535 && num >= 0) {
              if (lines_number == 0) {
                n = sprintf (buffer, "Connection rejected\n");
                n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
                if (n < 0) 
                  error ("ERROR writing to socket\n");
              } else {
                for (int i = 0; i < lines_number; i++) {
                  if (lines[i].ip2 == NULL) { // single ip
                    if (strcmp(space + 1, lines[i].ip1) == 0 && num == lines[i].port1) { // same
                      lines[i].listHead = (struct queries*)realloc(lines[i].listHead, (lines[i].queries_number + 1) * sizeof(struct queries));
                      if (lines[i].listHead == NULL) {
                          printf("Memory Allocation Error.\n");
                      }
                      lines[i].listHead[lines[i].queries_number].ip = strdup(space + 1);
                      lines[i].listHead[lines[i].queries_number].port = num;
                      lines[i].queries_number++;
                      noMatch--;
                    } 
                  } else { // multiple ip
                    if (sortip(space +1, lines[i].ip2) == -1 && sortip(lines[i].ip1, space + 1) == -1 && num <= lines[i].port2 && num >= lines[i].port1) { // in between
                      lines[i].listHead = (struct queries*)realloc(lines[i].listHead, (lines[i].queries_number + 1) * sizeof(struct queries));
                      if (lines[i].listHead == NULL) {
                          printf("Memory Allocation Error.\n");
                      }
                      lines[i].listHead[lines[i].queries_number].ip = strdup(space + 1);
                      lines[i].listHead[lines[i].queries_number].port = num;
                      lines[i].queries_number++;
                      noMatch--;
                    }
                  }
                }
              }
            } else {
                n = sprintf (buffer, "Illegal IP address or port specified\n");
                n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);
                if (n < 0) 
                  error ("ERROR writing to socket\n");
                }
          } else {
              n = sprintf (buffer, "Illegal IP address or port specified\n");
              n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1);            
              if (n < 0) 
                error ("ERROR writing to socket\n");
          }

          if (noMatch == lines_number) {
            n = sprintf (buffer, "Connection rejected\n");
            n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1); 
            if (n < 0) 
              error ("ERROR writing to socket\n");
          } else {
            n = sprintf (buffer, "Connection accepted\n");
            n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1); 
            if (n < 0) 
              error ("ERROR writing to socket\n");
          }
          break;
        default:
          noMatch = lines_number;
          if (tiret != NULL && tiret2 != NULL) {
            *tiret = '\0';
            *tiret2 = '\0';
            if (checkip(space + 1) == 0 && checkip(tiret + 1) == 0 && sscanf(space2 + 1, "%d", &num) == 1 && sscanf(tiret2 + 1, "%d",
&num2) == 1) {
              if (num <= 65535 && num >= 0 && num2 <= 65535 && num2 >= 0) {
                if (sortip(space + 1, tiret + 1) == -1 && num < num2) {
                  for (int i = 0; i < lines_number; i++) {
                    if (strcmp(space + 1, lines[i].ip1) == 0 && strcmp(tiret + 1, lines[i].ip2) == 0 && num == lines[i].port1 && num2 == lines[i].port2) {
                      for (int y = i; y < lines_number - 1; ++y) {
                        lines[y] = lines[y + 1];
                      }
                    
                      free(lines[lines_number - 1].ip1);
                      free(lines[lines_number - 1].ip2);
                      if (lines[lines_number - 1].queries_number != 0) {
                        for (int z = 0; z < lines[lines_number - 1].queries_number; z++) {
                          free(lines[lines_number - 1].listHead[z].ip);
                        }
                      }
                      
                      lines = realloc(lines, (lines_number - 1) * sizeof(struct rules));
                      lines_number--;
                      i--;
                    }
                  }
                } else {
                    n = sprintf (buffer, "Rule invalid\n");
                    n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1); 
                    if (n < 0) 
                      error ("ERROR writing to socket\n");
                }
              } else {
                n = sprintf (buffer, "Rule invalid\n");
                n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1); 
                if (n < 0) 
                  error ("ERROR writing to socket\n");
                }
            } else {
              n = sprintf (buffer, "Rule invalid\n");
              n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1); 
              if (n < 0) 
                error ("ERROR writing to socket\n");
            }
          } else if (tiret == NULL && tiret2 == NULL) {
            if (checkip(space + 1) == 0 && sscanf(space2 + 1, "%d", &num) == 1 && num <= 65535 && num >= 0) { 
              if (num <= 65535 && num >= 0) {       
                for (int i = 0; i < lines_number; i++) {
                  if (strcmp(space + 1, lines[i].ip1) == 0 && num == lines[i].port1 && lines[i].ip2 == NULL) {
                    for (int y = i; y < lines_number - 1; ++y) {
                      lines[y] = lines[y + 1];
                    }
                    if (lines[lines_number - 1].queries_number != 0) {
                      for (int z = 0; z < lines[lines_number - 1].queries_number; z++) {
                        free(lines[lines_number - 1].listHead[z].ip);
                      }
                    }
                    free(lines[lines_number - 1].ip1);
                    free(lines[lines_number - 1].listHead);
                    lines = realloc(lines, (lines_number - 1) * sizeof(struct rules));
                    lines_number--;
                    i--;
                  }
                }
              } else {
              n = sprintf (buffer, "Rule invalid\n");
              n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1); 
              if (n < 0) 
                error ("ERROR writing to socket\n");
              }
            } else {
              n = sprintf (buffer, "Rule invalid\n");
              n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1); 
              if (n < 0) 
                error ("ERROR writing to socket\n");
            }
          } else {
            n = sprintf (buffer, "Rule invalid\n");
            n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1); 
            if (n < 0) 
              error ("ERROR writing to socket\n");
          }

          if (noMatch == lines_number) {
            n = sprintf (buffer, "Rule not found\n");
            n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1); 
            if (n < 0) 
              error ("ERROR writing to socket\n");
          } else {
            n = sprintf (buffer, "Rule deleted\n");
            n = write (threadArgs->newsockfd, buffer, BUFFERLENGTH - 1); 
            if (n < 0) 
              error ("ERROR writing to socket\n");
          }
          break;
      }
  }

  pthread_mutex_unlock (&mut); /* release the lock */
       
  /* these two lines are required for proper thread termination */
  serverThreads[threadArgs->threadIndex].status = THREAD_FINISHED;
  pthread_cond_signal(&threadCond);
  
  close (threadArgs->newsockfd); /* important to avoid memory leak */  
  free (threadArgs);
  pthread_exit (&returnValue);
}

/* finds unused thread info slot; allocates more slots if necessary
   only called by main thread */
int findThreadIndex () {
    int i, tmp;

    for (i = 0; i < noOfThreads; i++) {
	if (serverThreads[i].status == THREAD_AVAILABLE) {
	    serverThreads[i].status = THREAD_IN_USE;
	    return i;
	}
    }

    /* no available thread found; need to allocate more threads */
    pthread_rwlock_wrlock (&threadLock);
    serverThreads = realloc(serverThreads, ((noOfThreads + THREADS_ALLOCATED) * sizeof(struct threadInfo_t)));
    noOfThreads = noOfThreads + THREADS_ALLOCATED;
    pthread_rwlock_unlock (&threadLock);
    if (serverThreads == NULL) {
    	fprintf (stderr, "Memory allocation failed\n");
	exit (1);
    }
    /* initialise thread status */
    for (tmp = i+1; tmp < noOfThreads; tmp++) {
	serverThreads[tmp].status = THREAD_AVAILABLE;
    }
    serverThreads[i].status = THREAD_IN_USE;
    return i;
}

/* waits for threads to finish and releases resources used by the thread management functions. You don't need to modify this function */
void *waitForThreads(void *args) {
    int i, res;
    while (1) {
	pthread_mutex_lock(&threadEndLock);
	pthread_cond_wait(&threadCond, &threadEndLock);
	pthread_mutex_unlock(&threadEndLock);

	pthread_rwlock_rdlock(&threadLock);
	for (i = 0; i < noOfThreads; i++) {
	    if (serverThreads[i].status == THREAD_FINISHED) {
		res = pthread_join (serverThreads[i].pthreadInfo, NULL);
		if (res != 0) {
		    fprintf (stderr, "thread joining failed, exiting\n");
		    exit (1);
		}
		serverThreads[i].status = THREAD_AVAILABLE;
	    }
	}
	pthread_rwlock_unlock(&threadLock);
    }
}

int main(int argc, char *argv[])
{


     socklen_t clilen;
     int sockfd, portno;
     struct sockaddr_in6 serv_addr, cli_addr;
     int result;
     pthread_t waitInfo;
     pthread_attr_t waitAttributes;

     if (argc < 2) {
         fprintf (stderr,"ERROR, no port provided\n");
         exit(1);
     }
     
     /* create socket */
     sockfd = socket (AF_INET6, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero ((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin6_family = AF_INET6;
     serv_addr.sin6_addr = in6addr_any;
     serv_addr.sin6_port = htons (portno);

     /* bind it */
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

     /* ready to accept connections */
     listen (sockfd,5);
     clilen = sizeof (cli_addr);
     
       /* create separate thread for waiting  for other threads to finish */
     if (pthread_attr_init (&waitAttributes)) {
	 fprintf (stderr, "Creating initial thread attributes failed!\n");
	 exit (1);
     }

     result = pthread_create (&waitInfo, &waitAttributes, waitForThreads, NULL);
       if (result != 0) {
	 fprintf (stderr, "Initial Thread creation failed!\n");
	 exit (1);
       }


     /* now wait in an endless loop for connections and process them */
       while(1) {
       
	 struct threadArgs_t *threadArgs; /* must be allocated on the heap to prevent variable going out of scope */
	 int threadIndex;

       threadArgs = malloc(sizeof(struct threadArgs_t));
        if (!threadArgs) {
	 fprintf (stderr, "Memory allocation failed!\n");
	 exit (1);
       }

       /* waiting for connections */
       threadArgs->newsockfd = accept( sockfd, 
			  (struct sockaddr *) &cli_addr, 
			  &clilen);
       if (threadArgs->newsockfd < 0) 
	 error ("ERROR on accept");

       /* create thread for processing of connection */
       threadIndex =findThreadIndex();
       threadArgs->threadIndex = threadIndex;
       if (pthread_attr_init (&(serverThreads[threadIndex].attributes))) {
	 fprintf (stderr, "Creating thread attributes failed!\n");
	 exit (1);
     }
       
       
       result = pthread_create (&(serverThreads[threadIndex].pthreadInfo), &(serverThreads[threadIndex].attributes), processRequest, (void *) threadArgs);
       if (result != 0) {
	 fprintf (stderr, "Thread creation failed!\n");
	 exit (1);
       }

        

       
     }
}





