#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../shared/worker_thread_pool.h"

typedef struct {
	int a;
	int b;
	int result;
	// optional, some tests don't need it
	// 0 means don't sleep
	uint64_t sleep;
	// optional, some tests don't need it
	sem_t *semaphore;
} task_data;

int callback(int id, void *data) {
	task_data *d = data;
	printf("task executing on worker %i, a=%i, b=%i\n", id, d->a, d->b);
	fflush(stdout);
	if (d->sleep) {
		usleep(d->sleep);
	}
	d->result = d->a + d->b;
	if (d->semaphore) {
		sem_post(d->semaphore);
	}
	return 0;
}

/*
launch each task and wait for completion
only one task should ever be running at once
*/
void test_wait_each(worker_thread_pool *pool, int expected_len, task_data *expected) {
	for (int i = 0; i < expected_len; i++) {
		task_data d;
		memset(&d, 0, sizeof(task_data));
		d.a = expected[i].a;
		d.b = expected[i].b;
		int task_result;
		int enqueue_result = worker_thread_pool_enqueue(pool, callback, &d, &task_result, -1);
		fflush(stdout);
		assert(enqueue_result == WORKER_THREAD_POOL_SUCCESS);
		assert(task_result == 0);
		assert(d.result == expected[i].result);
	}
}

/*
launch each task without waiting for completion
we should get a bunch of errors about failing because the queue is full
for all the other ones we should get the expected results
*/
void test_enqueue_failures(worker_thread_pool *pool, int expected_len, task_data *expected) {
	int success_count = 0;
	int queue_full_error_count = 0;
	int *tests_that_should_finish = malloc(sizeof(int) * expected_len);
	memset(tests_that_should_finish, 0, sizeof(int) * expected_len);
	task_data *td = malloc(sizeof(task_data) * expected_len);
	memset(td, 0, sizeof(task_data) * expected_len);
	sem_t semaphore;
	sem_init(&semaphore, 0, 0);
	for (int i = 0; i < expected_len; i++) {
		td[i].a = expected[i].a;
		td[i].b = expected[i].b;
		td[i].semaphore = &semaphore;
		int task_result;
		int enqueue_result = worker_thread_pool_enqueue(pool, callback, &td[i], &task_result, 0);
		switch (enqueue_result) {
		case WORKER_THREAD_POOL_SUCCESS:
			success_count++;
			tests_that_should_finish[i] = 1;
			break;
		case WORKER_THREAD_POOL_ERROR_QUEUE_FULL:
			queue_full_error_count++;
			break;
		}
	}
	printf("success_count = %i\n", success_count);
	printf("queue_full_error_count = %i\n", queue_full_error_count);
	fflush(stdout);
	assert(success_count + queue_full_error_count == expected_len);
	for (int i = 0; i < success_count; i++) {
		sem_wait(&semaphore);
	}
	for (int i = 0; i < expected_len; i++) {
		if (tests_that_should_finish[i]) {
			assert(td[i].result == expected[i].result);
		}
	}
	sem_destroy(&semaphore);
	free(td);
	free(tests_that_should_finish);
}

/*
perform tasks that should all time out
*/
void test_enqueue_task_times_out(worker_thread_pool *pool) {
	sem_t semaphore;
	sem_init(&semaphore, 0, 0);
	task_data d;
	memset(&d, 0, sizeof(task_data));
	d.a = rand();
	d.b = rand();
	// 1 second in microseconds
	d.sleep = 1000000ull;
	d.semaphore = &semaphore;
	int expected = d.a + d.b;
	int task_result = 42;
	// going to be comparing times to make sure it really waited
	struct timespec ts1;
	timespec_get(&ts1, TIME_UTC);
	// timeout is 0.5 seconds in nanoseconds
	int enqueue_result = worker_thread_pool_enqueue(pool, callback, &d, &task_result, 500000000ull);
	// assert that we waited
	struct timespec ts2;
	timespec_get(&ts2, TIME_UTC);
	int64_t wait_time = (ts2.tv_sec - ts1.tv_sec) * 1000000000ll + (ts2.tv_nsec - ts1.tv_nsec);
	assert(wait_time > 500000000ll);
	assert(enqueue_result == WORKER_THREAD_POOL_ERROR_TIMEOUT);
	// i.e. it's not been set to 0 because the task hasn't completed yet
	assert(task_result == 42);
	// still 0 because the callback hasn't fired
	assert(d.result == 0);
	fflush(stdout);
	sem_wait(&semaphore);
	fflush(stdout);
	// assert that after the full delay the task actually copmletes
	struct timespec ts3;
	timespec_get(&ts3, TIME_UTC);
	int64_t complete_time = (ts3.tv_sec - ts1.tv_sec) * 1000000000ll + (ts3.tv_nsec - ts1.tv_nsec);
	assert(complete_time > 1000000000ll);
	// i.e. it's still not been set to 0 because the task knows it timed out and can't touch this pointer
	assert(task_result == 42);
	// callback did execute though, so it set the result
	assert(d.result == expected);
	sem_destroy(&semaphore);
}

int main(int argc, char **argv) {
	worker_thread_pool pool;
	if (worker_thread_pool_init(&pool, 2, 10)) {
		return 1;
	}

	srand(time(NULL));

	const int expected_len = 10000;
	task_data *expected = malloc(sizeof(task_data) * expected_len);
	memset(expected, 0, sizeof(task_data) * expected_len);
	for (int i = 0; i < expected_len; i++) {
		expected[i].a = rand();
		expected[i].b = rand();
		expected[i].result = expected[i].a + expected[i].b;
	}

	printf("waiting on each result in sequence\n");
	test_wait_each(&pool, expected_len, expected);
	printf("\n\n");
	fflush(stdout);

	printf("not waiting, just execute as fast as possible\n");
	test_enqueue_failures(&pool, expected_len, expected);
	printf("\n\n");
	fflush(stdout);

	printf("task times out\n");
	test_enqueue_task_times_out(&pool);
	printf("\n\n");
	fflush(stdout);

	free(expected);
	worker_thread_pool_dealloc(&pool);
	return 0;
}