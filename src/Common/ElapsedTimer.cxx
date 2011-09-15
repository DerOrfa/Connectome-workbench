
/*LICENSE_START*/
/* 
 *  Copyright 1995-2011 Washington University School of Medicine 
 * 
 *  http://brainmap.wustl.edu 
 * 
 *  This file is part of CARET. 
 * 
 *  CARET is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2 of the License, or 
 *  (at your option) any later version. 
 * 
 *  CARET is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details. 
 * 
 *  You should have received a copy of the GNU General Public License 
 *  along with CARET; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 */ 


#define __ELAPSED_TIMER_DECLARE__
#include "ElapsedTimer.h"
#undef __ELAPSED_TIMER_DECLARE__

#include "CaretAssert.h"

using namespace caret;


/**
 * Constructor.
 */
ElapsedTimer::ElapsedTimer()
: CaretObject()
{
   m_started = false;
}

/**
 * Destructor.
 */
ElapsedTimer::~ElapsedTimer()
{
    
}

/**
 * Get a description of this object's content.
 * @return String describing this object's content.
 */
AString 
ElapsedTimer::toString() const
{
    return "ElapsedTimer";
}

/**
 * Start the timer.
 */
void 
ElapsedTimer::start()
{
#ifdef CARET_OS_WINDOWS
   m_startTime.m_tickCount = GetTickCount();//TODO: find a good way to use getTickCount64() so it doesn't have a reset at 49 days of uptime
#else
   gettimeofday(&(m_startTime.m_timeVal), NULL);
#endif
}

/**
 * Get the elapsed time in seconds.
 *
 * @return Elapsed time in seconds.
 */
double 
ElapsedTimer::getElapsedTimeSeconds() const
{
   return getElapsedTimeMilliseconds() / 1000.0;
}

/**
 * Get the elapsed time in milliseconds.
 *
 * @return Elapsed time in milliseconds.
 */
double 
ElapsedTimer::getElapsedTimeMilliseconds() const
{
   CaretAssertMessage(m_started, "Timer has not been started");
   MyTimeStore endTime;
#ifdef CARET_OS_WINDOWS
   endTime.m_tickCount = GetTickCount();//TODO: find a good way to use getTickCount64() when possible so it doesn't have a reset at 49 days of uptime
   if (endTime.m_tickCount < m_startTime.m_tickCount)//check for the 49 day wrap
   {
      endTime.m_tickCount += ((uint64_t)1)<<32;//m_tickCount is 64 bit, so this doesn't overflow
   }
   const double diffTimeMilli = (double)(endTime.m_tickCount - m_startTime.m_tickCount);//contrary to its name, it returns milliseconds, not ticks
#else
   gettimeofday(&(endTime.m_timeVal), NULL);
   double diffSeconds      = endTime.m_timeVal.tv_sec - m_startTime.m_timeVal.tv_sec;
   double diffMicroseconds = endTime.m_timeVal.tv_usec - m_startTime.m_timeVal.tv_usec;
   /*if (diffMicroseconds < 0) {//this is only needed if we are displaying both parts separately
      diffMicroseconds += 1000000;
      diffSeconds -= 1;//don't forget to subtract the second you just added to microseconds
   }//*/
   const double diffTimeMilli = diffSeconds + (diffMicroseconds / 1000.0);
#endif
   return diffTimeMilli;
}

