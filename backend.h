#ifndef BACKEND_H
#define BACKEND_H

#include "ds.h"
#include <unistd.h>
#include <pthread.h>

// --- Global Pipes ---
extern int pipe_cust_mgr[2];   // Customer -> Manager
// extern int pipe_mgr_cook[2];   // REMOVED: Replaced by Priority Queue
extern SafeQueue order_queue;    // Manager -> Cook (Thread Safe Queue)
extern int pipe_cook_waiter[2];// Cook -> Waiter

// --- Shared Resources ---
extern MenuList menu_list;
extern double Regular_Sales;
extern double Regular_Waiter_Sales;
extern int Total_Orders;
extern pthread_mutex_t sales_mutex;

// --- Thread Functions ---
void backend_init();
void* manager_runtime(void* arg);
void* cook_runtime(void* arg);
void* waiter_runtime(void* arg);
void start_customer_order(Order order); // Helper to write to pipe
void backend_cleanup();

#endif
