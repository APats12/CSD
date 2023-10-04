#include "asgn2_helper_funcs.h"

#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFF_SIZE 8192

#define METHOD       "([a-zA-Z]{1,8}) "
#define URI          "/([a-zA-Z0-9.-]{1,63}) "
#define VERSION      "(HTTP/[0-9]\\.[0-9])\r\n"
#define FULL_REQUEST "^" METHOD URI VERSION
#define HEADER       "([a-zA-Z0-9.-]{1,128}): ([ -~]{1,128})\r\n"

typedef struct Requests {
    int inputFile; // File descriptor of the client's input file
    int msgSize; // Size of the message body (if any) in the client's request
    int bytesLeft; // Number of bytes left to read in the message body
    char *get_put; // Pointer to the HTTP method (GET or PUT) extracted from the client's request
    char *path; // Pointer to the target path extracted from the client's request
    char *httpVersion; // Pointer to the HTTP version extracted from the client's request
    char *msg; // Pointer to the message body (if any) in the client's request
} Requests;

// helper function to handle different status-codes
void handle_error(int status_code, int fd) {
    switch (status_code) {
    case 400: // Status-Code 400: When a request is ill-formatted.
        dprintf(fd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        break;
    case 403: // Status-Code 403: When the server cannot access the URI's files.
        dprintf(fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
        break;
    case 404: // Status-Code 404: When the URI's file does not exist.
        dprintf(fd, "HTTP/1.1 404 Not Found\r\nContent-Length: %d\r\n\r\nNot Found\n", 10);
        break;
    case 500: // Status-Code 500: When an unexpected issue prevents processing.
        dprintf(fd,
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
            "Error\n",
            22);
        break;
    case 501: // Status-Code 501: When a request includes an uninmplemented Method.
        dprintf(
            fd, "HTTP/1.1 501 Not Implemented\r\nContent-Length: %d\r\n\r\nNot Implemented\n", 16);
        break;
    case 505: // Status-Code 505: When a request includes an unsupported version.
        dprintf(fd,
            "HTTP/1.1 505 Version Not Supported\r\nContent-Length: %d\r\n\r\nVersion Not "
            "Supported\n",
            22);
        break;
    default: break;
    }
}

// helper function to check if directory
int isDirectory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        return 0; // failed to stat file, assume not a directory
    }
    return S_ISDIR(statbuf.st_mode);
}

// helper function to extract request line components
void extractRequestLine(Requests *requestObj, char *buff, regmatch_t match[]) {

    // Extract the GET/PUT method from the buffer and store it in the requests struct
    requestObj->get_put = strndup(buff, match[1].rm_eo);

    // Extract the target path from the buffer and store it in the requests struct
    requestObj->path = strndup(buff + match[2].rm_so, match[2].rm_eo - match[2].rm_so);

    // Extract the HTTP version from the buffer and store it in the requests struct
    requestObj->httpVersion = strndup(buff + match[3].rm_so, match[3].rm_eo - match[3].rm_so);
}

// helper function to extract headers
void extractHeader(Requests *requestObj, char *buff, regmatch_t match[]) {

    // extract header name and value
    char *headerName = strndup(buff + match[1].rm_so, match[1].rm_eo - match[1].rm_so);
    char *headerValue = strndup(buff + match[2].rm_so, match[2].rm_eo - match[2].rm_so);

    // check if header is "Content-Length" and update value
    if (strncmp(headerName, "Content-Length", 14) == 0) {
        int val = strtol(headerValue, NULL, 10);
        if (errno == EINVAL) {
            handle_error(400, requestObj->inputFile);
        }
        requestObj->msgSize = val;
    }

    // free allocated memory
    free(headerName);
    free(headerValue);
}

