#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <sys/time.h>
#include <sys/resource.h>
#include "Timer.hh"

namespace BASim {

Timer::TimerMap Timer::timerMap;

inline double timeofday()
{
  timeval tp;
  gettimeofday(&tp, NULL);
  double result = (double) tp.tv_sec + 1e-6 * (double) tp.tv_usec;
  return result;
}

inline double cpuusage()
{
  struct rusage rus;
  getrusage(RUSAGE_SELF, &rus);
  double user = (double) rus.ru_utime.tv_sec + 1e-6
    * (double) rus.ru_utime.tv_usec;
  double sys = (double) rus.ru_stime.tv_sec + 1e-6
    * (double) rus.ru_stime.tv_usec;

  return user + sys;
}

inline double now()
{
  //return timeofday();
  return cpuusage();
}

Timer& Timer::getTimer(std::string name)
{
  TimerMap::iterator iter = timerMap.find(name);
  if (iter != timerMap.end()) return *(iter->second);

  Timer* t = new Timer();
  timerMap[name] = t;
  return *t;
}

void Timer::deleteAllTimers()
{
  TimerMap::iterator iter;

  for (iter = timerMap.begin(); iter != timerMap.end(); ++iter) {
    delete iter->second;
  }

  timerMap.clear();
}

 
void Timer::lap() 
{
  if (nestingLevel) 
  {
    elapsed   = now() - startTime;
    startTime = now();
  }

  cumulativeElapsed += elapsed;
  cumulativeCount   += count;

  elapsed = 0;
  count   = 0;
}


void Timer::report()
{
  TimerMap::iterator iter;

  for (iter = timerMap.begin(); iter != timerMap.end(); ++iter) {
    std::string name = iter->first;
    Timer* t = iter->second;
    std::cout << std::setw(50) << name << 
      //" (" << iter->second << ") " <<
      " = " << *t << std::endl;
    t->lap();
  }
}

std::ostream& operator<<(std::ostream& os, Timer& timer)
{
  os << "\t" << std::setw(10) << timer.getElapsed() << " (" << std::setw(5) << timer.getCount() << ") "
     << "\tcumul: " << std::setw(10) << timer.getCumulativeElapsed() << " (" << std::setw(5) << timer.getCumulativeCount() << ") "
     << "\tlevel: " << timer.getNestingLevel() << "/" << timer.getDeepestNestingLevel() ;
  return os;
}


Timer::Timer()
  : startTime(0)
  , elapsed(0)
  , cumulativeElapsed(0)
  , count(0)
  , cumulativeCount(0)
  , nestingLevel(0)
  , deepestNestingLevel(0)
{}

void Timer::clear()
{
  startTime           = 0.;
  elapsed             = 0.;
  cumulativeElapsed   = 0.;
  nestingLevel        = 0;
  count               = 0;
  cumulativeCount     = 0;
  deepestNestingLevel = 0;
}

void Timer::beginBlock()
{
  assert(nestingLevel >= 0);
  //std::cout << "Beginning timer block " << this << " from nestingLevel = " << nestingLevel; 
  if (!nestingLevel)
  {
    //std::cout << " STARTING ";
    startTime = now();
    ++count;
  }
  ++nestingLevel;
  if (nestingLevel > deepestNestingLevel) deepestNestingLevel = nestingLevel;
  //std::cout << " entering level " << nestingLevel << std::endl;
}

void Timer::endBlock()
{
  //std::cout << "Ending timer block " << this << " from nestingLevel = " << nestingLevel; 
  --nestingLevel;
  assert(nestingLevel >= 0);
  if (!nestingLevel) {
    //std::cout << " STOPPING ";
    elapsed = now() - startTime;
    times.push_back(elapsed);
    startTime = 0.;
  }
  //std::cout << " exiting to level " << nestingLevel << std::endl;
}

double Timer::getElapsed()
{
  double e = elapsed;

  if (nestingLevel) {
    e = now() - startTime;
  }

  return e;
}

double Timer::getCumulativeElapsed()
{
  double e = cumulativeElapsed;

  if (nestingLevel) {
    e += getElapsed();
  }

  return e;
}


} // namespace BASim
