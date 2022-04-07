#include <stdio.h>
#include <stdlib.h>
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
};

struct list{
   int size;
   struct node* head;
   struct node* tail;
};

void enq_node(struct list* l, void * data){
   struct node* new_node;
   new_node =  (struct node *)malloc(sizeof (struct node));
   if(new_node == NULL){
      perror("Error allocating memory for node");
      exit(-1);
   }
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

void* deq_node(struct list* l){
   if(l->size == 0)
      return NULL;
   struct node *ret = l->head;
   if(l->size == 1){
      l->head = NULL;
      l->tail = NULL;
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

void* remove_node(struct list* l, struct node* n){
   if(l->size == 0)
      return NULL;
   if(l->size == 1 || n == l->head){
      return deq_node(l);
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

void* replace_node(struct node* n, void * data){
   void* old = n->data;
   n->data = data;
   return old;
}