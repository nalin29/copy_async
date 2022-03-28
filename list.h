#include <stdio.h>
#include <stdlib.h>
#include "copy.h"
/*
*  Create data structures for the recursion and management of files
*  Linked List For current I/O events
*  Queue for next Files/Directories
*/

struct node;
struct list;

struct node{
   void* data;
   struct node* next;
   struct node* prev;
   struct list* list;
};

struct list{
   int size;
   struct node* head;
   struct node* tail;
};

void enq(struct list* l, void * data){
   struct node* new_node;
   new_node =  (struct node *)malloc(sizeof (new_node));
   CHECK_NULL(new_node, "allocating node memory");
   new_node->idx = l->size;
   new_node->list = l;
   new_node->data = data;
   if(l->size == 0){
      l->head = new_node;
      l->tail = new_node;

      new_node->next = new_node;
      new_node->prev = new_node;
   }
   else{
      new_node->next = l->head;
      new_node->prev = l->tail;

      struct node* tail = l->tail;
      struct node* head = l->head;

      tail->next = new_node;
      head->prev = new_node;

      l->tail = new_node;
   }
   l->size += 1;
}

void* deq(struct list* l){
   if(l->size == 0)
      return NULL;
   struct node *ret = l->head;
   if(l->size == 1){
      l->head = NULL;
      l->tail = NULL
   }
   else{
      l->head = ret->next;
      ret->next->prev = ret->prev;
      ret->prev->next = ret->next;
   }
   l->size -= 1;
   void * data = ret->data;
   free(ret);
   return data;  
}

// for list of size 0 or prev is the end of list enq
void add(struct list* l, struct node* prev, void *data){
   if(size == 0 || prev == l->tail){
      enq(l, data);
      return;
   }
   struct node* new_node;
   new_node =  (struct node *)malloc(sizeof (new_node));
   CHECK_NULL(new_node, "allocating node memory");
   new_node->list = l;
   new_node->data = data;
   new_node->next = prev->next;
   new_node->prev = prev;
   
   prev->next = new_node;
   new_node->next->prev = new_node;
}

void* remove(struct list* l, struct node* n){
   if(size == 0)
      return NULL;
   if(size == 1 || n == l->head){
      void* data = deq(l);
      return data;
   }
   
   n->prev->next = n->next;
   n->next->prev = n->prev;

   if(n == l->tail){
      l->tail = n->prev;
   }

   l->size -= 1;

   void* data = n->data;
   free(n);
   return data;

}

void* replace(struct node* n, void * data){
   void* old = n->data;
   n->data = data;
   return old;
}