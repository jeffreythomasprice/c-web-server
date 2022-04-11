#include "worker_thread_pool.h"

void *worker_thread_pool_pthread_callback(void *data) {
	worker_thread_pool_context *context = data;
	/*
	TODO implement worker thread

	while (not stopped) {
		wait for signal
		if stopped break
		if job available take one and do it, call callback with result
	}
	*/
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
		pool->contexts[i].pool = pool;
		pool->contexts[i].id = i;
		if (pthread_create(&pool->threads[i], NULL, worker_thread_pool_pthread_callback, &pool->contexts[i])) {
			// TODO close all other threads and join
			return 1;
		}
	}
	return 0;
}

int worker_thread_pool_stop_and_join(worker_thread_pool *pool) {
	/*
	TODO signal all threads that they should exit, join all threads, dealloc everything
	*/
}

int worker_thread_pool_enqueue(worker_thread_pool *pool, void *data) {
	/*
	TODO add new queue to task
	*/
}