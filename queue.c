#include "ksx.h"

Queue Queue_init(){
    Queue q = malloc(sizeof(struct queue));
    Node root = malloc(sizeof(struct node));
    root->key = NULL;
    root->next = NULL;
    Node tail = root;
    q->root = root;
    q->tail = tail;
    q->size = 0;
    return q;
    
}
Node enqueue(struct XMLNode* key, Queue q){
    if(key == NULL) return;
    Node x = malloc(sizeof (struct node));
    x->key = key;
    x->next = NULL;
    _enqueue(x, q);
    q->size++;
    return q->root->next;
}

void _enqueue(Node x, Queue q){
    if(x == NULL){
        return;
    }
    q->tail->next = x;
    q->tail = q->tail->next;
};

Node dequeue(Queue q){
    if(q->root->next == NULL){
        printf("no elements in here!!!\n");

    }
    Node result = q->root->next;
    q->root->next = result->next;
    if(q->root->next == NULL) q->tail = q->root;
    q->size--;
    return result;
}

Node top(Queue q){
    if(q->root->next == NULL){
            printf("no elements in here!!!\n");
    }
    return q->root->next;
}

int isEmpty(Queue q){
    return q->size == 0;
}


