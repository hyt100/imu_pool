#include "imupool.h"
#include <stdlib.h>
#include <string.h>

/* 编译器memory barrier */
#define barrier() __asm__ __volatile__("": : :"memory")
//#define ACCESS_ONCE(x)    (*(volatile typeof(x) *)&(x))

//跳过开始的若干帧，防止写端追赶上读操作的内存区域，读端可以不用加锁保护
#define INGORE_FRAMES   2

const char *imupool_strerr(int err)
{
  switch (err)
  {
    case IMUPOOL_NORMAL:     return "(Success)";
    case IMUPOOL_BUSY:       return "(Busy)";
    case IMUPOOL_NO_DATA:    return "(No Data)";
    case IMUPOOL_PARA_ERROR: return "(Para Error)";
    default:                 return "(Unknow)";
  }
}

imupool_t* imupool_create(int num)
{
  if (num <= 0)
    return NULL;
    
  imupool_t* pool = (imupool_t*)calloc(1, sizeof(imupool_t));
  if (!pool)
    return NULL;

  pool->items = (imu_item_t*)calloc(num, sizeof(imu_item_t));
  if (!pool->items)
  {
    free((void *)pool);
    return NULL;
  }

  pool->num = num;
  pool->index = -1;
  pool->used = 0;
  pool->operate = 0;
  return pool;
}

void imupool_destroy(imupool_t* pool)
{
  if (pool)
  {
    if (pool->items)
      free((void *)pool->items);
    free((void *)pool);
  }
}

int imupool_insert(imupool_t* pool, imu_item_t *item)
{
  if (!pool || !item)
    return IMUPOOL_PARA_ERROR;
  
  //wait in loop
  while (pool->operate);

  int next = (pool->index + 1) % pool->num;
  memcpy(&pool->items[next], item, sizeof(imu_item_t));
  pool->index = next;
  
  barrier();  //comiler memory barrier
  
  if (pool->used < pool->num)
    pool->used++;
  return IMUPOOL_NORMAL;
}

static int _binary_search_closest(imupool_t *pool, int low, int high, uint64_t target, int flag)
{
  if (target <= pool->items[low % pool->num].timestamp)
    return low;
  if (target >= pool->items[high % pool->num].timestamp)
    return high;

  int mid;
  while (high - low > 1)
  {
    mid = low + (high - low) / 2;
    if (target > pool->items[mid % pool->num].timestamp)
      low = mid;
    else if (target < pool->items[mid % pool->num].timestamp)
      high = mid;
    else
      return mid;
  }

  if (flag == 1)
    return high;
  if (flag == 2)
    return low;

  int delta_left  = IMUPOOL_ABS(target, pool->items[low % pool->num].timestamp);
  int delta_right = IMUPOOL_ABS(target, pool->items[high % pool->num].timestamp);

  if (delta_right <= delta_left)
    return high;
  else
    return low;
}

static int _binary_search_range(imupool_t *pool, int low, int high, 
                uint64_t target_begin, uint64_t target_end, 
                int *out_left, int *out_right)
{
  if (target_begin > pool->items[high % pool->num].timestamp || target_end < pool->items[low % pool->num].timestamp)
    return -1;
  *out_left  = _binary_search_closest(pool, low, high, target_begin, 1);
  *out_right = _binary_search_closest(pool, low, high, target_end, 2);
  return 0;
}

int imupool_request(imupool_t *pool, uint64_t reqns, uint64_t tolns, imu_item_t *out)
{
  if (!pool || !out)
    return IMUPOOL_PARA_ERROR;
  if (pool->used <= INGORE_FRAMES)
    return IMUPOOL_NO_DATA;

  int cur_index = pool->index;
  int low, high;
  if (pool->used < pool->num)
  {
    low = 0 + INGORE_FRAMES;
    high = cur_index;
  }
  else
  {
    low = cur_index + 1 + INGORE_FRAMES;
    high = pool->num + cur_index;
  }
  uint64_t beginns = (reqns > tolns) ? (reqns-tolns) : 0;
  uint64_t endns = reqns + tolns;

  pool->operate = 1;
  int search_index = _binary_search_closest(pool, low, high, reqns, 0);
  uint64_t val = pool->items[search_index % pool->num].timestamp;
  if (beginns <= val && val <= endns)
  {
    memcpy(out, &pool->items[search_index % pool->num], sizeof(imu_item_t));
    pool->operate = 0;
    return IMUPOOL_NORMAL;
  }

  pool->operate = 0;
  return IMUPOOL_NO_DATA;
}

int imupool_range_request(imupool_t *pool, uint64_t beginns, uint64_t endns, imu_item_t *out_items, int *out_num)
{
  if (!pool || !out_items || !out_num || beginns >= endns)
    return IMUPOOL_PARA_ERROR;
  if (pool->used <= INGORE_FRAMES)
    return IMUPOOL_NO_DATA;

  int cur_index = pool->index;
  int low, high;
  if (pool->used < pool->num)
  {
    low = 0 + INGORE_FRAMES;
    high = cur_index;
  }
  else
  {
    low = cur_index + 1 + INGORE_FRAMES;
    high = pool->num + cur_index;
  }
  int left = 0, right = 0;

  pool->operate = 1;
  if (_binary_search_range(pool, low, high, beginns, endns, &left, &right) != 0)
  {
    pool->operate = 0;
    return IMUPOOL_NO_DATA;
  }
  
  int num = right - left + 1;
  int end_space = pool->num - (left % pool->num);

  /* first get the data form OUT until the end of the buffer */
  int len = IMUPOOL_MIN(num, end_space);
  memcpy(out_items, &pool->items[left % pool->num], len * sizeof(imu_item_t));
  /* then get the rest (if any) from the beginning of the buffer */
  if (num > len)
    memcpy(out_items+len, &pool->items[0], (num - len) * sizeof(imu_item_t));

  *out_num = num;  
  pool->operate = 0;
  return IMUPOOL_NORMAL;
}

int imupool_peek_new(imupool_t *pool, imu_item_t *out)
{
  if (!pool || !out)
    return IMUPOOL_PARA_ERROR;
  if (pool->used <= INGORE_FRAMES)
    return IMUPOOL_NO_DATA;

  int cur_index = pool->index;
  pool->operate = 1;
  memcpy(out, &pool->items[cur_index], sizeof(imu_item_t));
  pool->operate = 0;
  return IMUPOOL_NORMAL;
  return 0;
}


