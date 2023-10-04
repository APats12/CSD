# Multi-threaded HTTP Server

This is a simple HTTP server program that handles client connections and serves GET and PUT requests. It supports multiple concurrent connections using a thread pool.

## Usage
Run the compiled executable with the following command:

./httpserver [-t threads] <port>

The -t option allows you to specify the number of threads to use (default is 4).
<port> specifies the port number on which the server will listen for connections.

## Implementation Details

### Main Function

The main function initializes the server, processes command-line arguments, and sets up the server's listener socket. It also initializes the mutex lock, queue, and threads for handling incoming connections. The number of threads can be specified using the -t option. The main function enters a loop to accept new connections and pushes them into the shared queue for processing by the worker threads.

### Connection Handling Functions

The program provides several functions to handle different types of connections:

#### handle_connection: 
Handles a client connection by parsing the request and sending the appropriate response. If the request is a GET or PUT request, it calls the corresponding handler function. If the request is unsupported, it calls the handle_unsupported function.

#### handle_get: 
Handles a GET request by opening the requested file, checking for errors, and sending the file contents as a response. It also logs the GET request details to stderr.

#### handle_put: 
Handles a PUT request by creating or opening the requested file, checking for errors, and receiving the file contents from the client. It sends the appropriate response and logs the PUT request details to stderr.

#### handle_unsupported: 
Handles an unsupported request by sending a "501 Not Implemented" response and logging the details to stderr.

### Helper Functions

The program provides additional helper functions:

#### handle_get_log: 
Logs the details of a GET request.

#### process_connection: 
Thread function responsible for handling incoming client connections. It retrieves a connection from the shared queue, processes it, and closes the connection.
