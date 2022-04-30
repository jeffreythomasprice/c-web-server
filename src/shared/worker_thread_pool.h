#ifndef worker_thread_pool_h
#define worker_thread_pool_h

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

// TODO thread pool errors should be an enum
#define WORKER_THREAD_POOL_SUCCESS 0
#define WORKER_THREAD_POOL_ERROR 1
#define WORKER_THREAD_POOL_ERROR_QUEUE_FULL 1
#define WORKER_THREAD_POOL_ERROR_TIMEOUT 2

typedef struct worker_thread_pool worker_thread_pool;

typedef struct worker_thread_pool_context {
	worker_thread_pool *pool;
	pthread_t thread;
	int thread_is_init;
	int running;
	int id;
} worker_thread_pool_context;

typedef int (*worker_thread_pool_callback)(int id, void *data);

struct worker_thread_pool_task;
typedef struct worker_thread_pool_task {
	struct worker_thread_pool_task *next;
	// provided by caller
	worker_thread_pool_callback callback;
	void *data;
	// will be filled in when result is available
	int *result;
	// signalled when task completes
	sem_t semaphore;
	// set to true if the task times out
	int timedout;
	// set to true when the task completes
	int completed;
} worker_thread_pool_task;

typedef struct worker_thread_pool {
	int num_threads;
	int max_queue_size;

	// locking around both the pool and the pending queue
	pthread_mutex_t tasks_mutex;
	int tasks_mutex_is_init;
	// signalled when tasks become available in the queue
	sem_t tasks_semaphore;
	int tasks_semaphore_is_init;
	// linked lists of tasks
	// the tasks that have been allocated but aren't in use
	worker_thread_pool_task *task_pool_first;
	int task_pool_len;
	// tasks that are waiting for threads to execute them
	worker_thread_pool_task *task_pending_first;
	worker_thread_pool_task *task_pending_last;
	int task_pending_len;

	worker_thread_pool_context *threads;
} worker_thread_pool;

/*
Initializes the thread pool with the given number of threads, and with a maximum number of tasks that can be queued at once.

Returns WORKER_THREAD_POOL_SUCCESS on success, WORKER_THREAD_POOL_ERROR on failure.

Fails if any of the inputs are invalid. Number of threads and queue size must be positive.
*/
int worker_thread_pool_init(worker_thread_pool *pool, int num_threads, int max_queue_size);

/*
Stops all worker threads and waits until all are completed. Frees all resources.

Returns WORKER_THREAD_POOL_SUCCESS on success, WORKER_THREAD_POOL_ERROR on failure.
*/
int worker_thread_pool_destroy(worker_thread_pool *pool);

/*
Enqueues a new job in the queue and blocks until it is complete.

If callback_result is provided and the task completes it's set to the result of the callback.

timeout is given in nanoseconds. A value of 0 means no timeout, timeout of -1 means infinite timeout.

Returns WORKER_THREAD_POOL_SUCCESS on success.

Returns WORKER_THREAD_POOL_ERROR_QUEUE_FULL if it can't start the task because the queue is full.

Returns WORKER_THREAD_POOL_ERROR_TIMEOUT if it started the task, but timed out waiting for the result.

Returns WORKER_THREAD_POOL_ERROR on all other errors.
*/
int worker_thread_pool_enqueue(worker_thread_pool *pool, worker_thread_pool_callback callback, void *data, int *callback_result,
							   uint64_t timeout);

#ifdef __cplusplus
}
#endif

#endif