#include "backend.h"
#include <stdio.h>
#include <unistd.h>

// Global Definitions
int pipe_cust_mgr[2];
// int pipe_mgr_cook[2]; // REMOVED
int pipe_cook_waiter[2];

SafeQueue order_queue; // New Priority Queue

MenuList menu_list;
double Regular_Sales = 0.0;
double Regular_Waiter_Sales = 0.0;
int Total_Orders = 0;
pthread_mutex_t sales_mutex;

pthread_t t_manager, t_cook[3], t_waiter[3];
int running = 1;

void backend_init() {
    // Initialize Data Structures
    menu_init(&menu_list);
    menu_load(&menu_list, "menu.txt");
    queue_init(&order_queue); // Init Queue
    pthread_mutex_init(&sales_mutex, NULL);

    // Initialize Pipes
    // pipe_mgr_cook Removed
    if (pipe(pipe_cust_mgr) == -1 || pipe(pipe_cook_waiter) == -1) {
        perror("Pipe failed");
        exit(1);
    }

    // Start Threads
    pthread_create(&t_manager, NULL, manager_runtime, NULL);
    for(int i=0; i<3; i++) {
        pthread_create(&t_cook[i], NULL, cook_runtime, NULL);
        pthread_create(&t_waiter[i], NULL, waiter_runtime, NULL);
    }
}

void* manager_runtime(void* arg) {
    (void)arg;
    Order order;
    while(running) {
        // Read from Customer pipe (GUI -> Manager)
        ssize_t n = read(pipe_cust_mgr[0], &order, sizeof(Order));
        if (n > 0) {
            printf("[Manager] Received %s Order #%d from Customer %d\n", 
                   order.is_vip ? "VIP" : "Regular", order.order_id, order.customer_id);
            
            // Update Regular_Sales
            pthread_mutex_lock(&sales_mutex);
            Regular_Sales += order.total_bill;
            Total_Orders++;
            pthread_mutex_unlock(&sales_mutex);

            // Forward to Cook (via Priority Queue)
            queue_push(&order_queue, order);
        }
    }
    return NULL;
}

void* cook_runtime(void* arg) {
    (void)arg;
    Order order;
    pthread_t my_id = pthread_self(); // For debug identification

    while(running) {
        // Pop from Priority Queue (Blocking)
        order = queue_pop(&order_queue);
        
        // Check for empty/dummy order if shutdown signal (rudimentary check)
        if (order.order_id == -1) break; 

        printf("[Cook %lu] Preparing Order #%d (%s)...\n", 
                (unsigned long)my_id, order.order_id, order.is_vip ? "VIP" : "Regular");
        
        // Calculate total prep time
        int total_prep_time = 0;
        for(int i=0; i<order.item_count; i++) {
            MenuItem* item = menu_find(&menu_list, order.item_ids[i]);
            if(item) total_prep_time += item->prep_time_sec;
        }
        if(total_prep_time == 0) total_prep_time = 2; // Default fallback

        // Simulate preparation time
        // printf("[Cook] Prep time for Order #%d is %d seconds.\n", order.order_id, total_prep_time);
        sleep(total_prep_time); 

        printf("[Cook %lu] Order #%d Ready!\n", (unsigned long)my_id, order.order_id);
        // Forward to Waiter pipe
        write(pipe_cook_waiter[1], &order, sizeof(Order));
    }
    return NULL;
}

void* waiter_runtime(void* arg) {
    (void)arg;
    Order order;
    while(running) {
        // Read from Cook pipe
        ssize_t n = read(pipe_cook_waiter[0], &order, sizeof(Order));
        if (n > 0) {
            printf("[Waiter] Delivering Order #%d...\n", order.order_id);
            sleep(1); // Delivery time

            // Update Waiter Sales
            pthread_mutex_lock(&sales_mutex);
            Regular_Waiter_Sales += order.total_bill;
            pthread_mutex_unlock(&sales_mutex);

            printf("[Waiter] Order #%d Delivered! Revenue: $%.2f\n", order.order_id, order.total_bill);
            
            // Save Receipt to file
            FILE* fp = fopen("receipts.txt", "a");
            if(fp) {
                fprintf(fp, "Order ID: %d | Customer: %s (ID: %d) | Bill: $%.2f | Status: Delivered\n", 
                        order.order_id, order.customer_name, order.customer_id, order.total_bill);
                fclose(fp);
            }
        }
    }
    return NULL;
}

void start_customer_order(Order order) {
    // Customer writes to Manager pipe
    printf("[Backend] Writing Order #%d to Manager Pipe...\n", order.order_id);
    ssize_t result = write(pipe_cust_mgr[1], &order, sizeof(Order));
    if(result == -1) perror("[Backend] Write to pipe failed");
    else printf("[Backend] Write success (%zd bytes).\n", result);
}

void backend_cleanup() {
    running = 0;
    // Close thread handles if needed or join
    // Generate Report
    FILE* fp = fopen("end_of_day_report.txt", "a"); // CHANGED to Append
    if(fp) {
        fprintf(fp, "\n=== End of Day Report (Session) ===\n");
        fprintf(fp, "Total Orders Served: %d\n", Total_Orders);
        fprintf(fp, "Total Revenue: $%.2f\n", Regular_Sales);
        fprintf(fp, "Waiter Collected: $%.2f\n", Regular_Waiter_Sales);
        if(Regular_Sales == Regular_Waiter_Sales) {
            fprintf(fp, "Status: BALANCED\n");
        } else {
            fprintf(fp, "Status: DISCREPANCY DETECTED\n");
        }
        fprintf(fp, "===================================\n");
        fclose(fp);
        printf("Report updated: end_of_day_report.txt\n");
    }

    menu_destroy(&menu_list);
    queue_destroy(&order_queue);
    pthread_mutex_destroy(&sales_mutex);
}
