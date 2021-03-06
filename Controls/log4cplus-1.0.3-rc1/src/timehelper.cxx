// Module:  Log4CPLUS
// File:    timehelper.cxx
// Created: 4/2003
// Author:  Tad E. Smith
//
//
// Copyright (C) Tad E. Smith  All rights reserved.
//
// This software is published under the terms of the Apache Software
// License version 1.1, a copy of which has been included with this
// distribution in the LICENSE.APL file.
//

#include <log4cplus/helpers/timehelper.h>
#include <log4cplus/streams.h>
#include <log4cplus/helpers/stringhelper.h>

#include <iomanip>

#if defined(LOG4CPLUS_HAVE_FTIME)
#include <sys/timeb.h>
#endif

#if defined(LOG4CPLUS_HAVE_GETTIMEOFDAY)
#include <sys/time.h>
#endif

#if defined(LOG4CPLUS_HAVE_GMTIME_R) && !defined(LOG4CPLUS_SINGLE_THREADED)
#define LOG4CPLUS_NEED_GMTIME_R
#endif

#if defined(LOG4CPLUS_HAVE_LOCALTIME_R) && !defined(LOG4CPLUS_SINGLE_THREADED)
#define LOG4CPLUS_NEED_LOCALTIME_R
#endif


#define BUFFER_SIZE 40
#define ONE_SEC_IN_USEC 1000000


using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;



//////////////////////////////////////////////////////////////////////////////
// Time ctors
//////////////////////////////////////////////////////////////////////////////

Time::Time()
: tv_sec(0),
  tv_usec(0)
{
}




Time::Time(long tv_sec, long tv_usec)
: tv_sec(tv_sec),
  tv_usec(tv_usec)
{
}




Time::Time(time_t time)
: tv_sec(time),
  tv_usec(0)
{
}



Time
Time::gettimeofday()
{
#if defined(LOG4CPLUS_HAVE_GETTIMEOFDAY)
    timeval tp;
    ::gettimeofday(&tp, 0);

    return Time(tp.tv_sec, tp.tv_usec);
#elif defined(LOG4CPLUS_HAVE_FTIME)
    struct timeb tp;
    ::ftime(&tp);

    return Time(tp.time, tp.millitm * 1000);
#else
#warning "Time::gettimeofday()- low resolution timer: gettimeofday and ftime unavailable"
    return Time(::time(0), 0);
#endif
}




//////////////////////////////////////////////////////////////////////////////
// Time methods
//////////////////////////////////////////////////////////////////////////////

int
Time::setTime(struct tm* t)
{
    time_t time = ::mktime(t);
    if(time != -1) {
        tv_sec = time;
    }

    return time;
}



time_t
Time::getTime() const
{
    return tv_sec;
}



void
Time::gmtime(struct tm* t) const
{
    time_t clock = tv_sec;
#ifdef LOG4CPLUS_NEED_GMTIME_R
    ::gmtime_r(&clock, t);
#else
    struct tm* tmp = ::gmtime(&clock);
    *t = *tmp;
#endif
}



void
Time::localtime(struct tm* t) const
{
    time_t clock = tv_sec;
#ifdef LOG4CPLUS_NEED_LOCALTIME_R
    ::localtime_r(&clock, t);
#else
    struct tm* tmp = ::localtime(&clock);
    *t = *tmp;
#endif
}



