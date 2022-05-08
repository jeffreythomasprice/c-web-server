#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "worker_thread_pool.h"

void *worker_thread_pool_pthread_callback(void *data) {
	worker_thread_pool_context *context = data;
	log_trace("worker_thread_pool_pthread_callback start, thread id %i\n", context->id);
	while (context->running) {
		// if there are no tasks to do sleep until signalled
		pthread_mutex_lock(&context->pool->tasks_mutex);
		if (context->pool->task_pending_len == 0) {
			pthread_mutex_unlock(&context->pool->tasks_mutex);
			struct timespec timeout;
			timespec_get(&timeout, TIME_UTC);
			timeout.tv_sec += 5;
			int semaphore_error;
			if (sem_timedwait(&context->pool->tasks_semaphore, &timeout)) {
				semaphore_error = errno;
			}
			switch (semaphore_error) {
			// succsss, we were signalled
			case 0:
			// timed out
			case ETIMEDOUT:
				break;
			default:
				log_error("worker_thread_pool_pthread_callback, thread id %i, pthread_cond_timedwait error %i\n", context->id,
						  semaphore_error);
			}
			// we're trying to shut down so just abort
			if (!context->running) {
				break;
			}
			pthread_mutex_lock(&context->pool->tasks_mutex);
		}

		// if after waiting there is still no work to do just keep going, some other thread got there first
		if (context->pool->task_pending_len == 0) {
			pthread_mutex_unlock(&context->pool->tasks_mutex);
			continue;
		}

		// get the next task from the queue
		worker_thread_pool_task *task = context->pool->task_pending_first;
		context->pool->task_pending_first = context->pool->task_pending_first->next;
		if (!context->pool->task_pending_first) {
			context->pool->task_pending_last = NULL;
		}
		context->pool->task_pending_len--;
		log_trace("worker_thread_pool_pthread_callback dequeued task, new task len %i\n", context->pool->task_pending_len);

		// intentionally not blocking the actual callback, that might take a while
		pthread_mutex_unlock(&context->pool->tasks_mutex);

		// actually do the work
		int callback_result = task->callback(context->id, task->data);
		if (task->result) {
			*task->result = callback_result;
		}

		// back to working with the task queue and pool
		pthread_mutex_lock(&context->pool->tasks_mutex);

		// if we've timed out then we're responsible for adding this back to the pool
		// if we didn't time out then we should signal our main thread that we're done
		if (task->timedout) {
			task->next = context->pool->task_pool_first;
			context->pool->task_pool_first = task;
			context->pool->task_pool_len++;
			log_trace("worker_thread_pool_pthread_callback task timed out, returned completed task to pool, new pool size %i\n",
					  context->pool->task_pool_len);
		} else {
			log_trace("worker_thread_pool_pthread_callback task completed successfully, signalling\n");
			task->completed = 1;
			sem_post(&task->semaphore);
		}

		pthread_mutex_unlock(&context->pool->tasks_mutex);
	}
	log_trace("worker_thread_pool_pthread_callback done, thread id %i\n", context->id);
	return NULL;
}

int worker_thread_pool_init(worker_thread_pool *pool, int num_threads, int queue_size) {
	log_trace("worker_thread_pool_init, num_threads=%i, queue_size=%i\n", num_threads, queue_size);
	if (num_threads < 1) {
		log_error("worker_thread_pool_init failed, num_threads must be positive\n");
		return WORKER_THREAD_POOL_ERROR;
	}
	if (queue_size < 1) {
		log_error("worker_thread_pool_init failed, queue_size must be positive\n");
		return WORKER_THREAD_POOL_ERROR;
	}
	memset(pool, 0, sizeof(worker_thread_pool));
	pool->num_threads = num_threads;
	pool->max_queue_size = queue_size;

	if (pthread_mutex_init(&pool->tasks_mutex, NULL)) {
		log_error("worker_thread_pool_init failed, pthread_mutex_init failed on task mutex\n");
		worker_thread_pool_dealloc(pool);
		return WORKER_THREAD_POOL_ERROR;
	}
	pool->tasks_mutex_is_init = 1;
	if (sem_init(&pool->tasks_semaphore, 0, 0)) {
		log_error("worker_thread_pool_init failed, sem_init failed on task semaphore %i\n", errno);
		worker_thread_pool_dealloc(pool);
		return WORKER_THREAD_POOL_ERROR;
	}
	pool->tasks_semaphore_is_init = 1;

	pool->threads = malloc(num_threads * sizeof(worker_thread_pool_context));
	memset(pool->threads, 0, num_threads * sizeof(worker_thread_pool_context));
	for (int i = 0; i < num_threads; i++) {
		worker_thread_pool_context *context = &pool->threads[i];
		context->pool = pool;
		context->running = 1;
		context->id = i;
		if (pthread_create(&context->thread, NULL, worker_thread_pool_pthread_callback, context)) {
			log_error("worker_thread_pool_init failed, pthread_create failed on thread %i\n", i);
			worker_thread_pool_dealloc(pool);
			return WORKER_THREAD_POOL_ERROR;
		}
		context->thread_is_init = 1;
	}
	log_trace("worker_thread_pool_init success\n");
	return WORKER_THREAD_POOL_SUCCESS;
}

