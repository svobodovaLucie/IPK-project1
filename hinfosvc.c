/* ****************** hinfosvc.c ****************** *
 *        Počítačové komunikace a sítě (IPK)        *
 *            Lucie Svobodová, xsvobo1x             *
 *                 FIT VUT v Brně                   *
 *                   2021/2022                      *
 * ************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>

// CPU stats
enum cpu_info {USER, NICE, SYSTEM, IDLE, IOWAIT, IRQ, SOFTIRQ, STEAL, GUEST, GUEST_NICE};
// server socket descriptor
int socketfd;

/** 
 * Function parses arguments and returns the port number.
 *
 * @return port number specified in argv[1] if successful, 
 *         -1 in the case of invalid arguments
 */
int get_port_num(int argc, char **argv) {
  // check number of arguments
  if (argc != 2) {
    fprintf(stderr, "Usage: ./hinfosvc port_number\n");
    return -1;
  }
  
  // get the port number (argv[1])
  char *endptr;
  unsigned long int port = strtoul(argv[1], &endptr, 10);
  if ((endptr != NULL && endptr[0] != '\0') || port > INT_MAX) {
    fprintf(stderr, "Port number must be unsigned int.\n");
    return -1;
  }
  
  return (int)port;
}


/**
 * Functions loads the hostname info from /proc/sys/kernel/hostname 
 * file and saves it to the message buffer.
 *
 * @return pointer to the message buffer, NULL in the case of allocation error
 */
char *hostname(char *message, unsigned *capacity) {
  // open file with the hostname information
  FILE *fp = fopen("/proc/sys/kernel/hostname", "r");
  if (fp == NULL) {
    fprintf(stderr, "/proc/sys/kernel/hostname did not open successfully.\n");
    return NULL;
  }

  // read and save the hostname to the response
  int c;
  sprintf(message, "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n");
  unsigned i = strlen("HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n");
  
  while ((c = fgetc(fp)) != EOF) {
    if (*capacity <= i) {
      // reallocate the buffer if its capacity is full
      message = realloc(message, *capacity * 2);
      *capacity *= 2;
    }
    message[i++] = c;
  }
  // remove the newline
  if (message[i-1] == '\n') {
    message[i-1] = '\0';
  }
  message[i] = '\0';

  fclose(fp);
  return message;
}


/**
 * Function gets the CPU name info from /proc/cpuinfo
 * file and saves it to the message buffer.
 *
 * @return pointer to the message buffer, NULL in the case of allocation error
 */
char *cpu_name(char *message, unsigned *capacity) {
  // open a process
  FILE *p = popen("cat /proc/cpuinfo | grep \"model name\" | head -n 1 | awk -F \": \" '{print $2}'","r");
  if(p == NULL) {
    fprintf(stderr, "Process did not open successfully.\n");
    return NULL;
  }

  // read and print the cpu name
  int c;
  sprintf(message, "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n");
  unsigned i = strlen("HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n");
  
  while ((c = fgetc(p)) != EOF) {
    if (*capacity <= i) {
      message = realloc(message, *capacity * 2);
      *capacity *= 2;
    }
    message[i++] = c;
  }
  // remove the newline
  if (message[i-1] == '\n') {
    message[i-1] = '\0';
  }
  message[i] = '\0';

  fclose(p);
  return message;
}


/**
 * Function loads stats from /proc/stat file into array.
 *
 * @return 0 if successful, -1 if not
 */
int load_stats(long long unsigned a[10]) {
  FILE *p = popen("cat /proc/stat | head -n 1","r");
  if (p == NULL) {
    fprintf(stderr, "/proc/stat did not open successfully.\n");
    return -1;
  }

  char info[1024];
  if (fgets(info, sizeof(info) - 1, p) == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    pclose(p);
    return -1;
  }
  // save current info
  sscanf(info, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
    &a[USER], &a[NICE], &a[SYSTEM], &a[IDLE], &a[IOWAIT], &a[IRQ], &a[SOFTIRQ], &a[STEAL], &a[GUEST], &a[GUEST_NICE]);

  pclose(p);

  return 0;
}


/**
 * Function computes the CPU load info.
 * The implementation was inspired by:
 * https://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux
 *
 * @return load percentage
 */
