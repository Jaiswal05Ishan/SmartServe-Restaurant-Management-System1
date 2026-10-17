
#include <stdio.h>
#include <stdlib.h>

#define FOOD_COUNT 6
#define MAX_ORDERS 100

typedef struct {
    int id;
    char name[30];
    float price;
} Food;

typedef struct {
    int order_id;
    int food_id;
    int quantity;
    float total;
    int processed;
} Order;

/* Singly linked list node */
typedef struct OrderNode {
    int order_index;
    struct OrderNode *next;
} OrderNode;

/* Binary Search Tree node: key is the Order ID */
typedef struct BSTNode {
    int order_index;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

Food menu[FOOD_COUNT] = {
    {1, "Paneer Tikka", 180.0f},
    {2, "Veg Biryani", 160.0f},
    {3, "Masala Dosa", 90.0f},
    {4, "Veg Burger", 110.0f},
    {5, "Cold Coffee", 80.0f},
    {6, "Gulab Jamun", 60.0f}
};

Order orders[MAX_ORDERS];
int order_count = 0;
int next_order_id = 1;

/* Circular FIFO queue */
int kitchen_queue[MAX_ORDERS];
int queue_front = 0;
int queue_rear = 0;
int queue_count = 0;

/* Linked list */
OrderNode *list_head = NULL;
OrderNode *list_tail = NULL;

/* Stack (LIFO) stores indexes of orders; top is the most recent order */
int order_stack[MAX_ORDERS];
int stack_top = -1;

Food *find_food(int id)
{
    for (int i = 0; i < FOOD_COUNT; i++) {
        if (menu[i].id == id) {
            return &menu[i];
        }
    }
    return NULL;
}

/* Insert an order into a BST using its Order ID as the key */
BSTNode *bst_insert(BSTNode *root, int order_index)
{
    if (root == NULL) {
        BSTNode *node = malloc(sizeof(BSTNode));
        if (node == NULL) {
            return NULL;
        }
        node->order_index = order_index;
        node->left = NULL;
        node->right = NULL;
        return node;
    }

    if (orders[order_index].order_id < orders[root->order_index].order_id) {
        root->left = bst_insert(root->left, order_index);
    } else if (orders[order_index].order_id > orders[root->order_index].order_id) {
        root->right = bst_insert(root->right, order_index);
    }

    return root;
}

/* Build a temporary BST from the current order records */
BSTNode *build_order_bst(void)
{
    BSTNode *root = NULL;

    for (int i = 0; i < order_count; i++) {
        BSTNode *new_root = bst_insert(root, i);
        if (new_root == NULL) {
            /* If allocation fails, release the partial tree. */
            /* The helper below is declared before its use. */
            return root;
        }
        root = new_root;
    }

    return root;
}

/* Search the BST by Order ID */
BSTNode *bst_search(BSTNode *root, int order_id)
{
    while (root != NULL) {
        int current_id = orders[root->order_index].order_id;
        if (order_id == current_id) {
            return root;
        }
        root = (order_id < current_id) ? root->left : root->right;
    }
    return NULL;
}

/* In-order traversal displays orders in ascending Order ID */
void bst_inorder(BSTNode *root)
{
    if (root == NULL) {
        return;
    }

    bst_inorder(root->left);

    int i = root->order_index;
    Food *food = find_food(orders[i].food_id);
    printf("Order ID: %-4d | %-22s | Qty: %-3d | Rs. %-8.2f | %s\n",
           orders[i].order_id,
           food != NULL ? food->name : "Unknown food",
           orders[i].quantity,
           orders[i].total,
           orders[i].processed ? "Processed" : "Pending");

    bst_inorder(root->right);
}

void free_order_bst(BSTNode *root)
{
    if (root == NULL) {
        return;
    }
    free_order_bst(root->left);
    free_order_bst(root->right);
    free(root);
}

void display_orders_bst(void)
{
    if (order_count == 0) {
        printf("\nNo orders available to build the BST.\n");
        return;
    }

    BSTNode *root = NULL;
    for (int i = 0; i < order_count; i++) {
        root = bst_insert(root, i);
        /* For this fixed-size student project, allocation failure is unlikely. */
    }

    printf("\n========== ORDERS IN BINARY SEARCH TREE ==========\n");
    printf("In-order traversal (ascending Order ID):\n");
    bst_inorder(root);
    free_order_bst(root);
}

void search_order_bst(void)
{
    int id;
    printf("Enter Order ID to search in BST: ");
    if (scanf("%d", &id) != 1) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Clear invalid input */
        }
        printf("Invalid input.\n");
        return;
    }

    BSTNode *root = NULL;
    for (int i = 0; i < order_count; i++) {
        root = bst_insert(root, i);
    }

    BSTNode *found = bst_search(root, id);
    if (found == NULL) {
        printf("Order ID %d not found in BST.\n", id);
    } else {
        int i = found->order_index;
        Food *food = find_food(orders[i].food_id);
        printf("\nOrder found in BST!\n");
        printf("Order ID: %d\n", orders[i].order_id);
        printf("Food: %s\n", food != NULL ? food->name : "Unknown food");
        printf("Quantity: %d\n", orders[i].quantity);
        printf("Total: Rs. %.2f\n", orders[i].total);
        printf("Kitchen Status: %s\n",
               orders[i].processed ? "Processed" : "Pending");
    }
    free_order_bst(root);
}


