/* ****************** hinfosvc.c ****************** *
 *        Počítačové komunikace a sítě (IPK)        *
 *            Lucie Svobodová, xsvobo1x             *
 *                 FIT VUT v Brně                   *
 *                   2021/2022                      *
 * ************************************************ */

/*
1) OK vytvořte si funkci, která vám vrátí aktuální využití procesoru
2) OK zjistete si hostname (/proc/sys/kernel/hostname)
3) OK zjistete si cpu name (popen("cat /proc/cpuinfo | grep "model name" | head -n 1 | awk '...)
4) OK socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); - vytvoří socket
5) OK setsockopt na 1 pro SO_REUSEADDR a SO_REUSEPORT - zajistí že nebude záhadně padat při testování
6) OK struct sockaddr_in address;
      address.sin_family = AF_INET;
      address.sin_port = htons(tvujPort);
      address.sin_addr.s_addr = INADDR_ANY;
7) OK bind(socket, address) - nastaví socket
8) OK listen() - socket začne přijímat spojení
9) while true do accept() - pak můžete se socketem pracovat jako se souborem
10) pokud spojení začíná na "GET /hostname" (strncmp) nebo jiný z těch tří endpointů...
11) tak do socketu zapiš "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n" a tvou odpověď, bez nového řádku na konci
12) Jinak zapiš odpověď se stavem "400 Bad Request"
13) Pokud tak ještě neučinils, vyčti zbytek dat v socketu ať si třeba prohlížeč nestěžuje
14) close()  end while
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

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
 * Function gets the CPU name info from /proc/cpuinfo
 * file and prints it to the stdout.
 *
 * @return 0 if successful, -1 if not
 */
int cpu_name() {
  // open a process
  FILE *p = popen("cat /proc/cpuinfo | grep \"model name\" | head -n 1 | awk -F \": \" '{print $2}'","r");
  if(p == NULL) {
    fprintf(stderr, "Process did not open successfully.\n");
    return -1;
  }

  // read and print the cpu name
  int c;
  while ((c = fgetc(p)) != EOF) {
    fputc(c, stdout);
  }

  pclose(p);
  return 0;
}


/**
 * Functions loads the hostname info from /proc/sys/kernel/hostname 
 * file and prints it to the stdout.
 *
 * @return 0 if successful, -1 if the file couldn't be open
 */
int hostname() {
  // open file with the hostname information
  FILE *fp = fopen("/proc/sys/kernel/hostname", "r");
  if (fp == NULL) {
    fprintf(stderr, "/proc/sys/kernel/hostname did not open successfully.\n");
    return -1;
  }

  // read and print the hostname to stdout
  int c;
  while ((c = fgetc(fp)) != EOF) {
    fputc(c, stdout);
  }

  fclose(fp);
  return 0;
}

enum cpu_info {USER, NICE, SYSTEM, IDLE, IOWAIT, IRQ, SOFTIRQ, STEAL, GUEST, GUEST_NICE};

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
int compute_load(long long unsigned f[10], long long unsigned s[10]) {
  printf("Inside compute_load(), f: %llu, s: %llu\n", f[USER], s[USER]);

  long long int prevIdle = f[IDLE] + f[IOWAIT];
  long long int idle = s[IDLE] + s[IOWAIT];

  long long int prevNonIdle = f[USER] + f[NICE] + f[SYSTEM] + f[IRQ] + f[SOFTIRQ] + f[STEAL];
  long long int nonIdle = s[USER] + s[NICE] + s[SYSTEM] + s[IRQ] + s[SOFTIRQ] + s[STEAL];

  long long int prevTotal = prevIdle + prevNonIdle;
  long long int total = idle + nonIdle;

  long long int totald = total - prevTotal;
  long long int idled = idle - prevIdle;

  double cpu_percentage = (totald - idled) / (double)totald;

  // round and convert to percentage
  // TODO not sure with the rounding and * 100
  return ((int)(cpu_percentage + 0.5)) * 100;
}


/**
 * Function computes the CPU load info
 * and prints it to the stdout.
 *
 * @return 0 if successful, -1 if not
 */
int cpu_load() {
  long long unsigned f[10], s[10];

  load_stats(f);
  sleep(1);
  load_stats(s);

  int load = compute_load(f, s);

  printf("Done cpu_load(), f: %llu, s: %llu, load: %d\n", f[USER], s[USER], load);
  return 0;
}


// Decide which query is asked


int main(int argc, char **argv) {
  // get the port number
  int port = get_port_num(argc, argv);
  if (port == -1) {
    return -1;
  }

  fprintf(stderr, "Arg num = %d, port number = %u\n", argc, port);

  hostname();
  
  cpu_name();

  cpu_load();

  // TODO Jindra mel misto 0 IPPROTO_TCP
  int socketfd = socket(AF_INET, SOCK_STREAM, 0);
  // setsockopt na 1 pro SO_REUSEADDR a SO_REUSEPORT - zajistí že nebude záhadně padat při testování
  int opt = 1;
  setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = INADDR_ANY;

  bind(socketfd, (struct sockaddr*)&address, sizeof(address));

  listen(socketfd, 1);

  /*
9) while true do accept() - pak můžete se socketem pracovat jako se souborem
10) pokud spojení začíná na "GET /hostname" (strncmp) nebo jiný z těch tří endpointů...
11) tak do socketu zapiš "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n" a tvou odpověď, bez nového řádku na konci
12) Jinak zapiš odpověď se stavem "400 Bad Request"
13) Pokud tak ještě neučinils, vyčti zbytek dat v socketu ať si třeba prohlížeč nestěžuje
14) close()  end while
  */

  char server_message[2000], client_message[2000];
    
  // Clean buffers:
  memset(server_message, '\0', sizeof(server_message));
  memset(client_message, '\0', sizeof(client_message));

  struct sockaddr client_addr;
  unsigned int client_size = sizeof(client_addr);
  int client_sock;

  while (1) {
    
    /*
    client_sock = accept(socketfd, (struct sockaddr*)&client_addr, &client_size);

    //recv(client_sock, client_message, sizeof(client_message), 0);
    
    if (client_sock < 0){
        printf("Can't accept\n");
        return -1;
    }
    //printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    // Receive client's message:
    if (recv(client_sock, client_message, sizeof(client_message), 0) < 0){
        printf("Couldn't receive\n");
        return -1;
    }
    printf("Msg from client: %s\n", client_message);
    
    // Respond to client:
    strcpy(server_message, "This is the server's message.");
    
    if (send(client_sock, server_message, strlen(server_message), 0) < 0){
        printf("Can't send\n");
        return -1;
    }

    //send(client_sock, server_message, strlen(server_message), 0);

    printf("Client: %s", client_message);
    //printf("Server: %s", server_message);
    close(client_sock);
    */

  }

  close(socketfd);

  return 0;
}