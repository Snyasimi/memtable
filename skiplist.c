#include "skiplist.h"

#include <stdlib.h>
#include <string.h>

static inline int generate_random_level(float probability, int max_level) {
    int level = 1;

    while (rand() / (double)RAND_MAX < probability && level < max_level) {
        level++;
    }

    return level;
}

int skiplist_new(skiplist_t** list, float probability, int max_level) {
    if (!list || probability <= 0.0f || probability >= 1.0f || max_level == 0) {
        return -1;
    }

    skiplist_t* new_list = (skiplist_t*)malloc(sizeof(skiplist_t));
    if (!new_list) return -1;

    size_t node_size = sizeof(skiplist_node_t) + (sizeof(skiplist_node_t*) * max_level);

    skiplist_node_t* head = (skiplist_node_t*)malloc(node_size);
    if (!head) {
        free(new_list);
        return -1;
    }

    for (int i = 0; i < max_level; i++) {
        head->forward[i] = NULL;
    }

    new_list->head = head;
    new_list->current_level = 1;
    new_list->probability = probability;
    new_list->max_level = max_level;

    *list = new_list;

    return 0;
}

skiplist_node_t* skiplist_create_node(skiplist_t* list, uint8_t* key, uint32_t key_size,
                                      uint8_t* value, uint32_t value_size, uint64_t sequence,
                                      int level, int is_delete) {
    if (!list || !key || key_size == 0 || !value || value_size == 0 || sequence == 0 ||
        level == 0) {
        return NULL;
    }

    size_t node_size = sizeof(skiplist_node_t) + (level * sizeof(skiplist_node_t*));

    skiplist_node_t* node = malloc(node_size);
    if (!node) return NULL;

    for (int i = 0; i < level; i++) {
        node->forward[i] = NULL;
    }

    uint8_t* key_buffer = (uint8_t*)malloc((size_t)key_size);
    if (!key_buffer) {
        free(node);
        return NULL;
    }

    memcpy(key_buffer, key, key_size);
    node->key = key;
    node->key_size = key_size;
    node->sequence = sequence;
    node->flags = 0;

    if (is_delete == 0) {
        uint8_t* value_buffer = (uint8_t*)malloc((size_t)value_size);
        if (!value_buffer) {
            free(node);
            free(key_buffer);

            return NULL;
        }

        memcpy(value_buffer, value, value_size);

        node->value = value;
        node->value_size = value_size;
    } else {
        node->value = NULL;
        node->value_size = 0;
        node->flags |= TOMBSTONE_KEY;
    }

    return node;
}

skiplist_node_t* skiplist_get_predecesor(skiplist_t* list, uint8_t* key, uint32_t key_size,
                                         uint64_t sequence, skiplist_node_t** update) {
    (void)sequence;

    if (!list || !key || key_size == 0) {
        return NULL;
    }

    skiplist_node_t* current = list->head;
    for (int i = list->current_level - 1; i >= 0; i--) {
        while (current->forward[i] && memcmp(current->forward[i]->key, key, key_size) < 0) {
            current = current->forward[i];
        }

        if (update) update[i] = current;
    }

    return current;
}

int skiplist_get(skiplist_t* list, uint8_t* key, uint32_t key_size, uint8_t** value,
                 uint32_t* value_size, uint64_t sequence) {
    if (!list || !key || key_size == 0 || !value || sequence == 0) {
        return -1;
    }

    skiplist_node_t* pred = skiplist_get_predecesor(list, key, key_size, sequence, NULL);
    if (!pred || !pred->forward[0]) {
        *value = NULL;
        return -1;
    }

    skiplist_node_t* target = pred->forward[0];
    if (!(target->flags & TOMBSTONE_KEY)) {
        if (memcmp(target->key, key, key_size) == 0 && target->sequence == sequence) {
            uint8_t* value_bufer = (uint8_t*)malloc(target->value_size);
            if (!value_bufer) return -1;

            memcpy(value_bufer, target->value, target->value_size);
            *value = value_bufer;
            *value_size = target->value_size;

            return 0;
        }
    }

    return SKIPLIST_ERR_NOT_FOUND;
}

int skiplist_put(skiplist_t* list, uint8_t* key, uint32_t key_size, uint8_t* value,
                 uint32_t value_size, uint64_t sequence) {
    if (!list || !key || key_size == 0 || !value || value_size == 0 || sequence == 0) {
        return -1;
    }

    skiplist_node_t* update[list->max_level];

    skiplist_node_t* pred = skiplist_get_predecesor(list, key, key_size, sequence, update);
    if (!pred) return -1;

    skiplist_node_t* target = pred->forward[0];
    if (target && memcmp(target->key, key, key_size) == 0 && target->sequence == sequence) {
        uint8_t* new_val = (uint8_t*)malloc((size_t)value_size);
        if (!new_val) return -1;

        memcpy(new_val, value, value_size);
        target->value = new_val;
        target->value_size = value_size;
    }

    int node_level = generate_random_level(list->probability, list->max_level);

    if (node_level > list->current_level) {
        for (int i = list->current_level; i < node_level; i++) {
            update[i] = list->head;
        }

        list->current_level = node_level;
    }

    skiplist_node_t* new_node =
            skiplist_create_node(list, key, key_size, value, value_size, sequence, node_level, 0);
    if (!new_node) return -1;

    for (int i = 0; i < list->current_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    return 0;
}