log4cplus::tstring
Time::getFormattedTime(const log4cplus::tstring& fmt, bool use_gmtime) const
{
    tchar buffer[BUFFER_SIZE];
    struct tm time;

    if(use_gmtime) {
        gmtime(&time);
    }
    else {
        localtime(&time);
    }

#ifdef UNICODE
    size_t len = ::wcsftime(buffer, BUFFER_SIZE, fmt.c_str(), &time);
#else
    size_t len = ::strftime(buffer, BUFFER_SIZE, fmt.c_str(), &time);
#endif

    buffer[len] = '\0';
    tstring ret(buffer);

    //size_t pos = ret.find( LOG4CPLUS_TEXT("%q") );
	size_t pos = ret.find( LOG4CPLUS_TEXT("qq") );
    if(pos != tstring::npos) {
        tstring tmp(ret.substr(0, pos));
        tstring seconds( convertIntegerToString((tv_usec / 1000)) );
        switch(seconds.length()) {
            case 1: tmp += LOG4CPLUS_TEXT("00"); break;
            case 2: tmp += LOG4CPLUS_TEXT("0"); break;
        }
        tmp += seconds;
        tmp += ret.substr(pos + 2);
        ret = tmp;
    }

    //pos = ret.find( LOG4CPLUS_TEXT("%Q") );
	pos = ret.find( LOG4CPLUS_TEXT("QQ") );
    if(pos != tstring::npos) {
        tstring tmp(ret.substr(0, pos));
        tstring seconds( convertIntegerToString((tv_usec / 1000)) );
        switch(seconds.length()) {
            case 1: tmp += LOG4CPLUS_TEXT("00"); break;
            case 2: tmp += LOG4CPLUS_TEXT("0"); break;
        }
        tmp += seconds;
#if defined(LOG4CPLUS_HAVE_GETTIMEOFDAY)
        tstring usecs( convertIntegerToString((tv_usec % 1000)) );
        switch(usecs.length()) {
            case 1: tmp += LOG4CPLUS_TEXT(".00"); break;
            case 2: tmp += LOG4CPLUS_TEXT(".0"); break;
            case 3: tmp += LOG4CPLUS_TEXT("."); break;
        }
        tmp += usecs;
#endif
        tmp += ret.substr(pos + 2);
        ret = tmp;
    }

    return ret;
}



Time&
Time::operator+=(const Time& rhs)
{
    tv_sec += rhs.tv_sec;
    tv_usec += rhs.tv_usec;

    if(tv_usec > ONE_SEC_IN_USEC) {
        ++tv_sec;
        tv_usec -= ONE_SEC_IN_USEC;
    }

    return *this;
}



Time&
Time::operator-=(const Time& rhs)
{
    tv_sec -= rhs.tv_sec;
    tv_usec -= rhs.tv_usec;

    if(tv_usec < 0) {
        --tv_sec;
        tv_usec += ONE_SEC_IN_USEC;
    }

    return *this;
}



Time&
Time::operator/=(long rhs)
{
    long rem_secs = tv_sec % rhs;
    tv_sec /= rhs;
    
    tv_usec /= rhs;
    tv_usec += ((rem_secs * ONE_SEC_IN_USEC) / rhs);

    return *this;
}



Time&
Time::operator*=(long rhs)
{
    long new_usec = tv_usec * rhs;
    long overflow_sec = new_usec / ONE_SEC_IN_USEC;
    tv_usec = new_usec % ONE_SEC_IN_USEC;

    tv_sec *= rhs;
    tv_sec += overflow_sec;

    return *this;
}





//////////////////////////////////////////////////////////////////////////////
// Time globals
//////////////////////////////////////////////////////////////////////////////


const Time
operator+(const Time& lhs, const Time& rhs)
{
    return Time(lhs) += rhs;
}



const Time
operator-(const Time& lhs, const Time& rhs)
{
    return Time(lhs) -= rhs;
}



const Time
operator/(const Time& lhs, long rhs)
{
    return Time(lhs) /= rhs;
}



const Time
operator*(const Time& lhs, long rhs)
{
    return Time(lhs) *= rhs;
}



bool
operator<(const Time& lhs, const Time& rhs)
{
    return (   (lhs.sec() < rhs.sec())
            || (   (lhs.sec() == rhs.sec()) 
                && (lhs.usec() < rhs.usec())) );
}



bool
operator<=(const Time& lhs, const Time& rhs)
{
    return ((lhs < rhs) || (lhs == rhs));
}



bool
operator>(const Time& lhs, const Time& rhs)
{
    return (   (lhs.sec() > rhs.sec())
            || (   (lhs.sec() == rhs.sec()) 
                && (lhs.usec() > rhs.usec())) );
}



bool
operator>=(const Time& lhs, const Time& rhs)
{
    return ((lhs > rhs) || (lhs == rhs));
}



bool
operator==(const Time& lhs, const Time& rhs)
{
    return (   lhs.sec() == rhs.sec()
            && lhs.usec() == rhs.usec());
}



bool
operator!=(const Time& lhs, const Time& rhs)
{
    return !(lhs == rhs);
}