int parseRequest(Requests *requestObj, char *buff, ssize_t bytes_read) {

    regex_t regex;
    regmatch_t match[4];

    int rc;
    // compile and execute the regex to match the request line
    rc = regcomp(&regex, FULL_REQUEST, REG_EXTENDED);
    rc = regexec(&regex, buff, 4, match, 0);

    int offset = 0;
    if (rc == 0) {
        // extract request line components
        extractRequestLine(requestObj, buff, match);
        buff += match[3].rm_eo + 2; // move buffer pointer past CRLF after request line
        offset += match[3].rm_eo + 2; // update total offset
    } else {
        // handle invalid request line
        handle_error(400, requestObj->inputFile);
        regfree(&regex);
        return (1);
    }

    // reset regex and match headers
    requestObj->msgSize = -1; // reset content length
    rc = regcomp(&regex, HEADER, REG_EXTENDED);
    rc = regexec(&regex, buff, 3, match, 0);

    while (rc == 0) {
        // extract header fields
        extractHeader(requestObj, buff, match);
        buff += match[2].rm_eo + 2; // move buffer pointer past header and CRLF
        offset += match[2].rm_eo + 2; // update total offset
        rc = regexec(&regex, buff, 3, match, 0);
    }

    // check if there's a message
    if ((rc != 0) && (buff[0] == '\r' && buff[1] == '\n')) {
        requestObj->msg = buff + 2; // set msg to beginning of message
        offset += 2; // update total offset
        requestObj->bytesLeft = bytes_read - offset; // calculate the bytes left
    } else if (rc != 0) {
        // handle invalid header fields
        handle_error(400, requestObj->inputFile);
        regfree(&regex);
        return (1);
    }

    regfree(&regex);
    return (EXIT_SUCCESS);
}

