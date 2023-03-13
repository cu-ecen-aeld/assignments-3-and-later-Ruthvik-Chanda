#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    int error_state;

    if(NULL == thread_param)
    {
    	return false;
    }
    
    struct thread_data *thread_func_args = (struct thread_data *)thread_param;

    error_state = usleep(thread_func_args->wait_to_obtain_ms*1000);
    if(-1 == error_state)
    {
        ERROR_LOG("Error !! Usleep Failed with Error Number : %d", errno);
    	return thread_param;
    }


    error_state = pthread_mutex_lock(thread_func_args->mutex);
    if(-1 == error_state)
    {
    	ERROR_LOG("Error !! pthread_mutex_lock Failed with Error Number : %d", errno);
    	return thread_param;
    }


    error_state = usleep(thread_func_args->wait_to_release_ms*1000);
    if(-1 == error_state)
    {
    	ERROR_LOG("Error !! Usleep Failed with Error Number : %d", errno);
    	return thread_param;
    }

    error_state = pthread_mutex_unlock(thread_func_args->mutex);
    if(-1 == error_state)
    {
    	ERROR_LOG("Error !! pthread_mutex_unlock Failed with Error Number : %d", errno);
    	return thread_param;
    }

    if(0 == error_state)
    {
    	thread_func_args->thread_complete_success=true;
    }
    else
    {
    	thread_func_args->thread_complete_success=false;
    }
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data *thread_parameters = (struct thread_data*)malloc(sizeof(struct thread_data));
    int error_state = true; 
    
    if(NULL == thread_parameters)
    {
    	ERROR_LOG("Error !! Malloc failed : %d", errno);
     	return false;
    }

    thread_parameters->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_parameters->mutex = mutex;
    thread_parameters->wait_to_release_ms = wait_to_release_ms;
    thread_parameters->thread = thread;
    
    error_state = pthread_create(thread,NULL,threadfunc, thread_parameters);

    if(error_state)
    {
    	ERROR_LOG("Error !! Thread create failed : %d", errno);
    	return false;
    }
    return true;
}

