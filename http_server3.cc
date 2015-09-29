//University of Pittsburgh
//9-22-15
//Brian Lester, bld20@pitt.edu
//Carmen Condeluci, crc73@pitt.edu
//CS1652 Project 1 - HTTP Server3

/* UNCOMMENT FOR MINET 
 * #include "minet_socket.h"
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <vector>

#define FILENAMESIZE 100
#define BUFSIZE 1024
#define MAXCONNECTIONS 32

// What the connection is currently doing
typedef enum { NEW,
	       READING_HEADERS,
	       WRITING_RESPONSE,
	       READING_FILE,
	       WRITING_FILE,
	       CLOSED } states;

typedef struct connection_s connection;

struct connection_s {
    // File descriptor of the socket 
    int sock;
    // File descriptor of the file
    int fd;
    // Name of the file
    char filename[FILENAMESIZE + 1];
    // Buffer (to be sent)
    char buf[BUFSIZE + 1];
    // The length of whats in the buffer
    int buflen = 0;
    char * endheaders;
    // The status of the response
    bool ok = true;
    // The length of the stile
    long filelen;
    // The state of the connection
    states state;

    // How many bytes of the headers have been read
    int headers_read;
    // How many bytes of the response have been writtem
    int response_written;
    // How many bytes of the file have been written
    int file_read;
    // How much of the file has been written to the socket.
    int file_written;

    connection * next;
};

void read_headers(connection * con);
void write_response(connection * con);
void read_file(connection * con);
void write_file(connection * con);
int parse_file(const char * request, char * filename, int len);

// The max file descriptor number
int max = 0;

int main(int argc, char * argv[]) {
  int server_port = -1;

  /* parse command line args */
  if (argc != 3) {
	fprintf(stderr, "usage: http_server3 k|u port\n");
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
  // Step 1.
  /* initialize and make socket */
  int acceptSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (acceptSocket < 0) {
  	fprintf(stderr, "Error creating accept socket.\n");
  	exit(-1);
  }
  // Step 2.
  /* set server address*/
  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(server_port);
  //step 2.
  /* bind listening socket */
  int bind_error = bind(acceptSocket, (struct sockaddr *)&saddr, sizeof(saddr));
  if (bind_error < 0) {
  	fprintf(stderr, "Error binding socket.\n");
  	exit(-1);
  }
  // Step 3/
  /* start listening */
  int listening = listen(acceptSocket, MAXCONNECTIONS);
  if (listening < 0) {
  	fprintf(stderr, "Error listening.\n");
  	exit(-1);
  }
  // Step 4.
  max = acceptSocket;
  // List of all connections
  std::vector<connection> full;
  fd_set readList;
  fd_set writeList;
  // Step 5.
  while (1) {
	  /* create read and write lists */
    FD_ZERO(&readList);
    FD_ZERO(&writeList);
    std::vector<connection>::iterator it;
    // iterate through the list of all connections
	  for (it = full.begin(); it != full.end(); it++) {
		  // Step e.
		  if(it->state == NEW) {
		  	it->state = READING_HEADERS;
		  }
      if (it->state == READING_HEADERS) {
    	  FD_SET(it->sock, &readList);
      }
      if (it->state == READING_FILE) {
    	  FD_SET(it->fd, &readList);
      }
      // Step g.
      if (it->state == WRITING_RESPONSE) {
    	  FD_SET(it->sock, &writeList);
      }
      if (it->state == WRITING_FILE) {
    	  FD_SET(it->fd, &writeList);
      }
	  }
	  // Step f.
	  FD_SET(acceptSocket, &readList);
	  // Step h.
	  /* do a select */
	  int result = select(max+1, &readList, &writeList, NULL, NULL);
	  if (result > 0) {
		  // Step i.
      /* process sockets that are ready to be read */
      // loop through all possible file dexcriptors
      for (int i = 0; i < max+1; i++) {
      	// if the file is in the read list and ready to be read
    	  if (FD_ISSET(i, &readList)) {
    		  // Step i (i).
    		  // If it is the accept Socket
          if (i == acceptSocket) {
          	// Make the new socket
        	  int clientSocket = accept(acceptSocket, NULL, NULL);
        	  if (clientSocket < 0) {
        		  fprintf(stderr, "Error accepting socket.\n");
        		  continue;
        	  } else {
              fcntl(clientSocket, F_SETFL, O_NONBLOCK);
        		  if (clientSocket > max) {
        			  max = clientSocket;
        		  }
        		  // Make a new connection
              connection *conn = new connection();
        		  conn->sock = clientSocket;
        		  conn->state = NEW;
        		  full.push_back(*conn);
        	  }
          }
          else {
          	// Loop through the fd's to see if i is a socket or file
            for (it = full.begin(); it != full.end(); it++) {
          	  // if it is a Socket.
          	  if (i == it->sock) {
                // Recv from socket
                read_headers(&(*it));
          	  }
          	  // If it is a file.
          	  if (i == it->fd) {
                // Read from file.
                read_file(&(*it));
          	  }
            }
          }
    	  }
      }
      /* process sockets that are ready to write */
      for (int i = 0; i < max; i++) {
    	  if (FD_ISSET(i, &writeList)) {
          for (it = full.begin(); it != full.end(); it++) {
        	  if (i == it->sock) {
              // Send on socket
              write_response(&(*it));
        	  }
          }
    	  }
      }
	  }
	  // remove closed sockets
	  for (it = full.begin(); it != full.end();) {
	  	if (it->state == CLOSED) {
	  		it = full.erase(it);
	  	} else {
	  		++it;
	  	}
	  }
  }
}

