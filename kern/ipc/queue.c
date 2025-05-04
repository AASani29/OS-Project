#include <kern/ipc/pubsub.h>
#include <lib/string.h> 
// Initialize the message queue
void init_message_queue(struct Topic *topic) {
    topic->head = 0;
    topic->tail = 0;
    topic->count = 0;
}

// Add a message to the queue
void enqueue_message(struct Topic *topic, const char *data) {
    if (topic->count == MAX_QUEUE_SIZE) {
        // Discard the oldest message
        topic->head = (topic->head + 1) % MAX_QUEUE_SIZE;
        topic->count--;
    }

    // Add the new message
    strncpy(topic->queue[topic->tail].data, data, MESSAGE_SIZE);
    topic->tail = (topic->tail + 1) % MAX_QUEUE_SIZE;
    topic->count++;
}

// Remove a message from the queue
void dequeue_message(struct Topic *topic) {
    if (topic->count > 0) {
        topic->head = (topic->head + 1) % MAX_QUEUE_SIZE;
        topic->count--;
    }
}

// kern/ipc/pubsub.c
void show_queue(const char *topic_name) {
    struct Topic *topic = find_topic(topic_name);
    if (!topic) {
        dprintf("Topic '%s' not found\n", topic_name);
        return;
    }

    if (topic->count == 0) {
        dprintf("Queue for '%s' is empty\n", topic_name);
        return;
    }

    dprintf("Messages in '%s' (%d/%d):\n", 
           topic_name, topic->count, MAX_QUEUE_SIZE);
    
    for (int i = 0; i < topic->count; i++) {
        int idx = (topic->head + i) % MAX_QUEUE_SIZE;
        dprintf("[%02d] %s\n", i+1, topic->queue[idx].data);
    }
}