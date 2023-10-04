#  httpserver.c 

This program is a simple HTTP server that implements the GET and PUT methods for serving static files. The server runs on a specified port number and serves files from a specified root directory.

# Usage

Compile the program by running make in the root directory. This will generate an executable called http_server.

To start the server, run the following command

./http_server <"port">

Once the server is running, you can access it by navigating to http://localhost:<"port"> in your web browser.

# Supported Methods

The server supports the following HTTP methods:

GET: Returns the contents of a file specified in the URI.

PUT: Saves the contents of the request message body to a file specified in the URI.

# Supported Status Codes

The server supports the following HTTP status codes:

200 OK: The request was successful.

201 Created: The URI’s file is created

400 Bad Request: The request was malformed or invalid.

403 Forbidden: The server could not access the requested file.

404 Not Found: The requested file does not exist.

500 Internal Server Error: An unexpected error occurred.

501 Not Implemented: The requested method is not implemented.

505 Version Not Supported: The HTTP version is not supported.

# Implementation Details

The server uses system calls and regular expressions to parse incoming HTTP requests. When a request is received, the server checks if the requested file exists and whether the requested method is supported. If the requested file does not exist or the method is not supported, the server returns an appropriate HTTP status code. If the request is valid, the server processes the request and returns the appropriate response.

Parsing HTTP Requests: The server uses regular expressions to parse incoming HTTP requests. It extracts information such as the HTTP method (GET or PUT), the requested URI (Uniform Resource Identifier), and the HTTP version. This parsing allows the server to understand and process client requests effectively.

File Handling: When handling GET requests, the server checks if the requested file exists within the specified root directory. If the file exists, it reads its contents and sends them as the response body. For PUT requests, the server verifies whether the requested file exists and whether the HTTP request contains a message body. If these conditions are met, it saves the message body as the content of the file.

HTTP Status Codes: The server is equipped to respond with appropriate HTTP status codes based on the outcome of request processing. For example, a 200 OK status is sent upon a successful GET request, while a 404 Not Found status is returned if the requested file does not exist.

Error Handling: Robust error handling is implemented to ensure that the server responds appropriately to various scenarios. For instance, if a client sends a malformed or unsupported request, the server issues a 400 Bad Request status code. In cases where the server cannot access the requested file due to permission issues, a 403 Forbidden status code is returned. Additionally, unexpected server errors trigger a 500 Internal Server Error status.

HTTP Version Support: The server supports HTTP/1.1, and it checks incoming requests for compatibility with this version. If a client uses an unsupported HTTP version, the server responds with a 505 Version Not Supported status code.

