#include <errno.h>
#include <stdlib.h>

#include "log.h"
#include "worker_thread_pool.h"

void *worker_thread_pool_pthread_callback(void *data) {
	worker_thread_pool_context *context = data;
	log_trace("worker_thread_pool_pthread_callback start, thread id %i\n", context->id);
	pthread_mutex_lock(&context->pool->tasks_mutex);
	while (context->running) {
		struct timespec timeout;
		timespec_get(&timeout, TIME_UTC);
		// TODO timeout should be a constant
		timeout.tv_sec += 1;
		int cond_error = pthread_cond_timedwait(&context->pool->tasks_condition, &context->pool->tasks_mutex, &timeout);
		if (cond_error == ETIMEDOUT) {
			log_trace("TODO JEFF worker_thread_pool_pthread_callback, thread id %i, pthread_cond_timedwait timed out\n", context->id);
		} else if (cond_error) {
			log_trace("TODO JEFF worker_thread_pool_pthread_callback, thread id %i, pthread_cond_timedwait error %i\n", context->id,
					  cond_error);
		} else {
			log_trace("TODO JEFF worker_thread_pool_pthread_callback, thread id %i, pthread_cond_timedwait signalled\n", context->id);
		}
		if (!context->running) {
			break;
		}

		// TODO if work is in the queue dequeue an element and execute it
	}
	pthread_mutex_unlock(&context->pool->tasks_mutex);
	log_trace("worker_thread_pool_pthread_callback done, thread id %i\n", context->id);
	return NULL;
}

int worker_thread_pool_init(worker_thread_pool *pool, worker_thread_pool_callback callback, int num_threads, int queue_size) {
	log_trace("worker_thread_pool_init, num_threads=%i, queue_size=%i\n", num_threads, queue_size);
	if (!callback) {
		log_error("worker_thread_pool_init failed, no callback\n");
		return 1;
	}
	if (num_threads < 1) {
		log_error("worker_thread_pool_init failed, num_threads must be positive\n");
		return 1;
	}
	if (queue_size < 1) {
		log_error("worker_thread_pool_init failed, queue_size must be positive\n");
		return 1;
	}
	pool->callback = callback;
	pool->num_threads = num_threads;
	pool->queue_size = queue_size;

	if (pthread_mutex_init(&pool->tasks_mutex, NULL)) {
		log_error("worker_thread_pool_init failed, pthread_mutex_init failed on task mutex\n");
		return 1;
	}
	if (pthread_cond_init(&pool->tasks_condition, NULL)) {
		log_error("worker_thread_pool_init failed, pthread_cond_init failed on task mutex\n");
		if (pthread_mutex_destroy(&pool->tasks_mutex)) {
			log_error("worker_thread_pool_init failed, pthread_mutex_destroy failed on task mutex\n");
		}
		return 1;
	}
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
			log_error("worker_thread_pool_init failed, pthread_create failed on thread %i\n", i);
			for (int j = 0; j < i; j++) {
				pool->contexts[j].running = 0;
			}
			for (int j = 0; j < i; j++) {
				pthread_cond_broadcast(&pool->tasks_condition);
				void *result;
				if (pthread_join(pool->threads[j], &result)) {
					log_error("worker_thread_pool_init failed, pthread_join failed during shutdown on thread %i\n", j);
				}
			}
			if (pthread_mutex_destroy(&pool->tasks_mutex)) {
				log_error("worker_thread_pool_init failed, pthread_mutex_destroy failed on task mutex\n");
			}
			if (pthread_cond_destroy(&pool->tasks_condition)) {
				log_error("worker_thread_pool_init failed, pthread_cond_destroy failed on task condition\n");
			}
			free(pool->tasks);
			free(pool->contexts);
			free(pool->threads);
			return 1;
		}
	}
	log_trace("worker_thread_pool_init success\n");
	return 0;
}

int worker_thread_pool_destroy(worker_thread_pool *pool) {
	log_trace("worker_thread_pool_destroy start\n");
	for (int i = 0; i < pool->num_threads; i++) {
		pool->contexts[i].running = 0;
	}
	for (int i = 0; i < pool->num_threads; i++) {
		pthread_cond_broadcast(&pool->tasks_condition);
		void *result;
		if (pthread_join(pool->threads[i], &result)) {
			log_error("worker_thread_pool_destroy failed, pthread_join failed during shutdown on thread %i\n", i);
		}
	}
	if (pthread_mutex_destroy(&pool->tasks_mutex)) {
		log_error("worker_thread_pool_destroy failed, pthread_mutex_destroy failed on task mutex\n");
	}
	if (pthread_cond_destroy(&pool->tasks_condition)) {
		log_error("worker_thread_pool_destroy failed, pthread_cond_destroy failed on task condition\n");
	}
	free(pool->tasks);
	free(pool->contexts);
	free(pool->threads);
	log_trace("worker_thread_pool_destroy success\n");
	return 0;
}

int worker_thread_pool_enqueue(worker_thread_pool *pool, void *data, int *thread_result) {
	log_trace("worker_thread_pool_enqueue start\n");
	// TODO JEFF this mutex doesn't grab, the worker threads got the mutex but this can't?
	pthread_mutex_lock(&pool->tasks_mutex);

	if (pool->tasks_len == pool->queue_size) {
		log_trace("worker_thread_pool_enqueue failed, queue is full\n");
		pthread_mutex_unlock(&pool->tasks_mutex);
		return 1;
	}

	int new_task_index = (pool->next_task + pool->tasks_len) % pool->queue_size;
	pool->tasks_len++;
	log_trace("worker_thread_pool_enqueue, new task index %i, new task queue length %i\n", new_task_index, pool->tasks_len);

	worker_thread_pool_task *task = &pool->tasks[new_task_index];
	task->data = data;
	task->result = thread_result;
	task->done = 0;

	while (!task->done) {
		struct timespec timeout;
		timespec_get(&timeout, TIME_UTC);
		// TODO timeout should be a constant
		timeout.tv_sec += 1;
		int cond_error = pthread_cond_timedwait(&pool->tasks_condition, &pool->tasks_mutex, &timeout);
		if (cond_error == ETIMEDOUT) {
			log_trace("TODO JEFF worker_thread_pool_enqueue, pthread_cond_timedwait timed out\n");
		} else if (cond_error) {
			log_trace("TODO JEFF worker_thread_pool_enqueue, pthread_cond_timedwait error %i\n", cond_error);
		} else {
			log_trace("TODO JEFF worker_thread_pool_enqueue, pthread_cond_timedwait signalled\n");
		}
		// TODO if pool has been destroyed abort
	}

	pthread_mutex_unlock(&pool->tasks_mutex);
	log_trace("worker_thread_pool_enqueue success\n");
	return 0;
}