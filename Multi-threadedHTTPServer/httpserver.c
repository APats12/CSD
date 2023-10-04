#include "asgn4_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "request.h"
#include "response.h"
#include "queue.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/file.h>

void handle_connection(int);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
void *process_connection();
void handle_get_log(char *uri, int code, conn_t *conn, const Response_t *res);

queue_t *new_q;
pthread_mutex_t mut;

int main(int argc, char **argv) {
    int option = 0;
    int num_threads = 4; // Set default number of threads to 4
    // Process command-line options using getopt
    while ((option = getopt(argc, argv, "t:")) != -1) {
        // Continue looping until all options have been processed (-1 indicates end of options)
        switch (option) {
        case 't':
            // Option -t: Set the number of threads
            num_threads = atoi(optarg);
            if (num_threads < 0) {
                fprintf(stderr, "Invalid thread size.\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            // Invalid option or missing arguments
            fprintf(stderr, "Usage: httpserver [-t threads] <port>\n");
            break;
        }
    }
    int errchk = optind + 1;
    while (errchk < argc) {
        fprintf(stderr,
            "Usage: httpserver [-t threads] <port>\n"); // Additional arguments following <port> argument
        return EXIT_FAILURE;
        errchk++;
    }
    // Using starter code from resources
    if (argc < 2) {
        warnx("wrong arguments: %s port_num",
            argv[optind]); // Using optind instead of hardcoded indices
        fprintf(stderr, "usage: %s <port>\n",
            argv[optind]); // Using optind instead of hardcoded indices
        return EXIT_FAILURE;
    }

    char *endptr = NULL;
    size_t port
        = (size_t) strtoull(argv[optind], &endptr, 10); // Using optind instead of hardcoded indices

    if (endptr && *endptr != '\0') {
        warnx("invalid port number: %s", argv[optind]); // Using optind instead of hardcoded indices
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);
    Listener_Socket sock;
    listener_init(&sock, port);
    // End starter code from resources

    // Set up mutex lock, queue, and threads
    pthread_mutex_init(&mut, NULL); // Used for put
    new_q = queue_new(num_threads);
    pthread_t th[num_threads];
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&(th[i]), NULL, process_connection, (void *) new_q);
    }

    // Set up dispatcher thread
    while (1) {
        // Accept a new connection
        uintptr_t connfd = listener_accept(&sock);
        // Push the connection into the queue
        queue_push(new_q, (void *) connfd);
    }
}

// Using starter code from resources
void handle_connection(int connfd) {
    conn_t *conn = conn_new(connfd);
    const Response_t *res = conn_parse(conn);
    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        debug("%s", conn_str(conn));
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
            handle_get(conn);
        } else if (req == &REQUEST_PUT) {
            handle_put(conn);
        } else {
            handle_unsupported(conn);
        }
    }
    conn_delete(&conn);
    return;
}

void handle_get_log(char *uri, int code, conn_t *conn, const Response_t *response) {
    conn_send_response(conn, response);
    char *requestId = conn_get_header(conn, "Request-Id");
    if (requestId == NULL) {
        requestId = "0"; // The requestID header was not found in the request
    }
    fprintf(stderr, "GET,/%s,%d,%s\n", uri, code, requestId); // Log the error details
}

