#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>


extern struct XMLNode ** getElementsByTagName(char *name, int*);
extern struct XMLattributeNode** getAttributeNodes(struct XMLNode*, int*);
extern struct XMLNode** getChildrenNodes(struct XMLNode*, int*);
extern struct XMLNode* getParent(struct XMLNode*);
extern struct XMLNode* firstChild(struct XMLNode*);
extern struct XMLNode* lastChild(struct XMLNode*);
extern struct XMLNode* nextSibling(struct XMLNode*);
extern struct XMLNode* previousSibling(struct XMLNode*);
extern char* getAttributeValue(struct XMLNode*, char *name);

typedef struct XMLNode{
    int type;
    char* name;
    char* value;
    struct XMLNode* next;
    struct XMLNode* prev;
    struct XMLNode* children;
    int childrenCount;
    struct XMLattributeNode* attributes;
     int attributeCount;
    struct XMLNode* parent;
    char *id;
} *XMLNodeP;

struct XMLattributeNode{
    char* key;
    char* value;
    struct XMLattributeNode* next;
    struct XMLattributeNode* prev;
} *XMLattributeNodeP;

typedef struct XMLprolog{
    char *version;
    char *encoding;
    char *standlone;
} *XMLprologP;


typedef struct node{
    struct XMLNode* key;
    struct node* next;
} *Node;

typedef struct queue{
    Node root;
    Node tail;
    size_t size;
} *Queue;


/*
 Queue api
*/
extern Queue Queue_init();

extern Node enqueue(struct XMLNode* key, Queue q);

extern void _enqueue(Node x, Queue q);

extern Node dequeue(Queue q);

extern Node top(Queue q);

extern int isEmpty(Queue q);

/*
Stack api
*/

typedef struct stack{
    Node root;
    Node tail;
    size_t size;
} *Stack;
extern Stack Stack_init();

extern Node push(struct XMLNode* key, Stack q);

extern void _push(Node x, Stack q);

extern Node pop(Stack q);

extern Node stack_top(Stack q);

extern int stack_isEmpty(Stack q);
