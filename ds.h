#ifndef DS_H
#define DS_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 50
#define MAX_ITEMS_PER_ORDER 10

// --- Data Models ---

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    double price;
    int prep_time_sec;
    char image_path[100];
} MenuItem;

typedef struct {
    int order_id;
    int customer_id;
    char customer_name[MAX_NAME_LEN]; // Added Name
    int item_ids[MAX_ITEMS_PER_ORDER];
    int item_count;
    double total_bill;
    int is_vip; // 0 = Regular, 1 = VIP
} Order;

// --- Linked List for Menu ---

typedef struct MenuNode {
    MenuItem data;
    struct MenuNode* next;
} MenuNode;

typedef struct {
    MenuNode* head;
    int size;
} MenuList;

// --- Thread-Safe Queue for Orders ---

typedef struct OrderNode {
    Order data;
    struct OrderNode* next;
} OrderNode;

typedef struct {
    OrderNode* front;
    OrderNode* rear;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} SafeQueue;

// --- Function Prototypes ---

// Menu Functions
void menu_init(MenuList* list);
void menu_load(MenuList* list, const char* filename);
void menu_add(MenuList* list, int id, const char* name, double price, int prep_time, const char* img_path);
MenuItem* menu_find(MenuList* list, int id);
void menu_destroy(MenuList* list);

// Queue Functions
void queue_init(SafeQueue* q);
void queue_push(SafeQueue* q, Order order);
Order queue_pop(SafeQueue* q); // Blocking pop
int queue_is_empty(SafeQueue* q);
void queue_destroy(SafeQueue* q);

#endif
