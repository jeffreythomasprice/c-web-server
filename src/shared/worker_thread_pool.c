#include "worker_thread_pool.h"

#define ONE_MILLISECOND_IN_MICROSECONDS 1000
#define ONE_SECOND_IN_MICROSECONDS 1000000

void *worker_thread_pool_pthread_callback(void *data) {
	worker_thread_pool_context *context = data;
	while (context->running) {
		// TODO wait for signal don't just sleep
		usleep(ONE_MILLISECOND_IN_MICROSECONDS * 10);

		/*
		TODO get work from queue, if available do it and execute callback with result
		*/
	}
}

int worker_thread_pool_init(worker_thread_pool *pool, worker_thread_pool_callback callback, int num_threads, int queue_size) {
	if (!callback) {
		return 1;
	}
	if (num_threads < 1) {
		return 1;
	}
	if (queue_size < 1) {
		return 1;
	}
	pool->callback = callback;
	pool->num_threads = num_threads;
	pool->queue_size = queue_size;

	pool->tasks = calloc(queue_size, sizeof(worker_thread_pool_task));
	pool->next_task = 0;
	pool->tasks_len = 0;

	pool->contexts = calloc(num_threads, sizeof(worker_thread_pool_context));
	pool->threads = calloc(num_threads, sizeof(pthread_t));
	for (int i = 0; i < num_threads; i++) {
		worker_thread_pool_context *context = &pool->contexts[i];
		context->pool = pool;
		context->running = 1;
		context->id = i;
		if (pthread_create(&pool->threads[i], NULL, worker_thread_pool_pthread_callback, &pool->contexts[i])) {
			for (int j = 0; j < i; j++) {
				pool->contexts[j].running = 0;
			}
			for (int j = 0; j < i; j++) {
				// TODO signal first?
				void *result;
				if (pthread_join(pool->threads[j], &result)) {
					// TODO non-0 means error, handling pthread_join failure
				}
			}
			free(pool->tasks);
			free(pool->contexts);
			free(pool->threads);
			return 1;
		}
	}
	return 0;
}

int worker_thread_pool_stop_and_join(worker_thread_pool *pool) {
	for (int i = 0; i < pool->num_threads; i++) {
		pool->contexts[i].running = 0;
	}
	for (int i = 0; i < pool->num_threads; i++) {
		// TODO signal first?
		void *result;
		if (pthread_join(pool->threads[i], &result)) {
			// TODO non-0 means error, handling pthread_join failure
		}
	}
	free(pool->tasks);
	free(pool->contexts);
	free(pool->threads);
	return 0;
}

int worker_thread_pool_enqueue(worker_thread_pool *pool, void *data) {
	/*
	TODO add new queue to task

	lock
	if queue is full, unlock and return failure
	write data to next index
	size++
	unlock
	return success
	*/
}