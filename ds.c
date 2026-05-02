#include "ds.h"

// --- Menu List Implementation ---

void menu_init(MenuList* list) {
    list->head = NULL;
    list->size = 0;
}


void menu_load(MenuList* list, const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open menu file");
        return;
    }
    
    char name[MAX_NAME_LEN];
    double price;
    int prep_time;
    int id = 1;
    
    while (fscanf(fp, "%s %lf %d", name, &price, &prep_time) == 3) {
        char img_path[100];
        // Simple mapping for demo assets
        if(strcasecmp(name, "Burger") == 0) strcpy(img_path, "burger_icon.png");
        else if(strcasecmp(name, "Pizza") == 0) strcpy(img_path, "pizza_icon.png");
        else if(strcasecmp(name, "Fries") == 0) strcpy(img_path, "fries.jpg");
        else if(strcasecmp(name, "Drink") == 0) strcpy(img_path, "colddrink.jpg"); 
        else if(strcasecmp(name, "Sandwich") == 0) strcpy(img_path, "sand.png");
        else strcpy(img_path, "default_icon.png");

        menu_add(list, id++, name, price, prep_time, img_path);
    }
    fclose(fp);
    printf("Menu loaded with %d items.\n", list->size);
    
}

void menu_add(MenuList* list, int id, const char* name, double price, int prep_time, const char* img_path) {
    MenuNode* new_node = (MenuNode*)malloc(sizeof(MenuNode));
    new_node->data.id = id;
    strncpy(new_node->data.name, name, MAX_NAME_LEN);
    new_node->data.price = price;
    new_node->data.prep_time_sec = prep_time;
    strncpy(new_node->data.image_path, img_path, 100);
    new_node->next = NULL;

    if (list->head == NULL) {
        list->head = new_node;
    } else {
        MenuNode* current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
    list->size++;
}

MenuItem* menu_find(MenuList* list, int id) {
    MenuNode* current = list->head;
    while (current != NULL) {
        if (current->data.id == id) {
            return &current->data;
        }
        current = current->next;
    }
    return NULL;
}

void menu_destroy(MenuList* list) {
    MenuNode* current = list->head;
    while (current != NULL) {
        MenuNode* temp = current;
        current = current->next;
        free(temp);
    }
    list->head = NULL;
    list->size = 0;
}

// --- Safe Queue Implementation ---

void queue_init(SafeQueue* q) {
    q->front = q->rear = NULL;
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void queue_push(SafeQueue* q, Order order) {
    OrderNode* new_node = (OrderNode*)malloc(sizeof(OrderNode));
    new_node->data = order;
    new_node->next = NULL;

    pthread_mutex_lock(&q->lock);

    if (q->front == NULL) {
        // Queue is empty
        q->front = q->rear = new_node;
    } else {
        if (order.is_vip) {
            // Priority Insert: Valid logic needs to find the insertion point
            // Case 1: Insert at front if head is not VIP
            if (!q->front->data.is_vip) {
                new_node->next = q->front;
                q->front = new_node;
            } else {
                // Case 2: Traverse to find the end of VIP block
                OrderNode* current = q->front;
                while (current->next != NULL && current->next->data.is_vip) {
                    current = current->next;
                }
                // Insert after 'current'
                new_node->next = current->next;
                current->next = new_node;
                
                // If we inserted at the very end, update rear
                if (new_node->next == NULL) {
                    q->rear = new_node;
                }
            }
        } else {
            // Regular Insert: Always at the end
            q->rear->next = new_node;
            q->rear = new_node;
        }
    }
    q->count++;
    
    pthread_cond_signal(&q->cond); // Signal waiting threads
    pthread_mutex_unlock(&q->lock);
}

Order queue_pop(SafeQueue* q) {
    pthread_mutex_lock(&q->lock);

    while (q->front == NULL) {
        pthread_cond_wait(&q->cond, &q->lock);
    }

    OrderNode* temp = q->front;
    Order data = temp->data;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    q->count--;
    free(temp);

    pthread_mutex_unlock(&q->lock);
    return data;
}

int queue_is_empty(SafeQueue* q) {
    pthread_mutex_lock(&q->lock);
    int is_empty = (q->front == NULL);
    pthread_mutex_unlock(&q->lock);
    return is_empty;
}

void queue_destroy(SafeQueue* q) {
    pthread_mutex_lock(&q->lock);
    while (q->front != NULL) {
        OrderNode* temp = q->front;
        q->front = q->front->next;
        free(temp);
    }
    q->rear = NULL;
    q->count = 0;
    pthread_mutex_unlock(&q->lock);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
}
