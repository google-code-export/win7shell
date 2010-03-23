#include "HPerformanceCounter.h"

#ifndef _WIN32
    bool QueryPerformanceFrequency( u_int64_t* puCounterFrecv );
    bool QueryPerformanceCounter(u_int64_t* puCounter);
#endif


bool const CHPerformanceCounter::bSupportPerformanceCounter = QueryPerformanceFrequency(&CHPerformanceCounter::CounterFrequency) ? true : false;
LARGE_INTEGER CHPerformanceCounter::CounterFrequency;




#ifndef _WIN32
bool QueryPerformanceFrequency(u_int64_t* puCounterFrecv  )
{
struct timespec tp;
	if (clock_getres (CLOCK_PROCESS_CPUTIME_ID, &tp) == 0 ) // succes
	{
	    *puCounterFrecv =  (__uint64_t)(tp.tv_sec * (__uint64_t)1000000000) + (__uint64_t)tp.tv_nsec;
	    return true;    
	}
	else
	    return false;



}


bool QueryPerformanceCounter(u_int64_t* puCounter)
{
    struct timespec tp;
	if (clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &tp) == 0 ) // succes
	{
	    *puCounter =  (__uint64_t)(tp.tv_sec * (__uint64_t)1000000000) + (__uint64_t)tp.tv_nsec;
	    return true;    
	}
	else
	    return false;

}

#endif
#include <stdio.h>
double CHPerformanceCounter::GetCurrentValue(){
	
	LARGE_INTEGER current;

	if(!bSupportPerformanceCounter)
		return 0;

	QueryPerformanceCounter(&current);
	
	
	#ifdef _WIN32
	    //return ((double)current.QuadPart * HPC_RESOLUTION / CounterFrequency.QuadPart);
		return (double)current.QuadPart;
	#else
	    return ((double)current * HPC_RESOLUTION / CounterFrequency);
	#endif
}




CHPerformanceCounter::CHPerformanceCounter() {

	#ifdef _WIN32
	    m_startValue.QuadPart = 0;
    	    m_stopValue.QuadPart = 0;
    	    m_value.QuadPart = 0;
	    m_cumulativeValue.QuadPart = 0;
	#else
	    m_startValue = 0;
    	    m_stopValue = 0;
    	    m_value = 0;
	    m_cumulativeValue = 0;
	#endif
	
	
	m_referenceCounter = 0;

	QueryPerformanceFrequency( &m_nFreq );

	//printf("FREQUENCY: %u\n", nFreq.QuadPart );

};

bool CHPerformanceCounter::start(){
	
	++m_referenceCounter;

	// only at the start we init the m_startValue
	if(m_referenceCounter == 1){
		QueryPerformanceCounter(&m_startValue);		
		return true;
	}
	else
		return false;

}

bool CHPerformanceCounter::stop(){
	
	--m_referenceCounter;
	
	if(m_referenceCounter == 0){
		QueryPerformanceCounter(&m_stopValue);		
		#ifdef _WIN32
		    m_cumulativeValue.QuadPart += m_value.QuadPart = m_stopValue.QuadPart - m_startValue.QuadPart;
		#else
		    m_cumulativeValue += m_value = m_stopValue - m_startValue;
		#endif
		
		return true;
	}
		return false;
}

void CHPerformanceCounter::reset(){

	#ifdef _WIN32
	    m_cumulativeValue.QuadPart = 0;
	#else
	    m_cumulativeValue = 0;
	#endif
	m_referenceCounter = 0;
}

double CHPerformanceCounter::getValue(){

	#ifdef _WIN32
    	    return ((double)m_value.QuadPart * HPC_RESOLUTION) / CounterFrequency.QuadPart;
	#else
	    return ((double)m_value * HPC_RESOLUTION) / CounterFrequency;
	#endif    
}

double CHPerformanceCounter::getCumulativeValue(){
	#ifdef _WIN32
	    return ((double)m_cumulativeValue.QuadPart * HPC_RESOLUTION) / CounterFrequency.QuadPart;
	#else
	    return ((double)m_cumulativeValue * HPC_RESOLUTION) / CounterFrequency;
	#endif    
}

double CHPerformanceCounter::getFrequency( void ) const
{
	return ((double)m_nFreq.QuadPart);
}