#include "config.h"
#include "esp_heap_caps.h"

void *_malloc(int count)
{
    return heap_caps_malloc(count, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}
