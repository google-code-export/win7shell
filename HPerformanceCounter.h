#ifndef _HPERFORMANCECOUNTER_H_
#define _HPERFORMANCECOUNTER_H_

#if defined(_WIN32)
	#include <windows.h>
#else
//	#error Not implemented
	#include <sys/types.h>
	#include <time.h>
	#define LARGE_INTEGER u_int64_t
#endif

/* counter resolution */
#define HPC_RESOLUTION 1000000 // 1 microsecond

/** High-resolution performance counter. It have a microsecond resolution */

class CHPerformanceCounter {
public:

	/** Get current counter value in microseconds */
	static double GetCurrentValue();

	/** Constructor */
	CHPerformanceCounter();

	/**
	 * Start the counter
	 * @return true if the reference count was 0 or false otherwise
	 **/
	bool start();
	/**
	 * Stop the counter
	 * @return true if reference count became 0 or false otherwise
	 **/
	bool stop();

	/**
	 * Reset cumulative value.
	 **/
	void reset();

	/** 
	 * Return the counter value
	 * @return the number of microseconds ellapsed between start() and stop()
	 **/
	double getValue();

	/**
	 * Return cumulative value. This is cumulative values between all the start() and stop() pairs. 
	 * This value can be reset using reset() function.
	 * @return cumulative value
	 **/
	double getCumulativeValue();

	double getFrequency( void ) const;

protected:

	/** Flag indicating if the performance counter is supported */
	static const bool bSupportPerformanceCounter;
	/** Counter frequency (increments/second) */
	static LARGE_INTEGER CounterFrequency;
	//static LARGE_INTEGER CounterFrequency;

	/** Start value */
	LARGE_INTEGER m_startValue;
	/** Stop value */
	LARGE_INTEGER m_stopValue;

	/** Last value */
	LARGE_INTEGER m_value;
	/** Cumulative value */
	LARGE_INTEGER m_cumulativeValue;
	LARGE_INTEGER m_nFreq;

	/** Use to count multiple start() stop() calls. Only when this value is 0 the start and stop values are updated */
	unsigned int m_referenceCounter;
	
	//LARGE_INTEGER queryPerfCounter();
	//LARGE_INTEGER queryPerfFrequency();

};

#endif // _HPERFORMANCECOUNTER_H_

