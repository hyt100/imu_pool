#pragma once
#include <iostream>
#include <string>
#include <vector>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<sys/time.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/select.h>

// for PRId64
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>


using std::string;
using std::vector;
using std::cout;
using std::endl;


static const int kMicroSecondsPerSecond = 1000 * 1000;

inline uint64_t getNs()
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  uint64_t ns = tp.tv_sec * 1000 * 1000 * 1000 + tp.tv_nsec;
  return ns;
}

inline void sleepUs(uint64_t usec)
{
  struct timespec ts = { 0, 0 };
  ts.tv_sec = static_cast<time_t>(usec / kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(usec % kMicroSecondsPerSecond * 1000);
  nanosleep(&ts, NULL);
}

inline void sleepMs(uint64_t ms)
{
  sleepUs(ms*1000);
}

inline string toString(uint64_t ns)
{
  char buf[64] = {0};
  uint64_t seconds = ns / (1000*1000*1000);
  uint64_t ms = ns % (1000*1000*1000) / (1000*1000);
  snprintf(buf, sizeof(buf)-1, "%" PRId64 ".%06" PRId64 "", seconds, ms);
  return buf;
}

std::ostream& operator<<(std::ostream &os, vector<int> &a)
{
  for (vector<int>::size_type i = 0; i<a.size(); ++i)
    cout << i << '\t';
  cout << endl;
  for (int x: a)
    cout << x << '\t';
  return os;
}



