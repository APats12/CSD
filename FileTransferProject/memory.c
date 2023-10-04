#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFF_SIZE    16384
#define COMMAND_SIZE 4

void print_error(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

void get_helper(char *filename) {
    // Open the file for reading
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        print_error("Invalid Command");
        return;
    }
    ssize_t rb, wb; // hold the number of bytes read and written
    char buf[BUFF_SIZE];
    while (1) {
        rb = read(fd, buf, sizeof(buf)); // Read from the file into the buffer
        if (rb == -1) {
            print_error("Invalid Command");
            close(fd);
            return;
        } else if (rb == 0) { // EOF
            break;
        }
        ssize_t written = 0;
        while (written < rb) { // Loop until we've written all the bytes read
            wb = write(STDOUT_FILENO, buf + written, rb - written);
            if (wb == -1) {
                print_error("Invalid Command");
                close(fd);
                return;
            }
            written += wb; // Increment the number of bytes written
        }
    }
    close(fd);
}

int main(void) {
    char main_buf[BUFF_SIZE];
    int command = COMMAND_SIZE;
    int filename = PATH_MAX * 2;

    // Reading command from stdin
    ssize_t bytes_read = 0;
    while (bytes_read < command) {
        ssize_t ret = read(STDIN_FILENO, main_buf + bytes_read, command - bytes_read);
        if (ret == -1) {
            print_error("Invalid Command");
        }
        if (ret == 0) { // EOF
            break;
        }
        bytes_read += ret; // Increment the number of bytes read
    }

    // Remove space and make sure command is valid
    if (main_buf[3] != ' ') {
        print_error("Invalid Command"); // no space between command and filename
    } else if (main_buf[3] == ' ') {
        main_buf[3] = '\0';
    }
    if (strcmp(main_buf, "get") != 0 && strcmp(main_buf, "set") != 0) {
        print_error("Invalid Command"); // invalid command (not GET or SET)
    }

    // If command is GET
    if (strcmp(main_buf, "get") == 0) {
        // Reading in filename
        bytes_read = 0;
        while (bytes_read < (filename - 1)) {
            ssize_t ret = read(STDIN_FILENO, main_buf + bytes_read, filename - bytes_read - 1);
            if (ret == -1) {
                print_error("Invalid Command");
            }
            if (ret == 0) { // EOF
                break;
            }
            bytes_read += ret; // Increment the number of bytes read
        }
        // Check for, remove, and replace newline.
        main_buf[filename] = '\0';
        char *ptr = main_buf;
        while (*ptr != '\n') {
            if (*ptr == ' ' || *ptr == '\0') {
                print_error("Invalid Command");
            }
            ptr++;
        }
        *ptr = '\0';
        if ((bytes_read - (ptr - main_buf + 1)) > 0) {
            print_error("Invalid Command");
        }
        // Reads a file and prints its contents to the stdout.
        get_helper(main_buf);

    }

    // If command is SET
    else if (strcmp(main_buf, "set") == 0) {
        // reading in filename
        bytes_read = 0;
        while (bytes_read < (filename - 1)) {
            ssize_t ret = read(STDIN_FILENO, main_buf + bytes_read, filename - bytes_read - 1);
            if (ret == -1) {
                print_error("Invalid Command");
            }
            if (ret == 0) {
                break;
            }
            bytes_read += ret; // Increment the number of bytes read
        }
        // Check for, remove, and replace newline.
        main_buf[filename] = '\0';
        char *ptr = main_buf;
        while (*ptr != '\n') {
            if (*ptr == ' ' || *ptr == '\0') {
                print_error("Invalid Command");
            }
            ptr++;
        }
        *ptr = '\0';
        // opening file for write
        int fd = open(main_buf, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            print_error("Invalid Command");
        }
        int wb, written = 0;
        int rb;
        bytes_read = bytes_read - (ptr - main_buf) - 1; // the remaining bytes are the content
        char *content = ptr + 1;
        while (written < bytes_read) {
            wb = write(fd, content + written, bytes_read - written);
            if (wb == -1) {
                print_error("Invalid Command");
                close(fd);
            }
            written += wb;
        }
        while ((rb = read(STDIN_FILENO, main_buf, PATH_MAX))
               > 0) { // reading the remaining content from STDIN
            int wb, written = 0;
            if (rb == -1) {
                print_error("Invalid Command");
                close(fd);
            }
            while (written < rb) {
                wb = write(fd, main_buf + written, rb - written);
                if (wb == -1) {
                    print_error("Invalid Command");
                    close(fd);
                }
                written += wb;
            }
        }
        const char *msg = "OK\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        close(fd);
    }
}
