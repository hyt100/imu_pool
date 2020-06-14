#include "demo.h"
#include "imupool.h"
#include <pthread.h>



//二分法查找相近的值
//flag=0   最接近的值
//flag=1   最接近的大于目标值
//flag=2   最接近的小与目标值
int binary_search_closest(int *a, int low, int high, int target, int flag = 0)
{
  if (target <= a[low])
    return low;
  if (target >= a[high])
    return high;

  int mid;
  while (high - low > 1)
  {
    //sleepMs(500);
    mid = low + (high - low) / 2;  //二分查找
    //mid = low + (high - low) * (target - a[low]) / (a[high] - a[low]); //插值查找
    //printf("mid=%d \n", mid);
    if (target > a[mid])
      low = mid;
    else if (target < a[mid])
      high = mid;
    else
      return mid;
  }

  if (flag == 1)
    return high;
  if (flag == 2)
    return low;

  int delta_left  = IMUPOOL_DIFF(target, a[low]);
  int delta_right = IMUPOOL_DIFF(target, a[high]);

  if (delta_right <= delta_left)
    return high;
  else
    return low;
}

void test1()
{
  vector<int> a = {1,2,3,5,7,10, 14, 19, 25, 100};
  //vector<int> a = {1,2,3,4,5,6,7,8,9,10};
  cout<<a<<endl;
  //cout<< "find: 3  => " << binary_search_closest(a.data(), 0, a.size()-1, 3) << endl;
  //cout<< "find: 4  => " << binary_search_closest(a.data(), 0, a.size()-1, 4) << endl;
  //cout<< "find: 8  => " << binary_search_closest(a.data(), 0, a.size()-1, 8) << endl;
  //cout<< "find: 9  => " << binary_search_closest(a.data(), 0, a.size()-1, 9) << endl;
  //cout<< "find: 11  => " << binary_search_closest(a.data(), 0, a.size()-1, 11) << endl;
  //cout<< "find: 6  => " << binary_search_closest(a.data(), 0, a.size()-1, 6) << endl;
  cout<< "find: 8  => " << binary_search_closest(a.data(), 0, a.size()-1, 8, 1) << endl;
  cout<< "find: 18  => " << binary_search_closest(a.data(), 0, a.size()-1, 18, 2) << endl;
}

int binary_search_range(int *a, int low, int high, int target_begin, int target_end, int *out_left, int *out_right)
{
  if (target_begin > a[high] || target_end < a[low])
    return -1;
  *out_left = binary_search_closest(a, low, high, target_begin, 1);
  *out_right = binary_search_closest(a, low, high, target_end, 2);
  return 0;
}

void test2()
{
  vector<int> a = {1,2,3,5,7,10, 14, 19, 25, 100};
  cout<<a<<endl;
  int left, right;
  if (binary_search_range(a.data(), 0, a.size()-1, 6, 24, &left, &right) == 0)
    cout << "[" <<left << ", " << right << "]" << endl;
  else
    cout << "Not found." << endl;
    
  if (binary_search_range(a.data(), 0, a.size()-1, 6, 25, &left, &right) == 0)
    cout << "[" <<left << ", " << right << "]" << endl;
  else
    cout << "Not found." << endl;
    
  if (binary_search_range(a.data(), 0, a.size()-1, 101, 200, &left, &right) == 0)
    cout << "[" <<left << ", " << right << "]" << endl;
  else
    cout << "Not found." << endl;
}

static imupool_t *g_pool = NULL;
static const int imu_num = 200;

void *write_routine(void *arg)
{
  (void)arg;
  imu_item_t data;
  
  while (1)
  {
    data.timestamp = getNs();
    imupool_insert(g_pool, &data);
    sleepMs(10); //100Hz
    //cout<<"hello"<<endl;
  }
  
  return NULL;
}

void *read_routine(void *arg)
{
  (void)arg;
  imu_item_t data[imu_num];
  int num;

  sleepMs(1000); //wait 1s
  
  while (1)
  {
    uint64_t now = getNs();
    int ret = imupool_range_request(g_pool, now - 90*1000*1000, now - 60*1000*1000, data, &num);
    //int ret = imupool_request(g_pool, now, 30*1000*1000, &data[0]);
    if (ret != IMUPOOL_NORMAL)
    {
      cout << toString(now) << "req fail. " << imupool_strerr(ret) << endl;
    }
    
    sleepMs(30);
  }

  return NULL;
}

int main(int argc, char *argv[])
{
  //test1(); return 0;
  //test2(); return 0;
  
  g_pool = imupool_create(imu_num);

  pthread_t tid_rd, tid_wr;
  pthread_create(&tid_wr, NULL, write_routine, NULL);
  pthread_create(&tid_rd, NULL, read_routine,  NULL);
  pthread_join(tid_wr, NULL);
  pthread_join(tid_rd, NULL);
  return 0;
}
