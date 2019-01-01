#if defined(WIN32) || (_WIN64 == 1) || (_WIN32 == 1)
#include <time.h>
#include <windows.h>

struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};
static const unsigned __int64 delta_epoch_microsec = 11644473600000000U;
#ifdef MSVC6
int gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	if (NULL != tz)
		return -1;
	FILETIME file_time;
	SYSTEMTIME system_time;
	ULARGE_INTEGER ularge;
	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time,&file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;
	tp->tv_sec=(long)((ularge.QuadPart/10 - delta_epoch_microsec) / 1000000);
	tp->tv_usec=(long)(system_time.wMilliseconds * 1000);
	return 0;
}
#else

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  if (NULL != tz)
	return -1;
  FILETIME ft;
  ULARGE_INTEGER ularge;

  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    ularge.HighPart = ft.dwHighDateTime;
    ularge.LowPart = ft.dwLowDateTime;

    ularge.QuadPart /= 10;  /*convert into microseconds because it comes in 100 nano resolution*/
    /*converting file time to unix epoch*/
    ularge.QuadPart -= delta_epoch_microsec; 
    tv->tv_sec = (long)(ularge.QuadPart / 1000000UL);
    tv->tv_usec = (long)(ularge.QuadPart % 1000000UL);
  }
  return 0;
}
#endif //MSVC6
#else
#include <sys/time.h>
#endif
#if defined(WIN32) || (_WIN64 == 1) || (_WIN32 == 1)
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif

int64_t getTime() // microseconds resolution
{
	struct timeval tp;
	gettimeofday ( &tp, 0 );
	return (int64_t)tp.tv_sec*1000000+tp.tv_usec;
}