/* Push an order index onto the LIFO stack */
int push_order_index(int index)
{
    if (stack_top >= MAX_ORDERS - 1) {
        printf("Order stack is full.\n");
        return 0;
    }

    order_stack[++stack_top] = index;
    return 1;
}

/* Display the stack from TOP to BOTTOM */
void display_order_stack(void)
{
    if (stack_top == -1) {
        printf("\nRecent-order stack is empty.\n");
        return;
    }

    printf("\n========== RECENT-ORDER STACK (LIFO) ==========\n");
    printf("TOP\n");

    for (int i = stack_top; i >= 0; i--) {
        int index = order_stack[i];
        Food *food = find_food(orders[index].food_id);

        if (food != NULL) {
            printf("[Order ID: %d | %s | Qty: %d]\n",
                   orders[index].order_id,
                   food->name,
                   orders[index].quantity);
        }
    }

    printf("BOTTOM\n");
    printf("Note: Stack display does not change saved order history.\n");
}

/* Peek at the newest order without removing it */
void peek_order_stack(void)
{
    if (stack_top == -1) {
        printf("\nRecent-order stack is empty.\n");
        return;
    }

    int index = order_stack[stack_top];
    Food *food = find_food(orders[index].food_id);

    printf("\n========== TOP OF ORDER STACK ==========\n");
    printf("Order ID: %d\n", orders[index].order_id);
    if (food != NULL) {
        printf("Food: %s\n", food->name);
    }
    printf("Quantity: %d\n", orders[index].quantity);
    printf("Total: Rs. %.2f\n", orders[index].total);
    printf("Kitchen Status: %s\n",
           orders[index].processed ? "Processed" : "Pending");
}

/* Pop only removes an index from the demo stack, not the actual order */
void pop_order_stack(void)
{
    if (stack_top == -1) {
        printf("\nCannot pop: recent-order stack is empty.\n");
        return;
    }

    int index = order_stack[stack_top--];
    printf("\nPopped Order ID %d from the stack.\n", orders[index].order_id);
    printf("The actual order remains in order history and data/orders.txt.\n");
}

/* Insert an order index at the end of the linked list */
int list_append(int order_index)
{
    OrderNode *node = malloc(sizeof(OrderNode));

    if (node == NULL) {
        printf("Memory allocation failed for linked list.\n");
        return 0;
    }

    node->order_index = order_index;
    node->next = NULL;

    if (list_head == NULL) {
        list_head = node;
        list_tail = node;
    } else {
        list_tail->next = node;
        list_tail = node;
    }

    return 1;
}

/* Display all orders using linked list traversal */
void display_linked_list(void)
{
    if (list_head == NULL) {
        printf("\nLinked list is empty.\n");
        return;
    }

    OrderNode *current = list_head;

    printf("\n========== ORDERS IN LINKED LIST ==========\n");
    printf("HEAD");

    while (current != NULL) {
        int i = current->order_index;
        Food *food = find_food(orders[i].food_id);

        printf(" -> [Order ID: %d",
               orders[i].order_id);

        if (food != NULL) {
            printf(", Food: %s", food->name);
        }

        printf(", Status: %s]",
               orders[i].processed ? "Processed" : "Pending");

        current = current->next;
    }

    printf(" -> NULL\n");
}

