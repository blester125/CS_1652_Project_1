//University of Pittsburgh
//9-22-15
//Brian Lester, bld20@pitt.edu
//Carmen Condeluci, crc73@pitt.edu
//CS1652 Project 1 - HTTP Clien

/* UNCOMMENT FOR MINET 
 * #include "minet_socket.h"
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <iostream>

#define BUFSIZE 1024

int main(int argc, char * argv[]) {
  int clientSocket;
  struct hostent *host;
  struct sockaddr_in addr;
  int resp_code = 0;
  fd_set set;
  char buf[BUFSIZE]; 
  std::string header;
  std::string response;
  std::size_t position;
  std::size_t position2;
  int connection;
  int sent;
  int read;
  int length;

  char * server_name = NULL;
  int server_port    = -1;
  char * server_path = NULL;
  char * req         = NULL;
  bool ok            = false;
  bool content_length = false;

  /*parse args */
  if (argc != 5) {
    fprintf(stderr, "usage: http_client k|u server port path\n");
    exit(-1);
  }

  server_name = argv[2];
  server_port = atoi(argv[3]);
  server_path = argv[4];

  // need to free at error
  req = (char *)malloc(strlen("GET  HTTP/1.0\r\n\r\n") 
    + strlen(server_path) + 1);  

  /* initialize */
  if (toupper(*(argv[1])) == 'K') { 
  /* UNCOMMENT FOR MINET 
   * minet_init(MINET_KERNEL);
   */
  } else if (toupper(*(argv[1])) == 'U') { 
  /* UNCOMMENT FOR MINET 
   * minet_init(MINET_USER);
   */
  } else {
    fprintf(stderr, "First argument must be k or u\n");
    free(req);
    exit(-1);
  }

  /* make socket */
  clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket < 0) {
    fprintf(stderr, "Error creating the Socket.\n");
    free(req);
    exit(-1);
  }
  /* get host IP address  */
  host = gethostbyname(server_name);
  if (host == NULL) {
    fprintf(stderr, "Error getting host IP.\n");
    free(req);
    close(clientSocket);
    exit(-1);
  }

  /* set address */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(server_port);
  memcpy(&addr.sin_addr.s_addr, host->h_addr, host->h_length);

  /* connect to the server socket */
  connection = connect(clientSocket, (struct sockaddr *)&addr, sizeof(addr));
  if (connect < 0) {
    fprintf(stdout, "Error connecting to Socket.\n");
    free(req);
    close(clientSocket);
    exit(-1);
  }

  /* send request message */
  char server_path_slash[strlen(server_path) + 1];
  if (server_path[0] != '/') {
    strcpy(server_path_slash, "/");
    strcat(server_path_slash, server_path);
  } else {
    strcpy(server_path_slash, server_path);
  } 
  sprintf(req, "GET %s HTTP/1.0\r\n\r\n", server_path_slash);
  sent = send(clientSocket, req, strlen(req), 0);
  if (sent <= 0) {
    fprintf(stdout, "Error sending request.\n");
    free(req);
    close(clientSocket);
    exit(-1);
  }

  /* wait till socket can be read. */
  FD_ZERO(&set);
  FD_SET(clientSocket, &set);
  select(clientSocket+1, &set, NULL, NULL, NULL);
  
  /* first read loop -- read headers */
  read = recv(clientSocket, &buf, BUFSIZE - 1, 0);
  if (read <= 0) {
    fprintf(stderr, "Error reading from socket.\n");
    free(req);
    close(clientSocket);
    exit(-1);
  }
  buf[read] = '\0';
  while (read > 0) {
    response += std::string(buf);
    position = response.find("\r\n\r\n", 0);
    if (position != std::string::npos) {
      header += response.substr(0,position);
      response = response.substr(position+4);
      break;
    } else {
      header += response;
    }
    read = recv(clientSocket, &buf, BUFSIZE - 1, 0);
    buf[read] = '\0';
  }

  /* examine return code */  
  position = header.find(" ");
  position2 = header.find(" ", position);
  resp_code = std::stoi(header.substr(position, position2));
  
  // Normal reply has return code 200
  if (resp_code == 200) {
    ok = true;
  }

  length = 0;
  position = header.find("Content-length: ");
  if (position == std::string::npos) {
    position = header.find("Content-Length: ");
  }
  if (position != std::string::npos) {
    position2 = header.find("\n", position);
    //16 is the length of the string "Content-length: "
    length = std::stoi(header.substr(position + 16, position2));
  }
  
  /* print first part of response: header, error code, etc. */
  std::cout << header << std::endl;

  /* second read loop -- print out the rest of the response: real web content */
  if (length > 0) {
    // Read until you get the whole thing
    while (response.length() < length) {
      read = recv(clientSocket, &buf, BUFSIZE - 1, 0);
      buf[read] = '\0';
      response += std::string(buf);
    }
  } else {
    // Read until the connection is closed
    read = 1;
    while (read > 0) {
      select(clientSocket+1, &set, NULL, NULL, NULL);
      read = recv(clientSocket, &buf, BUFSIZE - 1, 0);
      buf[read] = '\0';
      response += std::string(buf);
    }
  }
  
  // print response
  std::cout << response << std::endl;
  /*close socket and deinitialize */
  close(clientSocket);
  free(req);
  if (ok) {
    return 0;
  } else {
    return -1;
  }
}
