#include <stdio.h>
#pragma once


typedef struct queue* Queue;
typedef struct queue_node* QueueNode;


Queue queue_create();

int queue_length(Queue queue);

int queue_insert_last(Queue queue, void* value);

int queue_remove_first(Queue queue);

int queue_remove_value(Queue queue, void* value);

int queue_remove_pid(Queue queue, int pid);

int queue_destroy(Queue queue);

QueueNode queue_first(Queue queue);

QueueNode queue_last(Queue queue);

QueueNode queue_next(Queue queue, QueueNode node);

void* queue_node_value(Queue queue, QueueNode node);
