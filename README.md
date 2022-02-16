# IPK project 1 - HTTP server

This project implements a lightweight server for Linux that uses HTTP protocol for the communication. The server provides information about the system. It sends HTTP responses for these three queries (that are sent to the server by GET command):
- hostname (`/hostname`), 
- cpu-name (`/cpu-name`),
- current cpu load (`/load`).


## Installation

1. Clone this repository
```
$ git clone https://github.com/svobodovaLucie/IPK-project1.git
```
2. Build the project with make
```
$ cd IPK-project1 && make
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
