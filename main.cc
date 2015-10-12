#include "kfifo.h"

#include "common/tpool/Thread.h"
#include "common/tpool/BoundedBlockingQueue.h"

#include <assert.h>
#include <boost/bind.hpp>


static void KfifoWriter(kfifo* fifo, int test_num)
{
    for (int i = 0; i < test_num; ++i) {
        while (__kfifo_put(fifo, (unsigned char*)&i, sizeof(i)) == 0) {}
    }
}

static void KfifoReader(kfifo* fifo, int test_num)
{
    int num = 0;
    for (int i = 0; i < test_num; ++i) {
        while (__kfifo_get(fifo, (unsigned char*)&num, sizeof(num)) == 0) {}
    }
}

static void KfifoLockWriter(kfifo* fifo, int test_num)
{
    for (int i = 0; i < test_num; ++i) {
        while (kfifo_put(fifo, (unsigned char*)&i, sizeof(i)) == 0) {}
    }
}

static void KfifoLockReader(kfifo* fifo, int test_num)
{
    int num = 0;
    for (int i = 0; i < test_num; ++i) {
        while (kfifo_get(fifo, (unsigned char*)&num, sizeof(num)) == 0) {}
    }
}

static void TpoolWriter(::tpool::BoundedBlockingQueue<int>* queue, int test_num)
{
    for (int i = 0; i < test_num; ++i) {
        queue->Push(i);
    }
}

static void TpoolReader(::tpool::BoundedBlockingQueue<int>* queue, int test_num)
{
    int num = 0;
    for (int i = 0; i < test_num; ++i) {
        queue->Pop(num);
    }
}

int main(int argc, char** argv)
{
    int test_num = 100000;
    if (argc > 1) {
        test_num = atoi(argv[1]);
    }

    int test_type = 0;
    if (argc > 2) {
        test_type = atoi(argv[2]);
    }

    if (test_type == 1) {
        ::tpool::BoundedBlockingQueue<int> test_queue((1024));
        ::tpool::Thread t1((::boost::bind(&TpoolReader, &test_queue, test_num)));
        ::tpool::Thread t2((::boost::bind(&TpoolWriter, &test_queue, test_num)));
    } else if (test_type == 2) {
        pthread_mutex_t lock;
        pthread_mutex_init(&lock, NULL);

        kfifo* test_fifo = kfifo_alloc(1024 * sizeof(int), 0, &lock);

        {
            ::tpool::Thread t1((::boost::bind(&KfifoLockReader, test_fifo, test_num)));
            ::tpool::Thread t2((::boost::bind(&KfifoLockWriter, test_fifo, test_num)));
        }

        kfifo_free(test_fifo);
        pthread_mutex_destroy(&lock);
    } else {
        pthread_mutex_t lock;
        pthread_mutex_init(&lock, NULL);

        kfifo* test_fifo = kfifo_alloc(1024 * sizeof(int), 0, &lock);

        {
            ::tpool::Thread t1((::boost::bind(&KfifoReader, test_fifo, test_num)));
            ::tpool::Thread t2((::boost::bind(&KfifoWriter, test_fifo, test_num)));
        }

        kfifo_free(test_fifo);
        pthread_mutex_destroy(&lock);
    }

    return 0;
}