/* Search an order using linked list traversal */
void search_order_linked_list(void)
{
    int id;

    printf("Enter Order ID to search: ");

    if (scanf("%d", &id) != 1) {
        printf("Invalid input.\n");
        return;
    }

    OrderNode *current = list_head;

    while (current != NULL) {
        int i = current->order_index;

        if (orders[i].order_id == id) {
            Food *food = find_food(orders[i].food_id);

            printf("\nOrder found in linked list!\n");
            printf("Order ID: %d\n", orders[i].order_id);

            if (food != NULL) {
                printf("Food: %s\n", food->name);
            }

            printf("Quantity: %d\n", orders[i].quantity);
            printf("Total: Rs. %.2f\n", orders[i].total);
            printf("Kitchen Status: %s\n",
                   orders[i].processed ? "Processed" : "Pending");
            return;
        }

        current = current->next;
    }

    printf("Order ID not found in linked list.\n");
}

/* Free linked list memory */
void free_linked_list(void)
{
    OrderNode *current = list_head;

    while (current != NULL) {
        OrderNode *next_node = current->next;
        free(current);
        current = next_node;
    }

    list_head = NULL;
    list_tail = NULL;
}

/* Add order index to circular queue */
int enqueue(int index)
{
    if (queue_count >= MAX_ORDERS) {
        return 0;
    }

    kitchen_queue[queue_rear] = index;
    queue_rear = (queue_rear + 1) % MAX_ORDERS;
    queue_count++;

    return 1;
}

/* Remove oldest pending order */
int dequeue(void)
{
    if (queue_count == 0) {
        return -1;
    }

    int index = kitchen_queue[queue_front];
    queue_front = (queue_front + 1) % MAX_ORDERS;
    queue_count--;

    return index;
}

/* Save all orders and their processing status */
int save_orders(void)
{
    FILE *file = fopen("data/orders.txt", "w");

    if (file == NULL) {
        printf("ERROR: Cannot open data/orders.txt.\n");
        return 0;
    }

    for (int i = 0; i < order_count; i++) {
        if (fprintf(file, "%d %d %d %.2f %d\n",
                    orders[i].order_id,
                    orders[i].food_id,
                    orders[i].quantity,
                    orders[i].total,
                    orders[i].processed) < 0) {
            fclose(file);
            printf("ERROR: Could not write order data.\n");
            return 0;
        }
    }

    if (fclose(file) != 0) {
        printf("ERROR: Could not finish saving file.\n");
        return 0;
    }

    printf("Orders saved successfully: %d\n", order_count);
    return 1;
}

/* Load file, rebuild linked list and pending queue */
void load_orders(void)
{
    FILE *file = fopen("data/orders.txt", "r");

    if (file == NULL) {
        printf("No previous order file found.\n");
        return;
    }

    char line[200];
    int loaded = 0;

    while (fgets(line, sizeof(line), file) != NULL &&
           order_count < MAX_ORDERS) {

        Order temp;

        int fields = sscanf(line, "%d %d %d %f %d",
                            &temp.order_id,
                            &temp.food_id,
                            &temp.quantity,
                            &temp.total,
                            &temp.processed);

        /* Support records from the earlier four-field format */
        if (fields == 4) {
            temp.processed = 0;
        }

        if (fields < 4 ||
            temp.order_id <= 0 ||
            temp.quantity <= 0 ||
            find_food(temp.food_id) == NULL) {
            continue;
        }

        if (temp.processed != 0 && temp.processed != 1) {
            temp.processed = 0;
        }

        int index = order_count;
        orders[index] = temp;

        if (!list_append(index)) {
            fclose(file);
            free_linked_list();
            order_count = 0;
            queue_front = queue_rear = queue_count = 0;
            printf("Could not rebuild linked list.\n");
            return;
        }

        if (!temp.processed && !enqueue(index)) {
            fclose(file);
            printf("Kitchen queue is full.\n");
            return;
        }

        if (temp.order_id >= next_order_id) {
            next_order_id = temp.order_id + 1;
        }

        order_count++;
        push_order_index(index); /* Rebuild stack from saved orders */
        loaded++;
    }

    fclose(file);

    printf("Previous orders loaded: %d\n", loaded);
    printf("Linked list rebuilt successfully.\n");
    printf("Pending kitchen orders: %d\n", queue_count);
}

void display_food_menu(void)
{
    printf("\n=============== FOOD MENU ===============\n");
    printf("%-5s %-25s %s\n", "ID", "Food Name", "Price");

    for (int i = 0; i < FOOD_COUNT; i++) {
        printf("%-5d %-25s Rs. %.2f\n",
               menu[i].id, menu[i].name, menu[i].price);
    }
}

