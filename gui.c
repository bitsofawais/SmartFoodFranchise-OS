#include <gtk/gtk.h>
#include "backend.h"
#include "ds.h"

// Global Widgets
GtkWidget *stack;
GtkWidget *entry_name;
GtkWidget *entry_id;
GtkWidget *lbl_cart_status;
GtkWidget *lbl_sales;

// Cart Data
typedef struct {
    int item_ids[MAX_ITEMS_PER_ORDER];
    int count;
    double total;
} Cart;
Cart current_cart = { .count = 0, .total = 0.0 };

// --- CSS Styling ---
void load_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #f5f5f5; }"
        "button { padding: 10px; font-weight: bold; border-radius: 5px; background: #3498db; color: white; }"
        "button:hover { background: #2980b9; }"
        ".menu-card { background: white; padding: 15px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }"
        ".title { font-size: 24px; font-weight: bold; color: #333; }"
        ".price { color: #e74c3c; font-weight: bold; font-size: 16px; }"
        ".login-box { background: white; padding: 40px; border-radius: 10px; margin: 50px; }"
        , -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// --- Event Handlers ---

void on_login_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
    const char *id_str = gtk_entry_get_text(GTK_ENTRY(entry_id));
    
    if(strlen(name) > 0 && strlen(id_str) > 0) {
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "page_menu");
    }
}

void on_add_to_cart(GtkWidget *widget, gpointer data) {
    (void)widget;
    int item_id = GPOINTER_TO_INT(data);
    MenuItem* item = menu_find(&menu_list, item_id);
    
    if(item && current_cart.count < MAX_ITEMS_PER_ORDER) {
        current_cart.item_ids[current_cart.count++] = item->id;
        current_cart.total += item->price;
        
        char buffer[100];
        snprintf(buffer, 100, "Cart: %d items ($%.2f)", current_cart.count, current_cart.total);
        gtk_label_set_text(GTK_LABEL(lbl_cart_status), buffer);
    }
}

void on_checkout_clicked(GtkWidget *widget, gpointer data) {
    (void)data; // widget is used for g_object_get_data
    printf("[GUI] Checkout Clicked. Cart Count: %d\n", current_cart.count);
    if(current_cart.count == 0) {
        printf("[GUI] Cart is empty, ignoring checkout.\n");
        return;
    }
    
    // Retrieve Checkbox
    GtkWidget *chk_vip = (GtkWidget *)g_object_get_data(G_OBJECT(widget), "vip_checkbox");
    gboolean is_vip = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_vip));

    Order ord;
    ord.order_id = rand() % 1000 + 1;
    ord.customer_id = atoi(gtk_entry_get_text(GTK_ENTRY(entry_id)));
    
    // Copy Name from Entry
    const char* name_text = gtk_entry_get_text(GTK_ENTRY(entry_name));
    strncpy(ord.customer_name, name_text, MAX_NAME_LEN - 1);
    ord.customer_name[MAX_NAME_LEN - 1] = '\0';

    ord.item_count = current_cart.count;
    for(int i=0; i<current_cart.count; i++) ord.item_ids[i] = current_cart.item_ids[i];
    ord.total_bill = current_cart.total;
    ord.is_vip = is_vip ? 1 : 0; // Set VIP Status
    
    printf("[GUI] Sending %s Order #%d (Customer: %s, Bill: $%.2f) to Backend...\n", 
            is_vip ? "VIP" : "Regular", ord.order_id, ord.customer_name, ord.total_bill);
    start_customer_order(ord);
    
    // Reset Cart
    current_cart.count = 0;
    current_cart.total = 0.0;
    gtk_label_set_text(GTK_LABEL(lbl_cart_status), "Order Placed! Cart: 0 items");
}

void on_end_day_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    // Just trigger a report generation log
    g_print("End of Day Request sent (Simulation). Report will be in file on exit.\n");
    gtk_main_quit();
}

