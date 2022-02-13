/* ****************** hinfosvc.c ****************** *
 *        Počítačové komunikace a sítě (IPK)        *
 *            Lucie Svobodová, xsvobo1x             *
 *                 FIT VUT v Brně                   *
 *                   2021/2022                      *
 * ************************************************ */

/*
1) vytvořte si funkci, která vám vrátí aktuální využití procesoru
2) zjistete si hostname (/proc/sys/kernel/hostname)
3) zjistete si cpu name (popen("cat /proc/cpuinfo | grep "model name" | head -n 1 | awk '...)
4) socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); - vytvoří socket
5) setsockopt na 1 pro SO_REUSEADDR a SO_REUSEPORT - zajistí že nebude záhadně padat při testování
6) struct sockaddr_in address;
   address.sin_family = AF_INET;
   address.sin_port = htons(tvujPort);
   address.sin_addr.s_addr = INADDR_ANY;
7) bind(socket, address) - nastaví socket
8) listen() - socket začne přijímat spojení
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


/**
 * Function computes the CPU load info
 * and prints it to the stdout.
 *
 * @return 0 if successful, -1 if not
 */
int cpu_load() {

  return -1;
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

  return 0;
}