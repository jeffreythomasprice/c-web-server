#ifndef worker_thread_pool_h
#define worker_thread_pool_h

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

typedef struct {
	worker_thread_pool *pool;
	int id;
} worker_thread_pool_context;

typedef struct {
	void *data;
} worker_thread_pool_task;

typedef int (*worker_thread_pool_callback)(worker_thread_pool_context *context, void *data);

typedef struct {
	worker_thread_pool_callback callback;
	int queue_size;
	int num_threads;

	// TODO need a lock for task queue
	// the task queue, capcaity = queue_size
	worker_thread_pool_task *tasks;
	// the index of the next task that should be dequeued
	int next_task;
	// the number of tasks currently enqueued
	int tasks_len;

	worker_thread_pool_context *contexts;
	pthread_t *threads;
} worker_thread_pool;

/*
Initializes the thread pool with the given number of threads, and with a maximum number of tasks that can be queued at once.

Returns 0 on success, non-0 on failure.

Fails if any of the inputs are invalid. Callback is required, number of threads and queue size must be positive.
*/
int worker_thread_pool_init(worker_thread_pool *pool, worker_thread_pool_callback callback, int num_threads, int queue_size);

/*
Stops all worker threads and waits until all are completed. Frees all resources.

Returns 0 on success, non-0 on failure.
*/
int worker_thread_pool_stop_and_join(worker_thread_pool *pool);

/*
Enqueues a new job in the queue and blocks until it is complete.

Returns 0 on success, non-0 on failure.

Fails if the queue is full.

If thread_result is provided and the task completes thread_result is set to the result of the callback.
*/
int worker_thread_pool_enqueue(worker_thread_pool *pool, void *data, int *thread_result);

#ifdef __cplusplus
}
#endif

#endif