#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../skiplist.h"

static inline int generate_random_level(float probability, int max_level) {
    int level = 1;

    while (rand() / (double)RAND_MAX < probability && level < max_level) {
        level++;
    }

    return level;
}

void test_skiplist_new() {
    skiplist_t* list = NULL;
    int max_level = 8;
    float probability = 0.5;

    int rc = skiplist_new(&list, probability, max_level);
    assert(rc == 0);
    assert(list->current_level = 1);
    assert(list->max_level = max_level);
    assert(list->probability = probability);
    assert(list->head != NULL);

    skiplist_node_t* head = list->head;

    for (int i = 0; i < max_level; i++) {
        assert(head->forward[i] == NULL);
    }

    list = NULL;
    max_level = 0;
    probability = 1.0;

    assert(skiplist_new(&list, probability, max_level) == -1);
}

void test_skiplist_create_node() {
    skiplist_t* list = NULL;
    int max_level = 8;
    float probability = 0.5;
    uint8_t* key = (uint8_t*)"key";
    uint8_t* value = (uint8_t*)"value";
    uint64_t sequence = 1;
    int is_delete = 0;
    int level = generate_random_level(probability, max_level);

    assert(skiplist_new(&list, probability, max_level) == 0);

    skiplist_node_t* node = skiplist_create_node(list, key, strlen((char*)key), value,
                                                 strlen((char*)value), sequence, level, is_delete);

    assert(node != NULL);

    int fwd_pointer_count = 0;
    for (int i = 0; i < level; i++) {
        fwd_pointer_count++;
    }

    assert(fwd_pointer_count == level);
    assert(memcmp(node->key, key, node->key_size) == 0);
    assert(memcmp(node->value, value, node->value_size) == 0);

    is_delete = 1;
    node = skiplist_create_node(list, key, strlen((char*)key), value, strlen((char*)value),
                                sequence, level, is_delete);

    assert(node->flags & TOMBSTONE_KEY);
    assert(node->value == NULL);
    assert(node->value_size == 0);
}

void test_skiplist_put() {
    skiplist_t* list = NULL;
    int max_level = 8;
    float probability = 0.5;
    uint8_t* key = (uint8_t*)"key";
    uint8_t* value = (uint8_t*)"value";
    uint64_t sequence = 1;

    assert(skiplist_new(&list, probability, max_level) == 0);

    assert(skiplist_put(list, key, strlen((char*)key), value, strlen((char*)value), sequence) == 0);

    uint8_t* ret_value = NULL;
    uint32_t value_size;

    assert(skiplist_get(list, key, strlen((char*)key), &ret_value, &value_size, sequence) == 0);
    assert(memcmp(value, ret_value, value_size) == 0);

    value = (uint8_t*)"new_value";
    ret_value = NULL;
    assert(skiplist_put(list, key, strlen((char*)key), value, strlen((char*)value), sequence) == 0);

    assert(skiplist_get(list, key, strlen((char*)key), &ret_value, &value_size, sequence) == 0);
    assert(memcmp(value, ret_value, value_size) == 0);
}

int main(void) {
    test_skiplist_new();
    test_skiplist_create_node();
    test_skiplist_put();
}
