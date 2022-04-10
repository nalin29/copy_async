#define _GNU_SOURCE 

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/io_uring.h>
#include <sys/time.h>
#include <liburing.h>
#include "copy.h"
#include "list.h"
#include "logging.h"

#define QD 128
#define BS (128 * 1024)

static int infd, outfd;

int main(int argc, char *argv[])
{

    int fallocate_option = 0;
    int register_option = 0;

    struct timespec start, end;

    CHECK_ERROR(clock_gettime(CLOCK_MONOTONIC, &start), "getting start time");

    struct io_uring *ring;
    off_t file_size;

    if (argc < 3)
    {
        printf("Usage: %s <infile> <outfile>\n", argv[0]);
        return 1;
    }

    infd = open(argv[1], O_RDONLY);
    if (infd < 0)
    {
        perror("open infile");
        return 1;
    }

    outfd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outfd < 0)
    {
        perror("open outfile");
        return 1;
    }

    for (int i = 3; i < argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            fallocate_option = 1;
        }
        else if (strcmp(argv[i], "-rb") == 0)
        {
            register_option = 1;
        }
    }

    if (fallocate_option)
    {
        struct stat file_stat;
        CHECK_ERROR(fstat(infd, &file_stat), "fstat");
        file_size = file_stat.st_size;
        CHECK_ERROR(fallocate(outfd, 0, 0, file_size), "fallocate");
    }

    ring = malloc(sizeof(struct io_uring));
    CHECK_NULL(ring, "allocating ring");

    CHECK_ERROR(io_uring_queue_init(QD, ring, 0), "init ring");

    struct ioUringEntry *entry = (struct ioUringEntry *)malloc(sizeof(struct ioUringEntry));
    CHECK_NULL(entry, "allocating entry");

    entry->buffer = malloc(BS);
    CHECK_NULL(entry->buffer, "allocating buffer");

    entry->iov = (struct iovec*)malloc(sizeof(struct iovec));
    CHECK_NULL(entry->iov, "allocating iov");

    struct io_uring_sqe *sqe;

    entry->read = 1;
    entry->readOff = 0;
    entry->writeOff = 0;
    entry->fdDest = outfd;
    entry->fdSrc = infd;
    entry->iov->iov_base = entry->buffer;
    entry->iov->iov_len = BS;

    if (register_option)
    {
        CHECK_ERROR(io_uring_register_buffers(ring, entry->iov, 1), "registering buffer");
    }

    sqe = io_uring_get_sqe(ring);
    CHECK_NULL(sqe, "getting sqe");

    if (register_option)
    {
        io_uring_prep_read_fixed(sqe, infd, entry->buffer, BS, entry->readOff, 0);
    }
    else
    {
        io_uring_prep_readv(sqe, infd, entry->iov, 1, entry->readOff);
    }

    io_uring_sqe_set_data(sqe, entry);

    CHECK_ERROR(io_uring_submit(ring), "submitting to ring");

    while (1)
    {
        struct io_uring_cqe *cqe;
        struct ioUringEntry *entry;
        CHECK_ERROR(io_uring_wait_cqe(ring, &cqe), "waiting for cqe");
        if (!cqe)
            break;
        entry = (struct ioUringEntry *)io_uring_cqe_get_data(cqe);
        CHECK_NULL(entry, "getting entry from cqe");

        int res = cqe->res;
        CHECK_ERROR(res, "result of cqe");

        if (entry->read)
        {
            int num_read = res;
            if (num_read == 0)
            {
                io_uring_cqe_seen(ring, cqe);
                break;
            }
            entry->read = 0;
            entry->readOff += num_read;
            entry->iov->iov_len = num_read;
            sqe = io_uring_get_sqe(ring);
            CHECK_NULL(sqe, "getting sqe");
            if (register_option)
            {
                io_uring_prep_write_fixed(sqe, outfd, entry->buffer, num_read, entry->writeOff, 0);
            }
            else
            {
                io_uring_prep_writev(sqe, outfd, entry->iov, 1, entry->writeOff);
            }

            io_uring_sqe_set_data(sqe, entry);

            CHECK_ERROR(io_uring_submit(ring), "submitting to ring");
        }
        else
        {
            int num_write = cqe->res;
            entry->read = 1;
            entry->writeOff += num_write;
            entry->iov->iov_len = BS;
            sqe = io_uring_get_sqe(ring);
            CHECK_NULL(sqe, "getting sqe");

            if (register_option)
            {
                io_uring_prep_read_fixed(sqe, infd, entry->buffer, BS, entry->readOff, 0);
            }
            else
            {
                io_uring_prep_readv(sqe, infd, entry->iov, 1, entry->readOff);
            }

            io_uring_sqe_set_data(sqe, entry);
            CHECK_ERROR(io_uring_submit(ring), "submitting to ring");
        }
        io_uring_cqe_seen(ring, cqe);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken;
    time_taken = (end.tv_sec - start.tv_sec) * 1e9;
    time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
    printf("The elapsed time is %f seconds\n", time_taken);

    close(infd);
    close(outfd);
    io_uring_queue_exit(ring);
    return 0;
}