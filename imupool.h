#ifndef __IMUPOOL_H__
#define __IMUPOOL_H__

//#ifdef __cplusplus
//extern "C" {
//#endif  /*__cplusplus*/

#include <stdio.h>
#include <stdint.h>

enum IMUPOOL_ERR
{
  IMUPOOL_NORMAL = 0,
  IMUPOOL_BUSY,
  IMUPOOL_NO_DATA,
  IMUPOOL_PARA_ERROR,
};

typedef struct {
  uint64_t timestamp;
  //...
} imu_item_t;

typedef struct {
  imu_item_t *items;
  int num;   //the number of items

  int used;  //availabe resources (0 ~ num)
  int index; //the position of the newest resource (0 ~ num-1)
  int operate; //lock flag
} imupool_t;

#define IMUPOOL_DIFF(x, y)    (((x) >= (y)) ? ((x)-(y)) : ((y)-(x)))
#define IMUPOOL_MAX(x, y)    (((x) >= (y)) ? (x) : (y))
#define IMUPOOL_MIN(x, y)    (((x) >= (y)) ? (y) : (x))

const char *imupool_strerr(int err);

imupool_t* imupool_create(int num);
void imupool_destroy(imupool_t* pool);

int imupool_insert(imupool_t* pool, imu_item_t *item);

//request: reqns-tolns  ~  reqns+tolns
int imupool_request(imupool_t *pool, uint64_t reqns, uint64_t tolns, imu_item_t *out);
//request: beginns ~ endns
int imupool_range_request(imupool_t *pool, uint64_t beginns, uint64_t endns, imu_item_t *out_items, int *out_num);

int imupool_peek_new(imupool_t *pool, imu_item_t *out);


//#ifdef __cplusplus
//}
//#endif  /*__cplusplus*/

#endif
