#ifndef EVALUATION_COMMON_H_INCLUDED
#define EVALUATION_COMMON_H_INCLUDED

/* metric related */
#define SS_COUNT                8              /* the signal strength count to be gathered, for efficiency, this value is picked from {4, 8, 16, 32, 64, 128}
												  however, in the beginning phase, if less metrics are gathered, evaluation WILL yield results as well
												  so this is the "ideal fixed signal strength count we wish to be available"
											   */
#define SS_SHIFT_BITS           3              // this value corresponds to METRIC_COUNT, "a / 32" is equivalent to "a >> 5", this mechanism can NOT apply to type double
#define RTT_COUNT               32             // the "ideal fixed RTT count we wish to be available"
#define RTT_SHIFT_BITS          5              // this value corresponds to RTT_COUNT, "a / 32" is equivalent to "a >> 5", this mechanism can NOT apply to type double

/* evaluation results */
typedef enum
{
	NETWORK_QUALITY_UNKNOWN = 10,
	NETWORK_QUALITY_GOOD = 11,
	NETWORK_QUALITY_NORMAL = 12,
	NETWORK_QUALITY_BAD = 13,
} NETWORK_QUALITY;

typedef enum
{
	NETWORK_BOTTLENECK_UNKNOWN = 20,
	NETWORK_BOTTLENECK_SIGNAL = 21,
	NETWORK_BOTTLENECK_TRAFFIC = 22,
	NETWORK_BOTTLENECK_STABILITY = 23,
	NETWORK_BOTTLENECK_NONE = 24,
} NETWORK_BOTTLENECK;

/* misc values */
#ifndef NULL
#define NULL            (void*)0
#endif // NULL
#define MAX_VALUE       1000000
#define SMALL_VALUE     0.0001
#define INVALID_VALUE   -1

#endif // EVALUATION_COMMON_H_INCLUDED