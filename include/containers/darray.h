#pragma once

#include "defines.h"

/*
 * Memory layout:
 * u64 capacity = number of elements that can be held
 * u64 length = number of elements currently contained
 * u64 stride = size of each element in bytes
 * void *elements
 */

enum {
  DARRAY_CAPACITY,
  DARRAY_LENGTH,
  DARRAY_STRIDE,
  DARRAY_FIELD_LENGTH,
};

KAPI void *darray_create_(u64 length, u64 stride);
KAPI void darray_destroy_(void *array);

KAPI u64 darray_field_get_(void *array, u64 field);
KAPI void darray_field_set_(void *array, u64 field, u64 value);

KAPI void darray_resize_(void **array);

KAPI void darray_push_(void **array, const void *value_ptr);
KAPI void darray_pop_(void *array, void *dest);

KAPI void darray_remove_(void *array, u64 index, void *dest);
KAPI void darray_insert_(void **array, u64 index, void *value_ptr);

#define DARRAY_DEFAULT_CAPACITY 8
#define DARRAY_RESIZE_FACTOR 2

#define darray_create(type)                                                    \
  darray_create_(DARRAY_DEFAULT_CAPACITY, sizeof(type))

#define darray_with_capacity(type, capacity)                                   \
  darray_create_(capacity, sizeof(type))

#define darray_destroy(array) darray_destroy_(array);

#define darray_push(array, value)                                              \
  {                                                                            \
    typeof(value) temp = (value);                                              \
    darray_push_((void **)(array), &temp);                                     \
  }

#define darray_pop(array, value_ptr) darray_pop_(array, value_ptr)

#define darray_insert(array, index, value)                                     \
  {                                                                            \
    typeof(value) temp = (value);                                              \
    darray_insert_((array), (index), &temp);                                   \
  }

#define darray_remove(array, index, value_ptr)                                 \
  darray_remove_(array, index, value_ptr)

#define darray_clear(array) darray_field_set_(array, DARRAY_LENGTH, 0)

#define darray_capacity(array) darray_field_get_(array, DARRAY_CAPACITY)

#define darray_length(array) darray_field_get_(array, DARRAY_LENGTH)

#define darray_stride(array) darray_field_get_(array, DARRAY_STRIDE)

#define darray_length_set(array, length)                                       \
  darray_field_set_(array, DARRAY_LENGTH, length)

#define darray_for_each(array, value_ptr)                                      \
  for ((value_ptr) = array; (value_ptr) < (array) + darray_length(array);      \
       (value_ptr)++)

#define darray_enum_for_each(array, value_ptr, index)                          \
  for ((value_ptr) = (array), (index) = 0;                                     \
       (value_ptr) < (array) + darray_length(array); (value_ptr)++, (index)++)
