#include <stdlib.h>
#include "include\tl_vector.h"

void vector_init(struct Vector* v) {
    v->first = NULL;
    v->last = NULL;
    v->iterator = NULL;
}

void vector_append(struct Vector* v, void* e, void(*free_fun)(void*)) {
    struct VectorElement* new_element = malloc(sizeof(struct VectorElement));

    new_element->element = e;
    new_element->free_fun = free_fun;

    if (v->first == NULL) {
        v->first = v->last = new_element;
        new_element->next = new_element->previous = NULL;
    } else {
        new_element->next = NULL;
        v->last->next = new_element;

        new_element->previous = v->last;

        v->last = new_element;
    }
}

void vector_free(struct Vector* v) {
    while(v->first != NULL) {
        struct VectorElement* el_tmp = v->first->next;

        v->first->free_fun(v->first->element);
        free(v->first);

        v->first = el_tmp;
    }

    v->last = NULL;
    v->iterator = NULL;
}


void* vector_get(struct Vector* v) {
    return v->iterator == NULL ? NULL : v->iterator->element;
}

void* vector_first(struct Vector* v) {
    v->iterator = v->first;

    return vector_get(v);
}

void* vector_last(struct Vector* v) {
    v->iterator = v->last;

    return vector_get(v);
}

void* vector_next(struct Vector* v) {
    if (v->iterator != NULL)
        v->iterator = v->iterator->next;

    return vector_get(v);
}

void* vector_previous(struct Vector* v) {
    if (v->iterator != NULL)
        v->iterator = v->iterator->previous;

    return vector_get(v);
}
