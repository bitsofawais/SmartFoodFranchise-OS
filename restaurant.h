#ifndef RESTAURANT_H
#define RESTAURANT_H

#include <pthread.h>

#define MAX_ITEMS 5

typedef struct {
    char name[30];
    int price;
    int prep_time;
} MenuItem;

typedef struct {
    int order_id;
    char customer_name[30];
    int item_count;
    int item_index[MAX_ITEMS];
    int quantity[MAX_ITEMS];
    int total_bill;
} Order;

/* Pipes */
extern int cust_to_mgr[2];
extern int mgr_to_cook[2];
extern int cook_to_waiter[2];

/* Mutex */
extern pthread_mutex_t sales_mutex;
extern pthread_mutex_t file_mutex;

/* Sales */
extern int Regular_Sales;
extern int Regular_Waiter_Sales;

/* Functions */
void* customer_thread(void* arg);
void* manager_thread(void* arg);
void* cook_thread(void* arg);
void* waiter_thread(void* arg);

/* Backend call from GUI */
void start_customer_thread();

#endif
