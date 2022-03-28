# copy_async

## Originally wanted handlers to execute reads/writes for each file

Unfortunately aio calls are not handler safe, so this cannot be done without race conditions. This would be kind of similar to ASHs (but obv in user space) where we can asynchronously execute the next operation.

## READ AHEAD

The buffer size is set at 128Kb this is the size of the read ahead on my linux machine. This allows us to perform copies much faster and without waste.