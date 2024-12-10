#include <stdio.h>
#include <stdlib.h>
#include "../lib/queue.h"
#include "../lib/job.h"

struct queue {
    QueueNode dummy;
    QueueNode last;
    int length;
};

struct queue_node {
    QueueNode next;
    void* value;
};

Queue queue_create() {
    Queue queue = malloc(sizeof(*queue));
    queue->length = 0;
    queue->dummy = malloc(sizeof(*queue->dummy));
    queue->dummy->next = NULL;

    queue->last = queue->dummy;

    return queue;
}


int queue_length(Queue queue) {
    return queue->length;
}

int queue_insert_last(Queue queue, void* value) {
    // Store last node before insertion.
    QueueNode previousLast = queue->last;

    // Create node for creation
    QueueNode new = malloc(sizeof(*new));
    new->next = NULL;
    new->value = value;

    // Update links and length.
    previousLast->next = new;
    queue->last = new;
    queue->length++;


    return 0;
}

int queue_remove_first(Queue queue) {
    // Store node to be removed.
    QueueNode removed = queue->dummy->next;
    if(removed == NULL) {
        return -1; 
    }
    // Update dummy link to second node(now first) and length.
    queue->dummy->next = removed->next; // If next == NULL it is correct for empty list.
    queue->length--;

    // Update last if needed.
    if(queue->last == removed) {
        queue->last = queue->dummy;
    }
    
    // Free memory from node.
    free(removed);
    return 0;
}

int queue_destroy(Queue queue) {
    free(queue);
    return 0;
}

QueueNode queue_first(Queue queue) {
	return queue->dummy->next;
}

QueueNode queue_next(Queue queue, QueueNode node) {
	return node->next;
}

QueueNode queue_last(Queue queue) {
	if (queue->last == queue->dummy)
		return NULL;
	else
		return queue->last;
}

void* queue_node_value(Queue queue, QueueNode node) {
	return node->value;
}

int queue_remove_value(Queue queue, void* removeVal) {
    QueueNode node = queue_first(queue);

    // Element is first in queue
    if((node->value) == removeVal) {
        if(queue_remove_first(queue) != 0) {
            return -1;
        }
        return 0;
    }

    // Element is in the middle
    QueueNode node2 = node->next;
    while(node2->next != NULL) {
        if((node2->value) == removeVal) {
            node->next = node2->next;
            queue->length--;    
            free(node2);
            return 0;
        }
        node = node2; // Node is always one step behind node2
        node2 = node2->next;
    }

    // Element may be the last
    if((node2->value) == removeVal) {
        node->next = node2->next;
        free(node2);
        queue->length--;
        return 0;
    }

    // Value not found.
    return -1;
    }

int queue_remove_pid(Queue queue, int pid) {
    QueueNode node;
    for(node = queue_first(queue); node != NULL; node = queue_next(queue, node)) {
        Job* job = queue_node_value(queue, node);
        if(job->queuePos == pid) {
            queue_remove_value(queue, job);
            return 0;
        }
    }
    return -1;
}