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
#include <time.h>
#include <liburing.h>

#define QD 2
#define BS (128 * 1024)

static int infd, outfd;

struct uringEntry
{
    int read;
    off_t off;
    size_t length;
    struct iovec iov;
    char *buffer;
};

int main(int argc, char *argv[])
{

    double time_spent = 0.0;

    clock_t begin = clock();

    struct io_uring* ring;
    off_t file_size;
    int ret;

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

    ring = malloc(sizeof(struct io_uring));

    ret = io_uring_queue_init(QD, ring, 0);

    struct uringEntry *read_entry = malloc(sizeof(struct uringEntry));
    struct uringEntry *write_entry = malloc(sizeof(struct uringEntry));

    read_entry->buffer = malloc(BS);
    write_entry->buffer = read_entry->buffer;

    struct io_uring_sqe *sqe_read;
    struct io_uring_sqe *sqe_write;

    sqe_read = io_uring_get_sqe(ring);

    read_entry->read = 1;
    read_entry->off = 0;
    read_entry->length = BS;
    read_entry->iov.iov_base = read_entry->buffer;
    read_entry->iov.iov_len = BS;

    io_uring_prep_readv(sqe_read, infd, &read_entry->iov, 1, read_entry->off);
    io_uring_sqe_set_data(sqe_read, read_entry);

    write_entry->read = 0;
    write_entry->off = 0;
    write_entry->iov.iov_base = write_entry->buffer;
    write_entry->iov.iov_len = BS;

    io_uring_submit(ring);

    
    while (1)
    {
        struct io_uring_cqe *cqe;
        struct uringEntry *entry;
        ret = io_uring_wait_cqe(ring, &cqe);
        if (ret < 0)
        {
            perror("Failed on retrieving cqe");
            exit(1);
        }
        if (!cqe)
            break;
        entry = (struct uringEntry*) io_uring_cqe_get_data(cqe);
        int res = cqe->res;
        if (entry->read)
        {
            int num_read = res;
            if(num_read == 0){
                io_uring_cqe_seen(ring, cqe);
                break;
            }
            read_entry->off += num_read;
            write_entry->iov.iov_len = num_read;
            sqe_write = io_uring_get_sqe(ring);
            io_uring_prep_writev(sqe_write, outfd, &write_entry->iov, 1, write_entry->off);
            io_uring_sqe_set_data(sqe_write, write_entry);
            ret = io_uring_submit(ring);
        }
        else
        {
            int num_write = cqe->res;
            write_entry->off += num_write;
            read_entry->iov.iov_len = BS;
            sqe_read = io_uring_get_sqe(ring);
            io_uring_prep_readv(sqe_read, infd, &read_entry->iov, 1, read_entry->off);
            io_uring_sqe_set_data(sqe_read, read_entry);
            io_uring_submit(ring);
        }
        io_uring_cqe_seen(ring, cqe);
    }

    //ret = copy_file(ring, file_size);

    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    printf("The elapsed time is %f seconds", time_spent);

    close(infd);
    close(outfd);
    io_uring_queue_exit(ring);
    return ret;
}