#include "restaurant.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* Global pipes */
int cust_to_mgr[2];
int mgr_to_cook[2];
int cook_to_waiter[2];

/* Mutexes */
pthread_mutex_t sales_mutex;
pthread_mutex_t file_mutex;

/* Sales */
int Regular_Sales = 0;
int Regular_Waiter_Sales = 0;

/* Menu items */
MenuItem menu[MAX_ITEMS] = {
    {"Burger", 500, 5},
    {"Pizza", 1200, 8},
    {"Fries", 300, 3},
    {"Sandwich", 400, 4},
    {"Drink", 150, 1}
};

/* Customer Thread */
void* customer_thread(void* arg) {
    Order order;
    printf("\nCustomer Name: ");
    scanf("%s", order.customer_name);

    order.item_count = 2;
    order.item_index[0] = 0; order.quantity[0] = 2; // Burger
    order.item_index[1] = 2; order.quantity[1] = 1; // Fries

    write(cust_to_mgr[1], &order, sizeof(Order));
    printf("Customer placed order and is waiting...\n");
    sleep(5);
    printf("Customer received order and exited.\n");
    return NULL;
}

/* Manager Thread */
void* manager_thread(void* arg) {
    static int order_id_counter = 100;
    Order order;
    read(cust_to_mgr[0], &order, sizeof(Order));

    order.order_id = ++order_id_counter;
    order.total_bill = 0;

    for(int i=0;i<order.item_count;i++)
        order.total_bill += menu[order.item_index[i]].price * order.quantity[i];

    pthread_mutex_lock(&sales_mutex);
    Regular_Sales += order.total_bill;
    pthread_mutex_unlock(&sales_mutex);

    printf("Manager processed order %d | Bill: %d\n", order.order_id, order.total_bill);

    write(mgr_to_cook[1], &order, sizeof(Order));
    return NULL;
}

/* Cook Thread */
void* cook_thread(void* arg) {
    Order order;
    read(mgr_to_cook[0], &order, sizeof(Order));

    printf("Cook preparing order %d\n", order.order_id);

    int max_time = 0;
    for(int i=0;i<order.item_count;i++)
        if(menu[order.item_index[i]].prep_time > max_time)
            max_time = menu[order.item_index[i]].prep_time;

    sleep(max_time);
    printf("Cook finished order %d\n", order.order_id);

    write(cook_to_waiter[1], &order, sizeof(Order));
    return NULL;
}

/* Waiter Thread */
void* waiter_thread(void* arg) {
    Order order;
    read(cook_to_waiter[0], &order, sizeof(Order));

    printf("Waiter delivering order %d\n", order.order_id);

    pthread_mutex_lock(&sales_mutex);
    Regular_Waiter_Sales += order.total_bill;
    pthread_mutex_unlock(&sales_mutex);

    pthread_mutex_lock(&file_mutex);
    FILE *fp = fopen("receipt.txt", "a");
    fprintf(fp, "Order %d | Customer %s | Bill %d\n", order.order_id, order.customer_name, order.total_bill);
    fclose(fp);
    pthread_mutex_unlock(&file_mutex);

    printf("Order %d delivered successfully\n", order.order_id);
    return NULL;
}

/* Function for GUI to start a customer thread */
void start_customer_thread() {
    pthread_t cust;
    pthread_create(&cust, NULL, customer_thread, NULL);
}