void on_view_reports_clicked(GtkWidget *widget, gpointer data) {
    (void)data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Reports", GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "Close", GTK_RESPONSE_CLOSE,
                                                    NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *notebook = gtk_notebook_new();

    // Tab 1: Receipts
    GtkWidget *scrolled1 = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *tv_receipts = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv_receipts), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled1), tv_receipts);
    
    char *receipts_content = NULL;
    if (g_file_get_contents("receipts.txt", &receipts_content, NULL, NULL)) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv_receipts));
        gtk_text_buffer_set_text(buffer, receipts_content, -1);
        g_free(receipts_content);
    } else {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv_receipts));
        gtk_text_buffer_set_text(buffer, "No receipts found.", -1);
    }
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled1, gtk_label_new("Receipts"));

    // Tab 2: End of Day (Live Preview)
    GtkWidget *scrolled2 = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *tv_report = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv_report), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled2), tv_report);

    // Generate Live Report String
    char report_buf[512];
    pthread_mutex_lock(&sales_mutex);
    snprintf(report_buf, 512, 
        "=== LIVE SESSION REPORT ===\n\n"
        "Total Orders: %d\n"
        "Total Sales (Manager): $%.2f\n"
        "Total Collected (Waiter): $%.2f\n\n"
        "Status: %s\n\n"
        "(Note: Full report will be saved to 'end_of_day_report.txt' on exit)", 
        Total_Orders, Regular_Sales, Regular_Waiter_Sales,
        (Regular_Sales == Regular_Waiter_Sales) ? "BALANCED" : "PENDING/MISMATCH");
    pthread_mutex_unlock(&sales_mutex);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv_report));
    gtk_text_buffer_set_text(buffer, report_buf, -1);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled2, gtk_label_new("Live Report"));

    gtk_box_pack_start(GTK_BOX(content_area), notebook, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

gboolean timer_callback(gpointer data) {
    (void)data;
    char buffer[200];
    pthread_mutex_lock(&sales_mutex);
    snprintf(buffer, 200, "Sales: $%.2f", Regular_Sales);
    pthread_mutex_unlock(&sales_mutex);
    gtk_label_set_text(GTK_LABEL(lbl_sales), buffer);
    return TRUE;
}

// --- UI Construction ---

GtkWidget* create_menu_card(MenuItem* item) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "menu-card");

    // Load Image (Fallback if not found)
    GError *err = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(item->image_path, 120, 120, TRUE, &err);
    if(!pixbuf) {
        // Try fallback
        pixbuf = gdk_pixbuf_new_from_file_at_scale("burger_icon.png", 120, 120, TRUE, NULL); // Safe fallback
    }
    GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
    
    GtkWidget *lbl_name = gtk_label_new(item->name);
    
    char price_str[20];
    sprintf(price_str, "$%.2f", item->price);
    GtkWidget *lbl_price = gtk_label_new(price_str);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_price), "price");

    GtkWidget *btn_add = gtk_button_new_with_label("Add to Cart");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_add_to_cart), GINT_TO_POINTER(item->id));

    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), lbl_name, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), lbl_price, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_add, FALSE, FALSE, 0);

    return box;
}

GtkWidget* create_login_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "login-box");
    
    GtkWidget *lbl_title = gtk_label_new("Welcome to Smart Food Franchise");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_title), "title");
    
    entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_name), "Enter Name");
    
    entry_id = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_id), "Enter ID");
    
    GtkWidget *btn_login = gtk_button_new_with_label("Login");
    g_signal_connect(btn_login, "clicked", G_CALLBACK(on_login_clicked), NULL);
    
    gtk_box_pack_start(GTK_BOX(box), lbl_title, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), entry_name, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), entry_id, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_login, FALSE, FALSE, 10);
    
    return box;
}

GtkWidget* create_menu_page() {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    
    // Top Bar
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_title = gtk_label_new("Menu");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_title), "title");
    
    lbl_sales = gtk_label_new("Sales: $0.00");
    lbl_cart_status = gtk_label_new("Cart: 0 items");

    // VIP Checkbox
    GtkWidget *chk_vip = gtk_check_button_new_with_label("VIP Priority Order");
    
    GtkWidget *btn_checkout = gtk_button_new_with_label("Checkout");
    // Pass BOTH the checkbox and NULL using a struct or just manage state differently. 
    // To simplify, we will make 'chk_vip' global or static or use g_object_set_data.
    g_object_set_data(G_OBJECT(btn_checkout), "vip_checkbox", chk_vip); 
    g_signal_connect(btn_checkout, "clicked", G_CALLBACK(on_checkout_clicked), NULL);
    
    GtkWidget *btn_reports = gtk_button_new_with_label("View Reports");
    g_signal_connect(btn_reports, "clicked", G_CALLBACK(on_view_reports_clicked), NULL);

    GtkWidget *btn_exit = gtk_button_new_with_label("End Day & Exit");
    g_signal_connect(btn_exit, "clicked", G_CALLBACK(on_end_day_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(hbox), lbl_title, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), btn_exit, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), btn_reports, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), btn_checkout, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), chk_vip, FALSE, FALSE, 5); // Add VIP check
    gtk_box_pack_end(GTK_BOX(hbox), lbl_cart_status, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), lbl_sales, FALSE, FALSE, 5);
    
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);

    // Scrollable FlowBox
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *flowbox = gtk_flow_box_new();
    gtk_widget_set_valign(GTK_WIDGET(flowbox), GTK_ALIGN_START);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flowbox), 3);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flowbox), GTK_SELECTION_NONE);
    
    MenuNode* curr = menu_list.head;
    while(curr) {
        GtkWidget *card = create_menu_card(&curr->data);
        gtk_flow_box_insert(GTK_FLOW_BOX(flowbox), card, -1);
        curr = curr->next;
    }
    
    gtk_container_add(GTK_CONTAINER(scrolled), flowbox);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    return vbox;
}

int main(int argc, char *argv[]) {
    // Init Backend
    backend_init();

    // Init GTK
    gtk_init(&argc, &argv);
    load_css();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Smart Food Franchise System");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 700);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    
    gtk_stack_add_named(GTK_STACK(stack), create_login_page(), "page_login");
    gtk_stack_add_named(GTK_STACK(stack), create_menu_page(), "page_menu");
    
    gtk_container_add(GTK_CONTAINER(window), stack);
    
    gtk_widget_show_all(window);
    
    // Hide non-initial pages if needed, but stack handles visible child
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "page_login");

    g_timeout_add(1000, timer_callback, NULL);

    gtk_main();

    backend_cleanup();
    return 0;
}