int worker_thread_pool_dealloc(worker_thread_pool *pool) {
	log_trace("worker_thread_pool_dealloc start\n");
	// all threads should be exiting
	if (pool->threads) {
		for (int i = 0; i < pool->num_threads; i++) {
			pool->threads[i].running = 0;
		}
		// wake all threads up
		for (int i = 0; i < pool->num_threads; i++) {
			sem_post(&pool->tasks_semaphore);
		}
		// wait for all threads to die
		for (int i = 0; i < pool->num_threads; i++) {
			void *result;
			if (pool->threads[i].thread_is_init && pthread_join(pool->threads[i].thread, &result)) {
				log_error("worker_thread_pool_dealloc failed, pthread_join failed during shutdown on thread %i\n", i);
			}
			pool->threads[i].thread_is_init = 0;
		}
		free(pool->threads);
		pool->threads = NULL;
	}
	if (pool->tasks_semaphore_is_init && sem_destroy(&pool->tasks_semaphore)) {
		log_error("worker_thread_pool_dealloc failed, sem_destroy failed on task semaphore\n");
	}
	pool->tasks_semaphore_is_init = 0;
	if (pool->tasks_mutex_is_init && pthread_mutex_destroy(&pool->tasks_mutex)) {
		log_error("worker_thread_pool_dealloc failed, pthread_mutex_destroy failed on task mutex\n");
	}
	pool->tasks_mutex_is_init = 0;
	for (worker_thread_pool_task *t = pool->task_pool_first; t;) {
		worker_thread_pool_task *t2 = t;
		t = t->next;
		if (sem_destroy(&t2->semaphore)) {
			log_error("worker_thread_pool_dealloc failed, sem_destroy failed on specific task in pool semaphore\n");
		}
		free(t2);
	}
	pool->task_pool_first = NULL;
	pool->task_pool_len = 0;
	for (worker_thread_pool_task *t = pool->task_pending_first; t;) {
		worker_thread_pool_task *t2 = t;
		t = t->next;
		if (sem_destroy(&t2->semaphore)) {
			log_error("worker_thread_pool_dealloc failed, sem_destroy failed on specific task in pending semaphore\n");
		}
		free(t2);
	}
	pool->task_pending_first = NULL;
	pool->task_pending_last = NULL;
	pool->task_pending_len = 0;
	log_trace("worker_thread_pool_dealloc success\n");
	return WORKER_THREAD_POOL_SUCCESS;
}

