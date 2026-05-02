#include "restaurant.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

/* Forward declaration for GUI */
extern int gtk_main(int argc, char **argv);

int main(int argc, char *argv[]) {
    pthread_t mgr, cook, waiter;

    pthread_mutex_init(&sales_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);

    pipe(cust_to_mgr);
    pipe(mgr_to_cook);
    pipe(cook_to_waiter);

    pthread_create(&mgr, NULL, manager_thread, NULL);
    pthread_create(&cook, NULL, cook_thread, NULL);
    pthread_create(&waiter, NULL, waiter_thread, NULL);

    /* Launch GUI */
    extern int gui_main(int argc, char *argv[]);
    gui_main(argc, argv);

    pthread_join(mgr, NULL);
    pthread_join(cook, NULL);
    pthread_join(waiter, NULL);

    printf("\n--- End of Day Report ---\n");
    printf("Regular Sales: %d\n", Regular_Sales);
    printf("Waiter Sales: %d\n", Regular_Waiter_Sales);
    if(Regular_Sales == Regular_Waiter_Sales)
        printf("Sales matched successfully\n");
    else
        printf("Sales mismatch detected\n");

    return 0;
}
