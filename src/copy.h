/**
 * @file copy.h
 * @author Nalin Mahajan, Vineeth Bandi
 * @brief Data structures used for copying
 * @version 0.1
 * @date 2022-04-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <aio.h>
#include <signal.h>
#include <linux/io_uring.h>
#include <liburing.h>
#include <sys/ioctl.h>

// contains data on file
struct file_entry
{
   int infd;                  // the read file fd
   int outfd;                 // the write file fd
   off_t remainingBytes;      // remaining bytes to read/write
   unsigned long ops;
   off_t off;                 // current offset in file
};

// used for aio rec, contains necessary data
struct ioEntry
{
   int reading;               // if entry is currently readin
   int fdSrc;                 // fd of src file
   int fdDest;                // fd of dest file
   int readOff;               // offset for reading
   int last;                  // if io is last (unused)
   int writeOff;              // offset for writing
   char *srcName;             // name of src file (unused)
   char *destName;            // name of dest file (unused)
   char *buffer;              // pointer to read/write buffer
   int readStatus;            // status of read operation
   int writeStatus;           // status of write operation
   struct file_entry* fe;
   struct aiocb *read_aiocb;  // pointer to aio cb for reads
   struct aiocb *write_aiocb; // pointer to aio cb for writes
};

// used for ioUring rec, contains necessary data
struct ioUringEntry
{ 
   int fdSrc;                 // fd for src file
   int fdDest;                // fd for dest file
   int last;                  // indicator if last op on file (unused)
   int read;                  // if operation is a read
   int readOff;               // offset for reading
   int writeOff;              // offset for writing
   char* buffer;              // pointer to read/write buffer
   int buff_index;            // index of buffer (used for registering buffers)
   off_t file_size;           // size of file total
   int op_count;              // number of operations used for (inter-file operations)
   struct file_entry* fe;
   struct iovec* iov;         // io vector used for non registered operations
};


