#CS_1652_Project_1

The first project for CS 1652 at the University of Pittsburgh. By Brian Lester and Carmen Condeluci.
This is the implementation using standard unix system calls. For the version using minet functions 
please click [here](https://github.com/blester125/CS_1652_Project_1_Minet)

###The http_client
## 
Code that can fetch content from a server. It only creates GET requests. It is run with

`./http_client k serveraddress serverport /path`

###http_server1
##
A simple web server that can serve a page in the current working directory. It can only handle 
incoming GET requests. It only handles one connection at a time.

###http_server2
## 
A more complex web server that uses select to wait for connections to be ready so it can handle 
multiple connections at a time. However it still can block on reads or writes.

###http_server3
## 
Using nonblocking I/O this webserver handles multiple connections well.