void read_headers(connection * con) {
	con->state = READING_HEADERS;
  /* first read loop -- get request and headers*/
  int bytes_read = 0;
  // While you read something from the socket and you haven't filled the buffer
  do {
    // Add to amount of bytes read to headers_read
  	con->headers_read += bytes_read;
  	// Add \0 to help prosses string
  	con->buf[con->headers_read] = '\0';
  	// Look for '\r\n\r\n' that ends http headers
  	if(con->headers_read >= 4 
  		 && strcmp(con->buf + con->headers_read-4, "\r\n\r\n") == 0) {
  		break;
  	}
  	// Read from socket. writing to the end of the buffer buf[headers_read]
  	// Read the amount of buffer left
    bytes_read = recv(con->sock, con->buf + con->headers_read, 
    	                BUFSIZE - con->headers_read, 0);
    // If nothing is read we can close the connection
    if (bytes_read == 0) {
    	con->state = CLOSED;
    	return;
    }
    // If there is a EAGAIN try later
    else if (bytes_read == -1 && errno == EAGAIN) {
    	con->state = READING_HEADERS;
  	  return;
  	}
  	else if (bytes_read < 0) {
  		fprintf(stderr, "Error reading headers.");
  		con->state = CLOSED;
  		return;
  	}
  } while (bytes_read > 0 && con->headers_read < BUFSIZE);
  /* parse request to get file name */
  /* Assumption: this is a GET request and filename contains no spaces*/
  int fetch = parse_file(con->buf, con->filename, FILENAMESIZE);
  if (fetch < 0) {
  	fprintf(stderr, "Error parseing request.\n");
  	con->ok = false;
  }
  /* get file name and size, set to non-blocking */
  // If there is no file name it was '/' so server the index page
  if (!strcmp(con->filename, "")) {
  	strcpy(con->filename, "index.html");
  }
  // open the file
  con->fd = open(con->filename, O_RDONLY);
  if (con->fd < 0) {
  	fprintf(stderr, "Error opening file.\n");
  	con->ok = false;
  } 
  else {
  	con->ok = true;
  	// set to non clocking
  	fcntl(con->fd, F_SETFL, O_NONBLOCK);
  	struct stat filestats;
  	// get stats on the file
  	fstat(con->fd, &filestats);
  	// get the file size
  	con->filelen = filestats.st_size;
  	// update the maximum fd
  	if (con->fd > max) {
    	max = con->fd;
    }
  }
  write_response(con);
}