double compute_load(long long unsigned f[10], long long unsigned s[10]) {
  long long int prevIdle = f[IDLE] + f[IOWAIT];
  long long int idle = s[IDLE] + s[IOWAIT];

  long long int prevNonIdle = f[USER] + f[NICE] + f[SYSTEM] + f[IRQ] + f[SOFTIRQ] + f[STEAL];
  long long int nonIdle = s[USER] + s[NICE] + s[SYSTEM] + s[IRQ] + s[SOFTIRQ] + s[STEAL];

  long long int prevTotal = prevIdle + prevNonIdle;
  long long int total = idle + nonIdle;

  long long int totald = total - prevTotal;
  long long int idled = idle - prevIdle;

  double cpu_percentage = (totald - idled) / (double)totald;
  return cpu_percentage * 100;
}


/**
 * Function computes the CPU load info and saves it to the message buffer.
 *
 * @return pointer to the message buffer, NULL in the case of allocation error
 */
char *cpu_load(char *message) {
  long long unsigned f[10], s[10];

  if (load_stats(f) == -1) {
    return NULL;
  }

  sleep(1);

  if (load_stats(s) == -1) {
    return NULL;
  }

  double load = compute_load(f, s);

  // read and print the load (initial capacity is always bigger - no need to realloc)
  sprintf(message, "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n%lg%c", load, '%');
  return message;
}


/**
 * Function writes "bad request" response to the message buffer.
 *
 * @return pointer to the message buffer
 */
char *bad_req(char *message) {
  sprintf(message, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain;\r\n\r\nBad Request");
  return message;
}


/**
 * Function closes server socket and exits the program.
 * It is called when SIGINT signal is received.
 */
void release_resources(int sig_num){
  fprintf(stderr, "[signal %d] -> server socket was closed\n", sig_num);
  close(socketfd);
  exit(0); 
}


/**
 * Main function.
 */
int main (int argc, char **argv) {  
  // get the port number
  int port = get_port_num(argc, argv);
  if (port == -1) {
    return -1;
  }

  // server socket
  socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketfd == -1) {
		fprintf(stderr, "Could not create socket");
    return -1;
	}
  int opt = 1;
  setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
  
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = INADDR_ANY;
  if (bind(socketfd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
    fprintf(stderr, "Binding error.\n");
    return -1;
  }
  if (listen(socketfd, 1) == -1) {
    fprintf(stderr, "Listening error.\n");
    return -1;
  }

  // client socket, client message
  struct sockaddr client_addr;
  unsigned int client_size = sizeof(client_addr);
  int client_sockfd;
  char client_message[1000];
  memset(client_message, '\0', sizeof(client_message));

  // server response
  unsigned response_capacity = strlen("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain;\r\n\r\nBad Request");
  char *server_response = calloc(response_capacity, 1);
  if (server_response == NULL) {
    fprintf(stderr, "Couldn't allocate the memory.\n");
    return -1;
  }
  memset(server_response, '\0', response_capacity);

  // SIGINT signal handler
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = release_resources;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  
  /* server is running and sending responses; in the case of any error with the client socket 
     the client socket is closed and the server is waiting for receiving a new client socket */
  while (1) {
    // closing all resources if the SIGINT signal was received
    sigaction(SIGINT, &sigIntHandler, NULL);

    // receive the client socket
    client_sockfd = accept(socketfd, (struct sockaddr*)&client_addr, &client_size);
    if (client_sockfd == -1) {
      fprintf(stderr, "Client socked not received successfully.\n");
      continue;
    }
    if (read(client_sockfd, &client_message, sizeof(client_message)) == -1) {
      fprintf(stderr, "Reading error, client socket closed.\n");
      close(client_sockfd);
      continue;
    }
    
    // query check, send the response
    if (strncmp(client_message, "GET /hostname ", 14) == 0) {
      server_response = hostname(server_response, &response_capacity);
    } else if (strncmp(client_message, "GET /cpu-name ", 14) == 0) {
      server_response = cpu_name(server_response, &response_capacity);
    } else if (strncmp(client_message, "GET /load ", 10) == 0) {
      server_response = cpu_load(server_response);
    } else {
      server_response = bad_req(server_response);
    }
    if (server_response == NULL) {
      close(client_sockfd);
      continue;
    }
    if (send(client_sockfd, server_response, strlen(server_response), 0) == -1){
      fprintf(stderr, "Can't send a response.\n");
      close(client_sockfd);
      continue;
    }

    // close the client socket, clear messages
    memset(client_message, '\0', sizeof(client_message));
    memset(server_response, '\0', response_capacity);
    close(client_sockfd);
  }

  // close server socket 
  close(socketfd);
  return 0;
}