int worker_thread_pool_enqueue(worker_thread_pool *pool, worker_thread_pool_callback callback, void *data, int *callback_result,
							   uint64_t timeout) {
	log_trace("worker_thread_pool_enqueue start\n");
	if (!callback) {
		log_error("worker_thread_pool_enqueue failed, callback is required\n");
		return WORKER_THREAD_POOL_ERROR;
	}
	pthread_mutex_lock(&pool->tasks_mutex);

	// add to the queue, fail if queue is full
	if (pool->task_pending_len >= pool->max_queue_size) {
		log_error("worker_thread_pool_enqueue failed, queue is full\n");
		pthread_mutex_unlock(&pool->tasks_mutex);
		return WORKER_THREAD_POOL_ERROR_QUEUE_FULL;
	}

	// either we have a task already allocated on the pool, or we need to make a new one
	worker_thread_pool_task *task;
	if (pool->task_pool_len == 0) {
		log_trace("worker_thread_pool_enqueue had empty pool, allocating new task\n");
		task = malloc(sizeof(worker_thread_pool_task));
		task->next = NULL;
		task->result = NULL;
		if (sem_init(&task->semaphore, 0, 0)) {
			log_error("worker_thread_pool_enqueue error, sem_init failed\n");
			free(task);
			pthread_mutex_unlock(&pool->tasks_mutex);
			return WORKER_THREAD_POOL_ERROR;
		}
	} else {
		task = pool->task_pool_first;
		pool->task_pool_first = pool->task_pool_first->next;
		task->next = NULL;
		pool->task_pool_len--;
		log_trace("worker_thread_pool_enqueue got task from pool, new pool size %i\n", pool->task_pool_len);
	}

	// data for this task specifically
	task->callback = callback;
	task->data = data;
	task->result = callback_result;
	task->timedout = 0;
	task->completed = 0;

	// stick the new task on the end of the queue
	if (pool->task_pending_len > 0) {
		pool->task_pending_last->next = task;
		pool->task_pending_last = task;
	} else {
		pool->task_pending_first = task;
		pool->task_pending_last = task;
	}
	pool->task_pending_len++;
	log_trace("worker_thread_pool_enqueue new task queue length %i\n", pool->task_pending_len);

	// no longer blocking, and wake up a thread
	pthread_mutex_unlock(&pool->tasks_mutex);
	sem_post(&pool->tasks_semaphore);

	// wait until we have a result
	if (timeout == -1) {
		log_trace("worker_thread_pool_enqueue waiting forever\n");
		sem_wait(&task->semaphore);
	} else if (timeout == 0) {
		log_trace("worker_thread_pool_enqueue not waiting\n");
	} else {
		log_trace("worker_thread_pool_enqueue waiting %lu\n", timeout);
		struct timespec ts_timeout;
		timespec_get(&ts_timeout, TIME_UTC);
		ts_timeout.tv_nsec += timeout;
		ts_timeout.tv_sec += ts_timeout.tv_nsec / 1000000000ull;
		ts_timeout.tv_nsec = ts_timeout.tv_nsec % 1000000000ull;
		int wait_error = 0;
		if (sem_timedwait(&task->semaphore, &ts_timeout)) {
			wait_error = errno;
		}
		pthread_mutex_lock(&pool->tasks_mutex);
		int exit_with_error;
		int add_task_back_to_pool;
		if (task->completed) {
			log_trace("worker_thread_pool_enqueue is done waiting, but task is marked as completed so ignoring timeout\n");
			exit_with_error = 0;
			add_task_back_to_pool = 0;
		} else if (wait_error == 0) {
			exit_with_error = 0;
			add_task_back_to_pool = 1;
		} else if (wait_error == ETIMEDOUT) {
			// result pointer may now be invalid, the caller moved on
			task->result = NULL;
			task->timedout = 1;
			log_error("worker_thread_pool_enqueue failed, task timed out\n");
			exit_with_error = WORKER_THREAD_POOL_ERROR_TIMEOUT;
			add_task_back_to_pool = 0;
		} else {
			log_error("worker_thread_pool_enqueue failed, sem_timedwait failed %i\n", wait_error);
			exit_with_error = WORKER_THREAD_POOL_ERROR;
			add_task_back_to_pool = 1;
		}
		if (add_task_back_to_pool) {
			task->next = pool->task_pool_first;
			pool->task_pool_first = task;
			pool->task_pool_len++;
			log_trace("worker_thread_pool_enqueue, returned timed out task to pool, new pool size %i\n", pool->task_pool_len);
		}
		pthread_mutex_unlock(&pool->tasks_mutex);
		if (exit_with_error) {
			return exit_with_error;
		}
	}

	log_trace("worker_thread_pool_enqueue success\n");
	return WORKER_THREAD_POOL_SUCCESS;
}