void write_response(connection * con) {
	con->state = WRITING_RESPONSE;
  const char * ok_response_f = "HTTP/1.0 200 OK\r\n"	\
    "Content-Type: text/plain\r\n"			\
    "Content-Length: %d \r\n\r\n";
    
  const char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"	\
    "Content-Type: text/html\r\n\r\n"			\
    "<html><body bgColor=black text=white>\n"		\
    "<h2>404 FILE NOT FOUND</h2>\n"				\
    "</body></html>\n";
  const int notok_response_len = strlen(notok_response);

  /* send response */
  if (con->ok) {
  	// Send OK RESPONSE 
  	// If tehre is nothing in the buffer this is the first time the connection 
  	// has made it this far and we need to send headers.
  	if (con->buflen == 0) {
  	  sprintf(con->buf, ok_response_f, con->filelen);
  	  con->buflen = strlen(con->buf);
  	}
  	// If you haen't written the whole buffer
  	while(con->response_written < con->buflen) {
  		// Send starting from the last spot written in the buffer
  		// Send the amount left
  	  int size = send(con->sock, con->buf + con->response_written,
  	                con->buflen - con->response_written, 0);
  	  // If you don't send anything close
  	  if (size == 0) {
  	  	con->state = CLOSED;
  	  	return;
  	  }
  	  // If there is EAGAIN try again
  	  else if (size == -1 && errno == EAGAIN) {
  	  	con->state = WRITING_RESPONSE;
  	  	return;
  	  }
  	  else if (size < 0) {
  	  	fprintf(stderr, "Error sending OK response.\n");
  	  	con->state = CLOSED;
  	  }
  	  // add the amount written to the total written
  	  con->response_written += size;
  	}
  	// clear the buffer
  	con->buflen = 0;
  	con->buf[0] = '\0';
  	// start to read in file
  	read_file(con);
  } else {
  	// Send not OK Response
  	while(con->response_written < notok_response_len) {
  		// Send starting from the last spot used in the buffer
  		// Send the amount left in the buffer
      int size = send(con->sock, notok_response + con->response_written, 
    	                    notok_response_len - con->response_written, 0);
      if (size == 0) {
      	con->state = CLOSED;
      	return;
      }
      else if (size == -1 && errno == EAGAIN) {
      	con->state = WRITING_RESPONSE;
      	return;
      }
      else if (size < 0) {
      	fprintf(stderr, "Error sending not OK response.\n");
      	con->state = CLOSED;
      	return;
      }
      // Up the increased size
      con->response_written += size;
    }
    con->state = CLOSED;
  }
}

void read_file(connection * con) {
  con->state = READING_FILE;
  // while you haven't filled the buffer
  while (con->buflen < BUFSIZE) {
  	// Read into the last end of the buffer
  	// Read the amount left in the buffer
  	int bytes_read = read(con->fd, con->buf + con->buflen, 
  		                    BUFSIZE - con->buflen);
    if (bytes_read == 0) {
    	break;
    }
    else if (bytes_read < 0) {
    	if (errno == EAGAIN) {
    		return;
    	}
    	fprintf(stderr, "Error Reading file.");
    	break;
    }
    // updae connection information
    con->buflen += bytes_read;
    con->buf[con->buflen] = '\0';
    con->file_read += bytes_read;
  }
  if (con->file_read == con->filelen) {
  	// Whole file Read
  	close(con->fd);
  	write_file(con);
  }
  else if(con->buflen == BUFSIZE) {
  	// Buffer filled
  	write_file(con);
  }
}

void write_file(connection * con) {
  con->state = WRITING_FILE;
  // There is stuff in the buffer
  while (con->buflen) {
  	// write what was in the buffer
  	// % used to get the spot in the buffer based on amount written without
  	// going over the buffer
    int size = send(con->sock, con->buf + (con->file_written % BUFSIZE),
    	              con->buflen, 0);
    if (size <= 0) {
    	if (errno == EAGAIN) {
    		con->state = WRITING_FILE;
    		return;
    	}
    	fprintf(stderr, "Sending File Fail.\n");
    	break;
    }
    // Update connection info
    con->buflen -= size;
    con->file_written += size;
  }
  if (con->file_written == con->filelen) {
  	// Whole File Written.
  	con->state = CLOSED;
  }
  else if(con->buflen == 0) {
  	//Whole buffer written.
  	read_file(con);
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
