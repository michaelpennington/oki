#include "containers/darray.h"
#include "core/asserts.h"
#include "core/kmemory.h"

void *darray_create_(u64 length, u64 stride) {
  u64 header_size = DARRAY_FIELD_LENGTH * sizeof(u64);
  u64 array_size = length * stride;
  u64 *new_array = kallocate(header_size + array_size, MEMORY_TAG_DARRAY);
  kset_memory(new_array, 0, header_size + array_size);
  new_array[DARRAY_CAPACITY] = length;
  new_array[DARRAY_LENGTH] = 0;
  new_array[DARRAY_STRIDE] = stride;
  return (void *)(new_array + DARRAY_FIELD_LENGTH);
}

void darray_destroy_(void *array) {
  u64 *header = (u64 *)array - DARRAY_FIELD_LENGTH;
  kfree(header);
}

u64 darray_field_get_(void *array, u64 field) {
  u64 *header = (u64 *)array - DARRAY_FIELD_LENGTH;
  return header[field];
}

void darray_field_set_(void *array, u64 field, u64 value) {
  u64 *header = (u64 *)array - DARRAY_FIELD_LENGTH;
  header[field] = value;
}

void darray_resize_(void **array) {
  u64 length = darray_length(*array);
  u64 stride = darray_stride(*array);
  void *temp =
      darray_create_(DARRAY_RESIZE_FACTOR * darray_capacity(*array), stride);
  kcopy_memory(temp, *array, length * stride);

  darray_length_set(temp, length);
  darray_destroy(*array);
  *array = temp;
}

void darray_push_(void **array, const void *value_ptr) {
  u64 length = darray_length(*array);
  u64 stride = darray_stride(*array);
  if (length >= darray_capacity(*array)) {
    darray_resize_(array);
  }

  u8 *addr = (u8 *)*array;
  addr += (length * stride);
  kcopy_memory((void *)addr, value_ptr, stride);
  darray_field_set_(*array, DARRAY_LENGTH, length + 1);
}

void darray_pop_(void *array, void *dest) {
  u64 length = darray_length(array);
  kassert_debug_msg(length > 0, "Tried to pop from array of length 0!");
  u64 stride = darray_stride(array);
  u8 *addr = (u8 *)array;
  addr += ((length - 1) * stride);
  kcopy_memory(dest, (void *)addr, stride);
  darray_field_set_(array, DARRAY_LENGTH, length - 1);
}

void darray_remove_(void *array, u64 index, void *dest) {
  u64 length = darray_length(array);
  u64 stride = darray_stride(array);
  kassert_debug_msg(index < length, "Index outside of bounds of this array!");
  u8 *addr = (u8 *)array;
  kcopy_memory(dest, (void *)(addr + (index * stride)), stride);

  if (index != length - 1) {
    kcopy_memory((void *)(addr + (index * stride)),
                 (void *)(addr + ((index + 1) * stride)),
                 stride * (length - index));
  }

  darray_field_set_(array, DARRAY_LENGTH, length - 1);
}

void darray_insert_(void **array, u64 index, void *value_ptr) {
  u64 length = darray_length(*array);
  u64 stride = darray_stride(*array);
  kassert_debug_msg(index < length, "Index out of bounds for this array!");
  if (length >= darray_capacity(*array)) {
    darray_resize_(array);
  }

  u8 *addr = (u8 *)array;
  if (index != length - 1) {
    kcopy_memory((void *)(addr + ((index + 1) * stride)),
                 (void *)(addr + (index * stride)), stride * (length - index));
  }

  kcopy_memory((void *)(addr + (index * stride)), value_ptr, stride);

  darray_field_set_(*array, DARRAY_LENGTH, length + 1);
}
