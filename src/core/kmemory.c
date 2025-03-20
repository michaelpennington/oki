#include "core/kmemory.h"

#include "core/asserts.h"
#include "core/logger.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>

struct memory_stats {
  u64 total_allocated;
  u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

static const char *memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN    ", "ARRAY      ", "DARRAY     ", "DICT       ", "RING_QUEUE ",
    "BST        ", "STRING     ", "APPLICATION", "JOB        ", "TEXTURE    ",
    "MAT_INST   ", "RENDERER   ", "GAME       ", "TRANSFORM  ", "ENTITY     ",
    "ENTITY_NODE", "SCENE      ",
};

static struct memory_stats stats;

u64 memory_field_get(void *memory, u64 field) {
  kassert_debug_msg(memory, "Tried to get memory field of nullptr");
  u64 *header = (u64 *)memory - MEMORY_FIELD_LENGTH;
  kassert_debug_msg(field < MEMORY_FIELD_LENGTH,
                    "Tried to get invalid memory field");
  return header[field];
}

void memory_field_set(void *memory, u64 field, u64 val) {
  kassert_debug_msg(memory, "Tried to set memory field of nullptr");
  u64 *header = (u64 *)memory - MEMORY_FIELD_LENGTH;
  kassert_debug_msg(field < MEMORY_FIELD_LENGTH,
                    "Tried to set invalid memory field");
  header[field] = val;
}

void initialize_memory() { platform_zero_memory(&stats, sizeof(stats)); }

void shutdown_memory() {}

void *kallocate(u64 size, memory_tag tag) {
  if (tag == MEMORY_TAG_UNKNOWN) {
    kwarn(
        "kallocate called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
  }
  stats.total_allocated += size;
  stats.tagged_allocations[tag] += size;

  // TODO: Memory alignment
  void *block =
      platform_allocate(size + (MEMORY_FIELD_LENGTH * sizeof(u64)), false);
  kassert_debug(block);
  platform_zero_memory(block, size);
  block = (void *)((u64 *)block + MEMORY_FIELD_LENGTH);
  memory_field_set(block, MEMORY_FIELD_TAG, tag);
  memory_field_set(block, MEMORY_FIELD_SIZE, size);
  return block;
}

void kfree(void *block) {
  memory_tag tag = memory_field_get(block, MEMORY_FIELD_TAG);
  if (tag == MEMORY_TAG_UNKNOWN) {
    kwarn("kfree called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
  }
  u64 size = memory_field_get(block, MEMORY_FIELD_SIZE);
  stats.total_allocated -= size;
  stats.tagged_allocations[tag] -= size;
  // TODO: Memory alignment
  platform_free((u64 *)block - MEMORY_FIELD_LENGTH, false);
}

void *kzero_memory(void *block, u64 size) {
  return platform_zero_memory(block, size);
}

void *kcopy_memory(void *dest, const void *source, u64 size) {
  return platform_copy_memory(dest, source, size);
}

void *kset_memory(void *dest, i32 value, u64 size) {
  return platform_set_memory(dest, value, size);
}

#define BUFFER_SIZE 8000

void print_memory_usage_str() {
  const u64 gib = 1024UL * 1024 * 1024;
  const u64 mib = 1024UL * 1024;
  const u64 kib = 1024;

  char buffer[BUFFER_SIZE] = "System memory use (tagged):\n";
  u64 offset = strlen(buffer);
  for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
    char unit[4] = "XiB";
    f32 amount = 1.0F;

    if (stats.tagged_allocations[i] >= gib) {
      unit[0] = 'G';
      amount = (f32)stats.tagged_allocations[i] / (f32)gib;
    } else if (stats.tagged_allocations[i] >= mib) {
      unit[0] = 'M';
      amount = (f32)stats.tagged_allocations[i] / (f32)mib;
    } else if (stats.tagged_allocations[i] >= kib) {
      unit[0] = 'K';
      amount = (f32)stats.tagged_allocations[i] / (f32)kib;
    } else {
      unit[0] = 'B';
      unit[1] = '\0';
      amount = (f32)stats.tagged_allocations[i];
    }

    i32 length =
        snprintf(buffer + offset, BUFFER_SIZE - offset, "  %s: %.2f%s\n",
                 memory_tag_strings[i], amount, unit);
    offset += length;
  }

  kinfo(buffer);
}