void handle_get(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    const Response_t *response = NULL;
    int fd = open(uri, O_RDONLY);
    int code;
    if (fd < 0) {
        if (errno == EACCES) {
            response = &RESPONSE_FORBIDDEN;
            code = 403; // Set response code for forbidden access
        } else if (errno == ENOENT) {
            response = &RESPONSE_NOT_FOUND;
            code = 404; // Set response code for file not found
        } else {
            response = &RESPONSE_INTERNAL_SERVER_ERROR;
            code = 500; // Set response code for internal server error
        }
        handle_get_log(uri, code, conn, response);
        return;
    }
    // Lock the file in shared mode
    if (flock(fd, LOCK_SH) == -1) {
        response = &RESPONSE_INTERNAL_SERVER_ERROR;
        code = 500; // Set response code for internal server error
        handle_get_log(uri, code, conn, response);
        close(fd);
        return;
    }
    // Using fstat to get file information
    struct stat file_information;
    if (fstat(fd, &file_information) == -1) {
        response = &RESPONSE_INTERNAL_SERVER_ERROR;
        code = 500; // Set response code for internal server error
        handle_get_log(uri, code, conn, response);
        close(fd);
        return;
    }
    off_t size = file_information.st_size; //file size
    // Check if directory
    if (S_ISDIR(file_information.st_mode)) {
        response = &RESPONSE_FORBIDDEN;
        code = 403; // Set response code for forbidden access
        handle_get_log(uri, code, conn, response);
        flock(fd, LOCK_UN);
        close(fd);
        return;
    }
    // Send file
    response = conn_send_file(conn, fd, size);
    char *requestId = conn_get_header(conn, "Request-Id");
    if (requestId == NULL) {
        requestId = "0"; // The requestID header was not found in the request
    }
    fprintf(stderr, "GET,/%s,200,%s\n", uri, requestId); // Print successful GET request to stderr
    // Unlock and close the file
    flock(fd, LOCK_UN);
    close(fd);
    return;
}

void handle_put(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    // Check for if file exists
    bool file_exists = access(uri, F_OK) == 0;

    // Acquire the mutex lock to enter the critical region
    pthread_mutex_lock(&mut);

    const Response_t *response = NULL;
    // Create/Open File
    int fd = open(uri, O_CREAT | O_WRONLY, 0600);
    if (fd < 0) {
        // Check for specific error conditions
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            response = &RESPONSE_FORBIDDEN;
            goto send_response; // Jump to the send_response label
        } else {
            response = &RESPONSE_INTERNAL_SERVER_ERROR;
            goto send_response; // Jump to the send_response label
        }
    }
    flock(fd, LOCK_EX);

    // Release the mutex lock to exit the critical region
    pthread_mutex_unlock(&mut);

    ftruncate(fd, 0); // Truncate the file to size 0
    response = conn_recv_file(conn, fd);
    if (response == NULL && file_exists) {
        response
            = &RESPONSE_OK; // If response is NULL and file existed, set response to RESPONSE_OK
        //goto send_response;
    } else if (response == NULL && !file_exists) {
        response
            = &RESPONSE_CREATED; // If response is NULL and file did not exist, set response to RESPONSE_CREATED
        //goto send_response;
    } else {
        response
            = &RESPONSE_INTERNAL_SERVER_ERROR; // If none of the above conditions are met, set response to RESPONSE_INTERNAL_SERVER_ERROR
    }
send_response:
    conn_send_response(conn, response);
    char *requestId = conn_get_header(conn, "Request-Id");
    if (requestId == NULL) {
        requestId = "0"; // The requestID header was not found in the request
    }
    int code;
    if (response == &RESPONSE_OK) {
        code = 200;
    } else if (response == &RESPONSE_CREATED) {
        code = 201;
    } else if (response == &RESPONSE_FORBIDDEN) {
        code = 403;
    } else {
        code = 500;
    }
    fprintf(stderr, "PUT,/%s,%d,%s\n", uri, code, requestId);
    flock(fd, LOCK_UN); // Unlock the file
    close(fd);
}

void handle_unsupported(conn_t *conn) {
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
    char *requestId = conn_get_header(conn, "Request-Id");
    if (requestId == NULL) {
        requestId = "0"; // The requestID header was not found in the request
    }
    char *uri = conn_get_uri(conn);
    fprintf(stderr, "Response Not Implemented,/%s,501,%s\n", uri, requestId);
}
/* 
* process_connection() is responsible for handling incoming client 
* connections. It retrieves a connection from the shared queue, 
* processes it, and closes the connection, allowing the server to 
* handle multiple connections concurrently. 
*/
void *process_connection() {
    while (true) {
        // Get the connection file descriptor from the queue
        uintptr_t cfd = -1;
        queue_pop(new_q, (void **) &cfd);
        // Process the connection
        handle_connection(cfd);
        // Close the connection
        close(cfd);
    }
}
