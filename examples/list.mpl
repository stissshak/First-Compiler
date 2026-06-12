extern int rand();
extern void srand(int seed);
extern int time(void *ptr);


struct node{
    int data;
    node *prev;
    node *next;
}

node *initNode(int data){
    node *ptr = malloc(sizeof(node));
    ptr->data = data;
    return ptr;
}

void pushBack(node* head, int data){
    node *tmp = head;
    if(!tmp){
        return;
    }
    while(tmp->next){
        tmp = tmp->next;
    }
    tmp->next = initNode(data);
}

void freeList(node *head){
    if(!head) return;
    while(head){
        node *tmp = head->next;
        free(head);
        head = tmp;
    }
}

void printList(node *head){
    node *tmp = head;
    while(tmp){
        print("%d ", tmp->data);
        tmp = tmp->next;
    }
    print("\n");
}

int main(){
    srand(time(null));
    int n = 6;
    node *head = initNode(5);
    for(int i = 0; i < n; ++i){
        pushBack(head, rand() % 10);
    }
    printList(head);
    freeList(head);
}