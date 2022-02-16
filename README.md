# IPK project 1 - HTTP server

Author:  Lucie Svobodová, xsvobo1x@stud.fit.vutbr.cz  
Faculty: Faculty of Information Technology, BUT  
Course:  Computer Communications and Networks (IPK), 2021/2022  

This project implements a lightweight server for Linux that uses HTTP protocol for the communication. The server provides information about the system. It sends HTTP responses for these three queries (that are sent to the server by GET command):
- hostname (`/hostname`) 
- cpu-name (`/cpu-name`)
- current cpu load (`/load`)

Error messages are printed to the standard error output (stderr).  

## Build

Build the project with `make`
```
$ make
```


## Usage
Now you can run the server:
```
$ ./hinfosvc 12345
```
where `12345` is your port number. 

Then you can communicate with the server via web browser or `wget` and `curl` tools. Here are some examples how the program can be run:
```
$ ./hinfosvc 2342 &
$ curl http://localhost:2342/hostname
$ curl http://localhost:2342/cpu-name
$ wget http://localhost:2342/load
```
