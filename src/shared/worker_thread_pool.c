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
		struct timespec timeout;
		timespec_get(&timeout, TIME_UTC);
		// TODO timeout should be a constant
		timeout.tv_sec += 1;
		int semaphore_error = sem_timedwait(&context->pool->tasks_semaphore, &timeout);
		switch (semaphore_error) {
		// succsss, we were signalled
		case 0:
		// timed out
		case ETIMEDOUT:
			break;
		default:
			log_error("worker_thread_pool_pthread_callback, thread id %i, pthread_cond_timedwait error %i\n", context->id, semaphore_error);
		}
		if (!context->running) {
			break;
		}

		// if work is in the queue dequeue an element and execute it
		pthread_mutex_lock(&context->pool->tasks_mutex);
		if (context->pool->task_pending_len > 0) {
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
			int callback_result = context->pool->callback(context->id, task->data);
			if (task->result) {
				*task->result = callback_result;
			}

			// back to working with the task queue and pool
			pthread_mutex_lock(&context->pool->tasks_mutex);

			// task complete, return the pool
			task->next = context->pool->task_pool_first;
			context->pool->task_pool_first = task;
			context->pool->task_pool_len++;
			log_trace("worker_thread_pool_pthread_callback returned completed task to pool, new pool size %i\n",
					  context->pool->task_pool_len);

			// signal completion
			sem_post(&task->semaphore);

			// really done
			pthread_mutex_unlock(&context->pool->tasks_mutex);
		} else {
			// nothing to do
			pthread_mutex_unlock(&context->pool->tasks_mutex);
		}
	}
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
	memset(pool, 0, sizeof(worker_thread_pool));
	pool->callback = callback;
	pool->num_threads = num_threads;
	pool->max_queue_size = queue_size;

	if (pthread_mutex_init(&pool->tasks_mutex, NULL)) {
		log_error("worker_thread_pool_init failed, pthread_mutex_init failed on task mutex\n");
		worker_thread_pool_destroy(pool);
		return 1;
	}
	pool->tasks_mutex_is_init = 1;
	if (sem_init(&pool->tasks_semaphore, 0, 0)) {
		log_error("worker_thread_pool_init failed, sem_init failed on task semaphore %i\n", errno);
		worker_thread_pool_destroy(pool);
		return 1;
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
			worker_thread_pool_destroy(pool);
			return 1;
		}
		context->thread_is_init = 1;
	}
	log_trace("worker_thread_pool_init success\n");
	return 0;
}

int worker_thread_pool_destroy(worker_thread_pool *pool) {
	log_trace("worker_thread_pool_destroy start\n");
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
				log_error("worker_thread_pool_destroy failed, pthread_join failed during shutdown on thread %i\n", i);
			}
			pool->threads[i].thread_is_init = 0;
		}
		free(pool->threads);
		pool->threads = NULL;
	}
	if (pool->tasks_semaphore_is_init && sem_destroy(&pool->tasks_semaphore)) {
		log_error("worker_thread_pool_destroy failed, sem_destroy failed on task semaphore\n");
	}
	pool->tasks_semaphore_is_init = 0;
	if (pool->tasks_mutex_is_init && pthread_mutex_destroy(&pool->tasks_mutex)) {
		log_error("worker_thread_pool_destroy failed, pthread_mutex_destroy failed on task mutex\n");
	}
	pool->tasks_mutex_is_init = 0;
	for (worker_thread_pool_task *t = pool->task_pool_first; t;) {
		worker_thread_pool_task *t2 = t;
		t = t->next;
		if (sem_destroy(&t2->semaphore)) {
			log_error("worker_thread_pool_destroy failed, sem_destroy failed on specific task in pool semaphore\n");
		}
		free(t2);
	}
	pool->task_pool_first = NULL;
	pool->task_pool_len = 0;
	for (worker_thread_pool_task *t = pool->task_pending_first; t;) {
		worker_thread_pool_task *t2 = t;
		t = t->next;
		if (sem_destroy(&t2->semaphore)) {
			log_error("worker_thread_pool_destroy failed, sem_destroy failed on specific task in pending semaphore\n");
		}
		free(t2);
	}
	pool->task_pending_first = NULL;
	pool->task_pending_last = NULL;
	pool->task_pending_len = 0;
	log_trace("worker_thread_pool_destroy success\n");
	return 0;
}

int worker_thread_pool_enqueue(worker_thread_pool *pool, void *data, int *thread_result, worker_thread_pool_enqueue_mode mode) {
	log_trace("worker_thread_pool_enqueue start\n");
	pthread_mutex_lock(&pool->tasks_mutex);

	// don't let us queue tasks indefinitely
	/*
	TODO handle blocking or not blocking for enqueue
	if mode == WORKER_THREAD_POOL_ENQUEUE_MODE_NO_WAIT:
		fail if queue is full
	else:
		wait up until timeout for queue to not be full
		fail if queue is still full after timeout
	*/
	if (pool->task_pending_len >= pool->max_queue_size) {
		log_error("worker_thread_pool_enqueue failed, queue is full\n");
		pthread_mutex_unlock(&pool->tasks_mutex);
		return 1;
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
			return 1;
		}
	} else {
		task = pool->task_pool_first;
		pool->task_pool_first = pool->task_pool_first->next;
		task->next = NULL;
		pool->task_pool_len--;
		log_trace("worker_thread_pool_enqueue got task from pool, new pool size %i\n", pool->task_pool_len);
	}

	// data for this task specifically
	task->data = data;
	task->result = thread_result;

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
	/*
	TODO handle blocking or not blocking for completion
	if mode == WORKER_THREAD_POOL_ENQUEUE_MODE_WAIT_COMPLETE:
		wait for task to be complete
	else:
		return immediately
	*/
	sem_wait(&task->semaphore);

	log_trace("worker_thread_pool_enqueue success\n");
	return 0;
}