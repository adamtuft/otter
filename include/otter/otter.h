#if !defined(OTTER_H)
#define OTTER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>

// getrusage
#include <sys/time.h>
#include <sys/resource.h>

#include "otter/otter-ompt-header.h"

#include "otter/otter-common.h"
#include "otter/otter-entry.h"
#include "otter/stack.h"
#include "otter/trace.h"

/* A naming convention was chosen for the callbacks so that each callback is 
   named after the event in ompt.h which it handles. The struct passed to the
   tool by ompt-core has a member for each OMP event. This macro attaches
   a callback to its corresponding field in that struct.
*/
#define include_callback(callbacks, event) callbacks->on_##event = on_##event

#define STR_EQUAL(a, b) (!strcmp(a,b))

/* Include the function prototypes for the callbacks this tool implements */
#define implements_callback_thread_begin   
#define implements_callback_thread_end     
#define implements_callback_parallel_begin 
#define implements_callback_parallel_end   
#define implements_callback_task_create    
#define implements_callback_task_schedule  
#define implements_callback_implicit_task
#define implements_callback_work
#define implements_callback_sync_region
#define implements_callback_master
#include "otter/ompt-callback-prototypes.h"

// /* Used as an array index to keep track of unique ids for different entities */
// typedef enum unique_id_type_t {
//     id_timestamp        ,
//     id_parallel         ,
//     id_thread           ,
//     id_task             ,
//     NUM_ID_TYPES
// } unique_id_type_t;
// #define get_unique_parallel_id() get_unique_id(id_parallel)
// #define get_unique_thread_id()   get_unique_id(id_thread)
// #define get_unique_task_id()     get_unique_id(id_task)
// #define get_dummy_time()         get_unique_id(id_timestamp)

// unique_id_t get_unique_id(unique_id_type_t id_type);

#endif // OTTER_H
