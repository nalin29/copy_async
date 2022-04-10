/**
 * @file list.h
 * @author Nalin Mahajan, Vineeth Bandi
 * @brief A crude queue implementation
 * @version 0.1
 * @date 2022-04-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdio.h>
#include <stdlib.h>

struct node;
struct list;

// simple doubly linked node
struct node{
   void* data;
   struct node* next;
   struct node* prev;
};

// doubly linked list meta data block
struct list{
   int size;
   struct node* head;
   struct node* tail;
};

// enques new node to end of list, with data pointer
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

// deques node at front of the list (null if none) and returns the data pointer
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

// removes a node at desired node (return data pointer) 
// WARNING: list does not check if node is in list can cause undefined behavior
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

// swaps data at node
void* replace_node(struct node* n, void * data){
   void* old = n->data;
   n->data = data;
   return old;
}