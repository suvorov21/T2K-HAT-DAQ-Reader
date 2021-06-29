/*******************************************************************************
                             PandaX-III / T2K-II
                             ___________________

File:        fdecoder.h

Description: Header file for decoder of binary data files recorded by the TDCM.


Author:      D. Calvet,        denis.calvetATcea.fr


History:
April 2019    : created

July 2019     : renamed existing members and added EventSizeMismatchCount,
 LastEventNumber and EventNumberGapCount

*******************************************************************************/

#ifndef FDECODER_H
#define FDECODER_H


#include "platform_spec.h"

typedef struct _Features {
	__int64  TotalFileByteRead;      // Total number of bytes read from file
	char     RunString[256];         // Run information string
	int      EventSizeMismatchCount; // Number of events with event size found significantly different from indicated size
	int      PreviousEventNumber;    // Event Number of the previous event
	int      EventNumberGapCount;    // Number of times event number of event (N+1) is different from event number of (event N) + 1
} Features;

#endif
