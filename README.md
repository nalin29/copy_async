# 3 Implementation and Design
## 3.1 General Implementation
Our general implementation of all of the methods for copying files is to maintain a queue of
files and directories we need to process and a working set of files to request I/O operations
for. To start, we first perform Breadth-First Search on our source directory. If the next
structure is a directory, we will make an equivalent directory at our new destination and
start traversing this subdirectory to append files to our queue. If the structure was just a
file, we instead find its new name i.e., the name it would have in the destination directory.
We then populate the relevant I/O structures which will be discussed in further detail below.
## 3.2 Copy using aio
To use aio we first need to call aio_init with a aioinit struct which has a number of
parameters with most unused. We are interested in aio_threads which is the max number
of threads we want to use for the implementation, aio_num which is the maximum number of
requests that we will have enqueued simultaneously, and aio_idle_time which is the time
before idle threads will terminate. Next to make I/O requests, we must populate a aiocb
struct. We make two to handle the read and write for copying a file. These require a buffer,
file descriptor, number of bytes to read/write, and offset into the file which are referenced
with aio_buf, aio_fildes, aio_nbytes, and aio_offset, respectively.
Recall that aio only works asynchronously when we have the O_DIRECT flag set. Our imple-
mentations support three different ways of processing files, using single operations per file,
multiple operations per file, or batched operations. The specifics of these approaches are
outlined below.
### 3.2.1 Single Operation per File
Here in the main loop, we start queueing files to the working set and start making I/O
requests. After populating the aiocb struct, we can then start a read of the file in the source
directory. We continuously loop until we are done with open requests in our working set.
Inside this loop we check if the current file has finished reading and writing and if it has, we
add the next file to the queue and start processing that file. If it has only finished reading we
start the next write. Since we can only process files based on our buffer size, we read bytes
until our buffer is full then write those bytes and repeat until we are done with the file.
### 3.2.2 Multiple Operations Per File
For supporting multiple operations per file at a time, we now also maintain a queue of
currently executing files so that we can interweave reads and writes. We use the queue to
update the aiocb structs with the next pair of read and write operations that we want to
process.
### 3.2.3 Batched Operations
The idea behind batched operations it that we will be able to split our file into a series of
reads and writes and concurrently and asynchronously make requests on these files. This
is done similarly to multiple operations per file but we also have a list of read and write
aiocb structs to help us maintain these files. This leverages lio_opcode and lio_listio
which uses the list of aiocb structs to batch the operations based on the opcode. Note that
these operations can be executed in any order which does not necessarily matter in this use
case as long as we wait for all operations to finish before continuing. We ensure this by
passing LIO_WAIT as an argument to lio_listio which ensures that we block until all I/O
operations are complete.
## 3.3 Copy using io uring
To utilize io_uring we make our life easier by utilizing the API defined in the liburing
library. Similar to aio we need to initialize some of the command structures. The main
control structure is the struct io_uring. This refers to the combined ring structure which
we can pass to API calls to interact with the different ring buffer structures in io_uring.
Once we create the ring we can call io_uring_queue_init which will take in the queue
depth and pointer to the ring as well as special flags. This populates the ring data structure
very similarly to how we utilize aio_init. Then we can begin to start the I/O operations.
To perform a read we can gather a selection queue entry by calling io_uring_get_sqe. Then
to populate the entry we can call io_uring_prep_readv which takes in the entry, the source
file, an iov struct, the number of operations are supplied and an offset. The iov struct
is very similar to the aiocb structure used in aio but with even less constraints. We can
then set the data pointer using io_uring_sqe_set_data and then to start execution we can
place it in the selection queue by calling io_uring_submit which will submit all outstanding
entries for execution. For write operations we can use the similar io_uring_prep_writev.
The liburing library also provides a useful feature that allows to order certain operations.
These are called linked entries. After getting the entry and before prepping to the selection
queue we can add the IOSQE_IO_LINK flag by calling io_uring_sqe_set_flags. The next
entry that is prepped is then linked to this entry. This means that io_uring will ensure it is
ordered after the predecessor operation. We can generate arbitrary size length chains by also
adding the IOSQE_IO_LINK flag to the next entry. In this way we can easily create read-write
pairs. By linking read operations to write operations with the same buffers and offsets.
A special optimization that liburing exposes is the use of registered buffers. These are
buffers that we allow io_uring to know ahead of time. This reduces the cost of having
to redo buffer mappings into the kernel on every entry, since we now provide an index
representing the already mapped buffer to io_uring. We implemented this optimization
as an optional feature -rb. To implement this first we must register buffers with using
io_uring_register_buffers which takes a list of pointers to buffers. Then to execute
operations using these registered buffers we make use of io_uring_prep_write_fixed and
io_uring_prep_read_fixed. For these operations we must also pass an index referring to
the buffer index in the registered buffer list.
### 3.3.1 Single Operation per File
The first implementation of our copy program using io_uring was to have a single read-write
pair per file to see if we can make use of executing operations on multiple files at once. This
is implemented by generating a read-write pair file using the process mentioned above and
executing as many as we can. Then in a structured main loop we keep track of the number
of currently executing pairs and collect them using io_uring_submit_and_wait this lets us
retrieve the completion queue entry that contains the requisite information for this operation.
We then check to see that both the read and write operations for that file have checked in and
then either add a new read-write pair for the next offset in the file or start a new read-write
pair for the next file, submitting them immediately. To ensure that the completion queue
entry is popped off the queue we must use io_uring_cqe_seen after using the entry allowing
for it to be deallocated. This process is repeated until there are no more in-flight operations.
### 3.3.2 Multiple Operations Per File
The second implementation performs multiple operations per file making use of the asyn-
chronous executions to perform reads and writes at different operations of each file. This
also allows us to perform multiple reads and writes to the same file and different files at the
same time. This implemented very similarly as when only have one operation pair per file.
The only difference is now we must also track when all operations for a file have checked in
so the file may be closed and the current offset in the file for future operations.
### 3.3.3 Batched Operations
We then implemented batched operations. In this case we do not immediately submit
all the entries rather we wait for all entries to first check in. This can be done using
io_uring_submit_and_wait which submits all entries and then waits for completion. We
can then iterate over the entries and prepare new entries as we did in multiple operations per
file. The next loop will then submit and wait for completion allowing us to batch operations
with ease. We expect this approach to be the fastest as I/O controllers will be able to better
reorganize operations if they arrive all at once providing a performance boost