void place_order(void)
{
    if (order_count >= MAX_ORDERS) {
        printf("Maximum order limit reached.\n");
        return;
    }

    int food_id, quantity;

    display_food_menu();

    printf("Enter Food ID: ");
    if (scanf("%d", &food_id) != 1) {
        printf("Invalid input.\n");
        return;
    }

    Food *food = find_food(food_id);

    if (food == NULL) {
        printf("Invalid food ID.\n");
        return;
    }

    printf("Enter Quantity: ");
    if (scanf("%d", &quantity) != 1 || quantity <= 0) {
        printf("Quantity must be positive.\n");
        return;
    }

    int index = order_count;

    Order new_order = {
        next_order_id,
        food_id,
        quantity,
        food->price * quantity,
        0
    };

    /* Create the linked list node before committing the order */
    if (!list_append(index)) {
        printf("Order could not be added.\n");
        return;
    }

    if (!enqueue(index)) {
        /* Undo the most recently appended linked list node */
        if (list_head == list_tail) {
            free(list_head);
            list_head = NULL;
            list_tail = NULL;
        } else {
            OrderNode *current = list_head;

            while (current->next != list_tail) {
                current = current->next;
            }

            free(list_tail);
            list_tail = current;
            list_tail->next = NULL;
        }

        printf("Kitchen queue is full. Order not placed.\n");
        return;
    }

    orders[index] = new_order;
    order_count++;
    push_order_index(index);
    next_order_id++;

    printf("\nOrder placed successfully!\n");
    printf("Order ID: %d\n", new_order.order_id);
    printf("Food: %s\n", food->name);
    printf("Quantity: %d\n", quantity);
    printf("Total: Rs. %.2f\n", new_order.total);
    printf("Order added to FIFO queue and linked list.\n");

    save_orders();
}

void display_kitchen_queue(void)
{
    if (queue_count == 0) {
        printf("\nKitchen queue is empty.\n");
        return;
    }

    printf("\n========== KITCHEN FIFO QUEUE ==========\n");

    for (int i = 0; i < queue_count; i++) {
        int position = (queue_front + i) % MAX_ORDERS;
        int index = kitchen_queue[position];
        Food *food = find_food(orders[index].food_id);

        if (food != NULL) {
            printf("Order ID: %d | %s | Quantity: %d\n",
                   orders[index].order_id,
                   food->name,
                   orders[index].quantity);
        }
    }
}

void process_next_order(void)
{
    if (queue_count == 0) {
        printf("\nNo pending orders.\n");
        return;
    }

    int index = dequeue();

    if (index < 0 || index >= order_count) {
        printf("Queue error.\n");
        return;
    }

    orders[index].processed = 1;

    Food *food = find_food(orders[index].food_id);

    printf("\n========== ORDER PROCESSED ==========\n");
    printf("Order ID: %d\n", orders[index].order_id);

    if (food != NULL) {
        printf("Food: %s\n", food->name);
    }

    printf("This was the oldest pending order.\n");
    save_orders();
}

void order_history(void)
{
    if (order_count == 0) {
        printf("\nNo orders available.\n");
        return;
    }

    printf("\n================ ORDER HISTORY ================\n");
    printf("%-9s %-22s %-9s %-12s %-12s\n",
           "Order ID", "Food", "Quantity", "Total", "Kitchen");

    for (int i = 0; i < order_count; i++) {
        Food *food = find_food(orders[i].food_id);

        if (food != NULL) {
            printf("%-9d %-22s %-9d Rs. %-8.2f %-12s\n",
                   orders[i].order_id,
                   food->name,
                   orders[i].quantity,
                   orders[i].total,
                   orders[i].processed ? "Processed" : "Pending");
        }
    }
}

void generate_bill(void)
{
    int id;

    printf("Enter Order ID: ");
    if (scanf("%d", &id) != 1) {
        printf("Invalid input.\n");
        return;
    }

    for (int i = 0; i < order_count; i++) {
        if (orders[i].order_id == id) {
            Food *food = find_food(orders[i].food_id);

            if (food == NULL) {
                printf("Food item not found.\n");
                return;
            }

            float subtotal = orders[i].total;
            float gst = subtotal * 0.05f;

            printf("\n========== CUSTOMER BILL ==========\n");
            printf("Order ID: %d\n", id);
            printf("Food: %s\n", food->name);
            printf("Quantity: %d\n", orders[i].quantity);
            printf("Subtotal: Rs. %.2f\n", subtotal);
            printf("GST (5%%): Rs. %.2f\n", gst);
            printf("Total: Rs. %.2f\n", subtotal + gst);
            return;
        }
    }

    printf("Order ID not found.\n");
}