void getRequest(Requests *requestObj) {

    // Open the file specified by the path in read-only mode
    int fd = open(requestObj->path, O_RDONLY);

    if (fd == -1) { // If the file could not be opened, handle the error

        // If the file does not exist, return 404 error
        if (access(requestObj->path, F_OK) == -1) {
            handle_error(404, requestObj->inputFile);
        }

        // If the file exists but cannot be read, return 403 error
        else if (access(requestObj->path, R_OK) == -1) {
            handle_error(403, requestObj->inputFile);
        }

        // If there is another error, return 500 error
        else {
            handle_error(500, requestObj->inputFile);
        }
    }
    // If the file was successfully opened
    else {

        // Use fstat to get all file information.
        struct stat fileStat;

        if (fstat(fd, &fileStat) == -1) {
            handle_error(500, requestObj->inputFile);
        }

        // If the file is a directory, return 403 error
        if (S_ISDIR(fileStat.st_mode)) {
            handle_error(403, requestObj->inputFile);
        }

        // If the file is not a directory
        else {

            // Get the file size and send an HTTP response with the file size
            off_t fileSize = fileStat.st_size;
            dprintf(
                requestObj->inputFile, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", fileSize);

            // Call the pass_bytes helper function to read bytes from the file and write them to the response
            int bytesWritten = pass_bytes(fd, requestObj->inputFile, fileSize);

            // If there was an error during the pass_bytes call, handle the error
            if (bytesWritten == -1) {
                handle_error(500, requestObj->inputFile);
            }
        }

        // Close the file
        close(fd);
    }
}

void putRequest(Requests *requestObj) {

    int fd = open(requestObj->path, O_WRONLY | O_TRUNC, 0666);
    int status_code = 0;

    // Check if file already exists
    if (access(requestObj->path, F_OK) == 0) {
        // File already exists, truncate it
        if (fd == -1) {
            // Handle error if unable to open file for writing
            if (errno == EACCES) {
                handle_error(403, requestObj->inputFile); // No permission to truncate file
            } else {
                handle_error(500, requestObj->inputFile); // Internal server error
            }
        }
        status_code = 200; // File truncated successfully
    } else {
        // File does not exist, create it and open for writing
        fd = open(requestObj->path, O_WRONLY | O_CREAT, 0666);
        if (fd == -1) {
            // Handle error if unable to open file for writing
            if (errno == EACCES) {
                handle_error(403, requestObj->inputFile); // No permission to create file
            } else {
                handle_error(500, requestObj->inputFile); // Internal server error
            }
        }
        status_code = 201; // File created successfully
    }

    // Write request msg to file
    int bytesWritten = write_all(fd, requestObj->msg,
        requestObj
            ->bytesLeft); // Write the bytes that are left of the request message to the target file descriptor using the write_all function. The number of bytes written is stored in bytesWritten variable.
    if (bytesWritten == -1) {
        handle_error(500, requestObj->inputFile); // Internal server error
    }
    // Write any remaining data from input to file
    int totWritten
        = requestObj->msgSize
          - requestObj
                ->bytesLeft; // Calculate the total number of bytes already written to the file by subtracting the bytes that are left from the content length.
    bytesWritten = pass_bytes(requestObj->inputFile, fd, totWritten);
    if (bytesWritten == -1) {
        handle_error(500, requestObj->inputFile); // Internal server error
    }
    // Send response message
    if (status_code == 201) {
        dprintf(requestObj->inputFile,
            "HTTP/1.1 201 Created\r\nContent-Length: %d\r\n\r\nCreated\n", 8);
    } else if (status_code == 200) {
        dprintf(requestObj->inputFile, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\nOK\n", 3);
    }
    // Close the file
    close(fd);
}

int main(int argc, char *argv[]) {
    // Check command line for correct number of arguments
    if (argc != 2) {
        fprintf(stderr, "usage: ./httpserver <port>\n");
        exit(1);
    }

    // Declare variable to hold the Listener_Socket object
    Listener_Socket sd;

    int port_num = atoi(argv[1]); // Converting to integer

    if (port_num > 65535 || port_num < 1) {
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }

    // Initialize the listener socket with the provided port number and store the result in socketStatus
    int sock = listener_init(&sd, port_num);

    if (sock
        == -1) { // If the listener socket failed to initialize, print an error message and exit the program
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }

    char buf[BUFF_SIZE];
    memset(buf, '\0', sizeof(buf));

    bool x = true;
    while (x) {
        // Wait for a client to connect and obtain a file descriptor for the new socket.
        int client_socket = listener_accept(&sd);

        // Create a new Requests object and store the file descriptor in its inputFile field
        Requests requestObj;
        requestObj.inputFile = client_socket;

        if (client_socket == -1) {
            fprintf(stderr, "Error while establishing connection\n");
        }

        // Read data from the client and store it in the buffer
        ssize_t bytes_read = read_until(client_socket, buf, BUFF_SIZE, "\r\n\r\n");

        // Parse the request from the client and determine which action to take
        int parsed = parseRequest(&requestObj, buf, bytes_read);

        if (bytes_read == -1) {
            handle_error(400, requestObj.inputFile);
        }
        if (parsed != 1) {
            if (strncmp(requestObj.httpVersion, "HTTP/1.1", 8) != 0) {
                handle_error(505, requestObj.inputFile);
            } else if (requestObj.get_put[0] == 'G' && requestObj.get_put[1] == 'E'
                       && requestObj.get_put[2] == 'T') {

                // Verify if the GET request has a message or a content length if so, return a 400 Bad Request error to the client.
                if (requestObj.bytesLeft > 0) {
                    handle_error(400, requestObj.inputFile);
                }
                if (requestObj.msgSize != -1) {
                    handle_error(400, requestObj.inputFile);
                }
                getRequest(&requestObj);
            } else if (requestObj.get_put[0] == 'P' && requestObj.get_put[1] == 'U'
                       && requestObj.get_put[2] == 'T') {

                if (requestObj.msgSize == -1) {
                    handle_error(400, requestObj.inputFile);
                }
                // Check if target path is a directory
                if (isDirectory(requestObj.path) == 1) {
                    handle_error(403, requestObj.inputFile);
                }
                putRequest(&requestObj);
            } else {
                handle_error(501, requestObj.inputFile);
            }
        }
        // Close the connection to the client and clear the buffer
        close(client_socket);
        bzero(buf, sizeof(buf));
    }
    return (EXIT_SUCCESS);
}
