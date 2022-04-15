#ifndef worker_thread_pool_h
#define worker_thread_pool_h

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <semaphore.h>

typedef struct worker_thread_pool worker_thread_pool;

typedef struct worker_thread_pool_context {
	worker_thread_pool *pool;
	pthread_t thread;
	int thread_is_init;
	int running;
	int id;
} worker_thread_pool_context;

struct worker_thread_pool_task;
typedef struct worker_thread_pool_task {
	struct worker_thread_pool_task *next;
	// provided by caller
	void *data;
	// will be filled in when result is available
	int *result;
	// signalled when task completes
	sem_t semaphore;
} worker_thread_pool_task;

typedef int (*worker_thread_pool_callback)(int id, void *data);

typedef struct worker_thread_pool {
	worker_thread_pool_callback callback;
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

Returns 0 on success, non-0 on failure.

Fails if any of the inputs are invalid. Callback is required, number of threads and queue size must be positive.
*/
int worker_thread_pool_init(worker_thread_pool *pool, worker_thread_pool_callback callback, int num_threads, int max_queue_size);

/*
Stops all worker threads and waits until all are completed. Frees all resources.

Returns 0 on success, non-0 on failure.
*/
int worker_thread_pool_destroy(worker_thread_pool *pool);

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