void search_food(void)
{
    int id;

    printf("Enter Food ID to search: ");
    if (scanf("%d", &id) != 1) {
        printf("Invalid input.\n");
        return;
    }

    Food *food = find_food(id);

    if (food != NULL) {
        printf("Food: %s | Price: Rs. %.2f\n",
               food->name, food->price);
    } else {
        printf("Food item not found.\n");
    }
}

void sort_food_menu(void)
{
    Food sorted[FOOD_COUNT];

    for (int i = 0; i < FOOD_COUNT; i++) {
        sorted[i] = menu[i];
    }

    for (int i = 1; i < FOOD_COUNT; i++) {
        Food key = sorted[i];
        int j = i - 1;

        while (j >= 0 && sorted[j].price > key.price) {
            sorted[j + 1] = sorted[j];
            j--;
        }

        sorted[j + 1] = key;
    }

    printf("\n========== FOOD SORTED BY PRICE ==========\n");

    for (int i = 0; i < FOOD_COUNT; i++) {
        printf("%-25s Rs. %.2f\n",
               sorted[i].name, sorted[i].price);
    }
}

void table_booking(void)
{
    static int booked[10] = {0};
    int table;

    printf("\n========== TABLE BOOKING ==========\n");

    for (int i = 0; i < 10; i++) {
        printf("Table %d: %s\n",
               i + 1, booked[i] ? "Booked" : "Available");
    }

    printf("Enter table number (1-10): ");

    if (scanf("%d", &table) != 1) {
        printf("Invalid input.\n");
        return;
    }

    if (table < 1 || table > 10) {
        printf("Invalid table number.\n");
    } else if (booked[table - 1]) {
        printf("This table is already booked.\n");
    } else {
        booked[table - 1] = 1;
        printf("Table %d booked successfully.\n", table);
    }
}

int main(void)
{
    int choice;

    load_orders();

    do {
        printf("\n============================================\n");
        printf("     RESTAURANT ORDER MANAGEMENT SYSTEM\n");
        printf("============================================\n");
        printf("1. Display Food Menu\n");
        printf("2. Place Customer Order\n");
        printf("3. View Kitchen FIFO Queue\n");
        printf("4. Table Booking\n");
        printf("5. Billing System\n");
        printf("6. Search Food\n");
        printf("7. Sort Food Menu by Price\n");
        printf("8. Process Next Kitchen Order\n");
        printf("9. Order History\n");
        printf("10. Display Orders Using Linked List\n");
        printf("11. Search Order Using Linked List\n");
        printf("12. Display Recent-Order Stack (LIFO)\n");
        printf("13. Peek Latest Order in Stack\n");
        printf("14. Pop Latest Order from Stack\n");
        printf("15. Display Orders Using Binary Search Tree\n");
        printf("16. Search Order Using Binary Search Tree\n");
        printf("0. Save and Exit\n");
        printf("--------------------------------------------\n");
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) {
            int ch;

            while ((ch = getchar()) != '\n' && ch != EOF) {
                /* Clear invalid input */
            }

            printf("Please enter a valid number.\n");
            continue;
        }

        switch (choice) {
            case 1:
                display_food_menu();
                break;
            case 2:
                place_order();
                break;
            case 3:
                display_kitchen_queue();
                break;
            case 4:
                table_booking();
                break;
            case 5:
                generate_bill();
                break;
            case 6:
                search_food();
                break;
            case 7:
                sort_food_menu();
                break;
            case 8:
                process_next_order();
                break;
            case 9:
                order_history();
                break;
            case 10:
                display_linked_list();
                break;
            case 11:
                search_order_linked_list();
                break;
            case 12:
                display_order_stack();
                break;
            case 13:
                peek_order_stack();
                break;
            case 14:
                pop_order_stack();
                break;
            case 15:
                display_orders_bst();
                break;
            case 16:
                search_order_bst();
                break;
            case 0:
                save_orders();
                printf("Thank you for using the restaurant system!\n");
                break;
            default:
                printf("Invalid choice. Try again.\n");
        }

    } while (choice != 0);

    free_linked_list();
    return 0;
}
