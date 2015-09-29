//University of Pittsburgh
//9-22-15
//Brian Lester, bld20@pitt.edu
//Carmen Condeluci, crc73@pitt.edu
//CS1652 Project 1 - HTTP Server1

/* UNCOMMENT FOR MINET 
 * #include "minet_socket.h"
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <unistd.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100
#define MAXCONNECTIONS 32

int handle_connection(int sock);
int parse_file(const char * request, char * filename, int len);

int main(int argc, char * argv[]) {
  struct sockaddr_in saddr;
  int listenSocket;
  int servSocket;
  int binding;
  int listening;
  int sending;

  int server_port = -1;
  int rc          =  0;
  int sock        = -1;

  /* parse command line args */
  if (argc != 3) {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
  }

  server_port = atoi(argv[2]);

  if (server_port < 1500) {
    fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", server_port);
    exit(-1);
  }

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
    exit(-1);
  }
  /* initialize and make socket */
  listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listenSocket < 0) {
    fprintf(stderr, "Error creating the socket.\n");
    exit(-1);
  }
  /* set server address*/
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(server_port);
  /* bind listening socket */
  binding = bind(listenSocket, (struct sockaddr *)&saddr, sizeof(saddr));
  if (binding < 0) {
    fprintf(stderr, "Error binding the socket.\n");
    exit(-1);
  }
  /* start listening */
  listening = listen(listenSocket, MAXCONNECTIONS);
  if (listening < 0) {
    fprintf(stderr, "Error listening.\n");
    exit(-1);
  }
  /* connection handling loop: wait to accept connection */
  while ((servSocket = accept(listenSocket, NULL, NULL)) >= 0) {
    /* handle connections */
    rc = handle_connection(servSocket);
    if (rc < 0) {
      fprintf(stderr, "Error handling connection\n");
    }
    close(servSocket);
  }
  close(listenSocket);
  return 0;
}

int handle_connection(int sock) {
  bool ok = true;
  char recvbuf[BUFSIZE];
  int bytes_read;
  int fetch;
  int sending;
  std::string req;
  char filename[FILENAMESIZE];
  char buf[BUFSIZE];
  std::string file_string;
  std::size_t pos;
  
  const char * ok_response_f = "HTTP/1.0 200 OK\r\n"	\
    "Content-Type: text/plain\r\n"			\
    "Content-Length: %d \r\n\r\n";
  char ok_response[strlen(ok_response_f)];

  const char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"	\
    "Content-Type: text/html\r\n\r\n"			\
    "<html><body bgColor=black text=white>\n"		\
    "<h2>404 FILE NOT FOUND</h2>\n" \
    "</body></html>\n";
  /* first read loop -- get request and headers*/
  // Read the Headers.
  bytes_read = recv(sock, recvbuf, BUFSIZE - 1, 0);
  if (bytes_read <= 0) {
    fprintf(stderr, "Error reading request.\n");
    return -1;
  }
  recvbuf[bytes_read] = '\0';
  // Keep reading until "\r\n\r\n" is found.
  while (bytes_read > 0) {
    req += std::string(recvbuf);
    pos = req.find("\r\n\r\n", 0);
    if (pos != std::string::npos) {
    	req = req.substr(0, pos);
    	break;
    }
    bytes_read = recv(sock, recvbuf, BUFSIZE - 1, 0);
    recvbuf[bytes_read] = '\0';
  }
  /* parse request to get file name */
  /* Assumption: this is a GET request and filename contains no spaces*/
  fetch = parse_file(req.c_str(), filename, FILENAMESIZE);
  if (fetch < 0) {
    fprintf(stderr, "Error finding filename.\n");
    ok = false;
  }
  if (!strcmp(filename, "")) {
    strcpy(filename, "index.html");
  }
  /* try opening the file */
  FILE * reqfile = fopen(filename, "rb");
  if (reqfile == NULL) {
    fprintf(stderr, "Error opening file %s.\n", filename);
    ok = false;
  }
  // If no error has occured read the file.
  // Not called if the file is not found.
  if (ok) {
    while (bytes_read = fread(&buf, 1, BUFSIZE - 1, reqfile)) {
      if (bytes_read <= 0) {
        break;
      }
      buf[bytes_read] = '\0';
      file_string += std::string(buf);
    }
  }
  /* send response */
  if (ok) {
    /* send headers */ 
    // Create response with the content length.
    sprintf(ok_response, ok_response_f, file_string.size());
    // Add the response and thcontent into a string
    file_string = std::string(ok_response) + file_string;
    // Send the string over the socket
    sending = send(sock, file_string.c_str(), file_string.size(), 0);
    if (sending <= 0) {
      fprintf(stderr, "Error sending file");
      ok = false;
    }
  } else {
    //send error response
    sending = send(sock, notok_response, strlen(notok_response), 0);
    if (sending <= 0) {
      fprintf(stderr, "Error sending the not OK response.\n");
      ok = false;
    }
  }
  /* close socket and free space */
  if (ok) {
    return 0;
  } else {
    return -1;
  }
}

int parse_file(const char * request, char * filename, int len) {
  int req_len = strlen(request);
  char * temp = new char[req_len + 1];
  char * temp2 = new char[req_len + 1];
  strcpy(temp, request);
  char * pos = strstr(temp, " HTTP/1.");
  if (!pos || (strncmp(temp, "GET ", 4) != 0)) {
    delete [] temp;
    delete [] temp2;
    return -1;
  }
  *pos = '\0';
  if (temp [4] == '/') {
    strcpy(temp2, &temp[5]);
  } else {
    strcpy(temp2, &temp[4]);
  }
  if ((int)strlen(temp2) + 1 > len) {
    delete [] temp;
    delete [] temp2;
    return -1;
  }
  strcpy(filename, temp2);
  delete [] temp;
  delete [] temp2;
  return 0;
}