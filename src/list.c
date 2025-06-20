#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <list.h>

ArrayList *create_list() {
    ArrayList *list = calloc(1, sizeof(*list));
    return list;
}

ArrayList *alloc_list(size_t element_size) {
    ArrayList *list = create_list();
    if(!list) return NULL;
    list->elements = calloc(256, element_size);
    if(!(list->elements)) {
        free(list);
        return NULL;
    }
    list->len = 256;
    list->size_elements = element_size;
    return list;
}

void append_element(ArrayList *list, void *new_element) {
    if(!list) return;
    if(list->count == list->len) {
        void *check = realloc(list->elements, list->size_elements * list->len * 2);
        if(!check) return;
        list->elements = check;
        list->len *= 2;
    }
    //if you want to use pointers like int *a, you'd still need to pass as a ref &a
    for(size_t i = 0; i < list->size_elements; i++)
        *((byte *)list->elements + i + (list->size_elements * list->count)) = *(i + (byte *)new_element);
    list->count += 1;
}

void append_many(ArrayList *list, void *elements, size_t elements_count, size_t elements_size) {
    if(!list) return;
    if(elements_size != list->size_elements) return;
    if(list->count + elements_count >= list->len) {
        while(list->len < list->count + elements_count) {
            void *check = realloc(list->elements, list->size_elements * list->len * 2);
            if(!check) return;
            list->elements = check;
            list->len *= 2;
        }
    }
    for(size_t i = 0; i < elements_count; i++)
        append_element(list, (void *)(elements + (i * elements_size)));
}

ArrayList *concat_lists(ArrayList *l1, ArrayList *l2) {
    if(l1->size_elements != l2->size_elements) return NULL; //err check;
    ArrayList *lret = create_list();
    if(!lret) return NULL;
    uint64_t n_size = l1->count + l2->count;
    uint64_t i;
    for(i = 1; i < n_size; i *= 2);
    if(i < 256) i = 256;
    byte *a = calloc(i, l1->size_elements);
    if(!a) {
        free(lret);
        return NULL;
    }
    for(uint64_t j = 0; j < l1->size_elements * l1->count; j++)
        a[j] = *(j + (byte*)l1->elements);
    for(uint64_t j = 0; j < l2->size_elements * l2->count; j++)
        a[l1->count * l1->size_elements + j] = *(j + (byte*)l2->elements);
    lret->elements = (void *)a;
    lret->size_elements = l1->size_elements;
    lret->count = n_size;
    lret->len = i;
    return lret;
}

bool is_list_empty(ArrayList list) {
    return (list.count == 0);
}

void remove_at(ArrayList *list, size_t pos) {
    for(int i = pos + 1; i < list->count; i++) {
        for(int j = 0; j < list->size_elements; j++) {
            *((i-1) * list->size_elements + j + (byte*)list->elements) = *(i * list->size_elements + j + (byte*)list->elements);
        }
    }
    list->count--;
}

//TODO: pop dequeue remove_at remove

void free_list(ArrayList *list) {
    free(list->elements);
    list->elements = 0;
    list->len = 0;
    list->size_elements = 0;
    free(list);
}
