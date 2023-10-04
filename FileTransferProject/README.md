# memory.c - File Transfer Program

This is a simple C program for a file transfer utility. It reads commands from standard input (stdin) and performs two operations: get and set. The program uses a fixed-size buffer for reading commands and transferring files.

# Implementation Details 

The program reads and processes commands from standard input. It performs error checking to validate the commands and handle errors gracefully.For the get command, it reads a file from standard input and prints its contents to standard output. For the set command, it reads the content from standard input and writes it to a file with the specified name.

# Usage

You can use this program to transfer files between the standard input (stdin) and the standard output (stdout). The program expects commands in the following format:

```
<COMMAND> <FILENAME>
```
'<COMMAND>' can be either get or set.
'<FILENAME>'  is the name of the file to be transferred.




