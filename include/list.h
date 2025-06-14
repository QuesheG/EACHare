#ifndef _LIST_H
#define _LIST_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define byte uint8_t

typedef struct _arrlist {
    void *elements;
    size_t count;
    size_t len;
    size_t size_elements;
} ArrayList;

ArrayList *alloc_list(size_t element_size);
void append_element(ArrayList *list, void *new_element);
void append_many(ArrayList *list, void *elements, size_t elements_count, size_t elements_size);
ArrayList *concat_lists(ArrayList *l1, ArrayList *l2);
bool is_list_empty(ArrayList list);
void free_list(ArrayList *list);
#endif