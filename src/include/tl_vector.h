#ifndef __TL_VECTOR_H__
#define __TL_VECTOR_H__

struct VectorElement {
    void* element;

    void(*free_fun)(void*);
    
    struct VectorElement* next;
    struct VectorElement* previous;
};

struct Vector {
    struct VectorElement* first;
    struct VectorElement* last;
    struct VectorElement* iterator;
};

void vector_init(struct Vector* v);
void vector_append(struct Vector* v, void* e, void(*free_fun)(void*));
void vector_free(struct Vector* v);
void* vector_get(struct Vector* v);
void* vector_first(struct Vector* v);
void* vector_last(struct Vector* v);
void* vector_next(struct Vector* v);
void* vector_previous(struct Vector* v);

#endif