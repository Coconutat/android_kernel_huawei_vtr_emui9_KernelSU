/******************************************************************************
   说明：本文件包含basten项目的network evaluation模块的辅助函数
******************************************************************************/

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "../emcom_utils.h"
#include <huawei_platform/emcom/evaluation_utils.h>

#undef HWLOG_TAG
#define HWLOG_TAG emcom_nweval_utils
HWLOG_REGIST();

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/* constant arrays */
static const int32_t TRAFFIC_SS_SEGMENT_ENDS[TRAFFIC_SS_SEGMENT_ENDPOINTS] = {-100, -60, -40, 0};
static const int32_t TRAFFIC_RTT_SEGMENT_ENDS[RTT_POINT_ENDPOINTS] = {0, 80, 180, 10000};
static const POINT_SET WIFI_REFERENCE_PROBABILITY_PERCENT_SETS[TRAFFIC_SS_SEGMENTS] = {{{100, 0, 0}, /**/ {50, 40, 10}, /**/{0, 20, 80}},            // good, normal, bad respectively
																		   {{100, 0, 0}, /**/ {80, 20, 0}, /**/ {30, 40, 30}},
																		   {{100, 0, 0}, /**/ {90, 10, 0}, /**/ {50, 30, 20}}};
static const POINT_SET LTE_REFERENCE_PROBABILITY_PERCENT_SETS[TRAFFIC_SS_SEGMENTS] = {{{100, 0, 0}, /**/ {50, 40, 10}, /**/{0, 20, 80}},            // good, normal, bad respectively
																		   {{100, 0, 0}, /**/ {80, 20, 0}, /**/ {30, 40, 30}},
																		   {{100, 0, 0}, /**/ {90, 10, 0}, /**/ {50, 30, 20}}};
static const POINT_SET OTHERS_REFERENCE_PROBABILITY_PERCENT_SETS[TRAFFIC_SS_SEGMENTS] = {{{100, 0, 0}, /**/ {50, 40, 10}, /**/{0, 20, 80}},            // good, normal, bad respectively
																		   	{{100, 0, 0}, /**/ {80, 20, 0}, /**/ {30, 40, 30}},
																		   	{{100, 0, 0}, /**/ {90, 10, 0}, /**/ {50, 30, 20}}};
static const int32_t STABILITY_RTT_SEGMENT_ENDS[STABILITY_RTT_SEGMENT_ENDPOINTS] = {0, 80, 150, 200, 10000};                          // all network types share this rtt segment setting
static const int32_t STABILITY_RTT_FINE_SQRDEV_SCALS[STABILITY_RTT_FINE_STDEV_ENDPOINTS] = {160, 300, 500, 2500, 10000};                     // formally used stdev = {1, 10, 20, 50, 100}

/*****************************************************************************
  3 函数声明
*****************************************************************************/
/* function decarations */
static POINT_SET convert_probability_to_count(POINT_SET* probability_set, int8_t data_length);
static int32_t cal_euclidiean_distance_square(POINT point1, POINT point2);
static int8_t find_nearest_reference_point(int32_t* distance_squares, int8_t number);

/*****************************************************************************
  4 函数实现
*****************************************************************************/
/* function implementations */

int8_t cal_traffic_index(int8_t network_type, int32_t signal_strength, int32_t* rtt_array, int8_t rtt_array_length)
{
	int32_t signal_strength_interval = INVALID_VALUE;
	int8_t result_index = INVALID_VALUE;
	int32_t distance_squares[INDEX_QUALITY_LEVELS] = {INVALID_VALUE};
	PPOINT_SET ptr_reference_probability_set;
	POINT_SET reference_point_set;
	POINT current_point;
	int index;
	if (!VALIDATE_NETWORK_TYPE( network_type) || signal_strength >= 0 || NULL == rtt_array || rtt_array_length <= 0)
	{
		EMCOM_LOGE(" : illegal parameter in cal_signal_index");
		return INVALID_VALUE;
	}

	/* pick reference points based on network type AND signal strength */
	for (index = 0; index < TRAFFIC_SS_SEGMENTS; index++)
	{
		if (signal_strength > TRAFFIC_SS_SEGMENT_ENDS[index] && signal_strength <= TRAFFIC_SS_SEGMENT_ENDS[index+1])
		{
			signal_strength_interval = index;
			break;
		}
	}

	if (signal_strength_interval < 0)
	{
		EMCOM_LOGE(" : negative signal_strength_interval");
		return INVALID_VALUE;
	}

	switch (network_type)
	{
	case NETWORK_TYPE_WIFI:
		ptr_reference_probability_set = (PPOINT_SET) &WIFI_REFERENCE_PROBABILITY_PERCENT_SETS[signal_strength_interval];
		break;
	case NETWORK_TYPE_4G:
		ptr_reference_probability_set = (PPOINT_SET) &LTE_REFERENCE_PROBABILITY_PERCENT_SETS[signal_strength_interval];
		break;
	case NETWORK_TYPE_3G:
		ptr_reference_probability_set = (PPOINT_SET) &OTHERS_REFERENCE_PROBABILITY_PERCENT_SETS[signal_strength_interval];
		break;
	case NETWORK_TYPE_2G:
		EMCOM_LOGD("2G network is not supported!");
		return INVALID_VALUE;
	default:
		EMCOM_LOGE(" : illegal network type in cal_traffic_index, %d", network_type);
		return INVALID_VALUE;
	}
	/* convert probability percent to count */
	reference_point_set = convert_probability_to_count(ptr_reference_probability_set, rtt_array_length);
	/* get the statistical characteristic of rtt_array, generate current point */
	current_point = get_space_point(rtt_array, rtt_array_length, TRAFFIC_RTT_SEGMENT_ENDS, RTT_POINT_ENDPOINTS);

	/* calculate Euclidean distance between current point and reference points */
	distance_squares[LEVEL_GOOD] = cal_euclidiean_distance_square(current_point, reference_point_set.good_point);
	distance_squares[LEVEL_NORMAL] = cal_euclidiean_distance_square(current_point, reference_point_set.normal_point);
	distance_squares[LEVEL_BAD] = cal_euclidiean_distance_square(current_point, reference_point_set.bad_point);           // GOOD, NORMAL, BAD are 0, 1, 2 respectively

	/* find the point with shortest distance */
	result_index = find_nearest_reference_point(distance_squares, INDEX_QUALITY_LEVELS);

	if (result_index < 0)
	{
		EMCOM_LOGE(" : illegal result in cal_traffic_index");
		return INVALID_VALUE;
	}

	EMCOM_LOGD(" : traffic index %d calculated!", result_index);
	return result_index;
}
EXPORT_SYMBOL(cal_traffic_index);


int8_t cal_rtt_stability_index(int8_t network_type, int32_t avg, int32_t sqrdev)
{
	int32_t value_rtt_sqrdev_bad = 0;
	int32_t value_rtt_sqrdev_normal = 0;
	int8_t stability_index = LEVEL_GOOD;

	if (!VALIDATE_NETWORK_TYPE(network_type))
	{
		EMCOM_LOGE(" : illegal parameter in cal_rtt_stability_index, network type %d", network_type);
		return INVALID_VALUE;
	}

	/* determine ss interval and rtt interval according to average value */
	if (avg <= STABILITY_RTT_SEGMENT_ENDS[0] || avg > STABILITY_RTT_SEGMENT_ENDS[4])
	{
		EMCOM_LOGI(" : rtt_avg out of evaluation boundary");
		return INVALID_VALUE;
	}
	else if (avg <= STABILITY_RTT_SEGMENT_ENDS[1])
	{
		value_rtt_sqrdev_bad = STABILITY_RTT_FINE_SQRDEV_SCALS[1];
		value_rtt_sqrdev_normal = STABILITY_RTT_FINE_SQRDEV_SCALS[0];
	}
	else if (avg <= STABILITY_RTT_SEGMENT_ENDS[2])
	{
		value_rtt_sqrdev_bad = STABILITY_RTT_FINE_SQRDEV_SCALS[2];
		value_rtt_sqrdev_normal = STABILITY_RTT_FINE_SQRDEV_SCALS[1];
	}
	else if (avg <= STABILITY_RTT_SEGMENT_ENDS[3])
	{
		value_rtt_sqrdev_bad = STABILITY_RTT_FINE_SQRDEV_SCALS[3];
		value_rtt_sqrdev_normal = STABILITY_RTT_FINE_SQRDEV_SCALS[2];
	}
	else                                                     // (avg <= index_rtt_segment_ends[4])
	{
		value_rtt_sqrdev_bad = STABILITY_RTT_FINE_SQRDEV_SCALS[4];
		value_rtt_sqrdev_normal = STABILITY_RTT_FINE_SQRDEV_SCALS[3];
	}

	EMCOM_LOGD(" : value_rtt_sqrdev_bad : %d, value_rtt_sqrdev_normal : %d", value_rtt_sqrdev_bad, value_rtt_sqrdev_normal);

	/* compare the input stdev values with the above reference ones */
	if (sqrdev > value_rtt_sqrdev_bad)
	{
		stability_index = LEVEL_BAD;
	}
	else if (sqrdev > value_rtt_sqrdev_normal && sqrdev <= value_rtt_sqrdev_bad)
	{
		stability_index = LEVEL_NORMAL;
	}
	else if (sqrdev >= 0 && sqrdev <= value_rtt_sqrdev_normal)       // note "=" is included here since 0 stdev is GOOD
	{
		stability_index = LEVEL_GOOD;
	}
	else
	{
		EMCOM_LOGI(" : unknown error in cal_stability_index");
		return INVALID_VALUE;
	}

	EMCOM_LOGD(" : stability index %d generated for network type %d", stability_index, network_type);
	return stability_index;
}
EXPORT_SYMBOL(cal_rtt_stability_index);


/******************************************* support functions ***********************************************/

/**
	 * @Function: get_space_point
	 * @Description : generate a point (x, y, z) in the 3-dimensional space, given the data array and the segments
	 *                x, y, z are the statistical probabilities that data fall in the first, second and third segment, respectively
	 * @Input : array, length, segment_ends, endpoints
	 * @Output : POINT
*/
POINT get_space_point(int32_t* array, int8_t length, const int32_t* segment_ends, int8_t endpoints)
{
	POINT generated_point = {INVALID_VALUE, INVALID_VALUE, INVALID_VALUE};
	int8_t segments = endpoints - 1;
	int8_t count_in_dimensions[POINT_DIMENSIONS];
	int8_t* count_in_segments;
	int index1;
	int index2;
	if (NULL == array || NULL == segment_ends || length <= 0 || segments <= 0)
	{
		EMCOM_LOGE(" : illegal parameter in get_probability_space_point");
		return generated_point;
	}

	count_in_segments = (int8_t*) kmalloc(segments * sizeof(int8_t), GFP_KERNEL);
	if (NULL == count_in_segments)
	{
		EMCOM_LOGE(" : memery allocation failed");
		return generated_point;
	}

	/* initialize the varibles */
	for (index1 = 0; index1 < segments; index1++)
	{
		count_in_segments[index1] = 0;
	}
	for (index1 = 0; index1 < POINT_DIMENSIONS; index1++)
	{
		count_in_dimensions[index1] = 0;
	}

	/* calculate counts in different segments */
	for (index1 = 0; index1 < length; index1++)               // outer loop for array[i]
	{
		for (index2 = 0; index2 < segments; index2++)        // inner loop for segment[j]
		{
			/* note the use of index1 and index2, do not be confused */
			if (array[index1] > segment_ends[index2] && array[index1] <= segment_ends[index2+1])    // it is possible that some data do not fall in any segment
			{
				count_in_segments[index2]++;
				break;                           // break inner loop for segment
			}
		}
	}

	/* distinguish if this call is in "standard form" or not */
	if (POINT_DIMENSIONS == segments)       // standard form (i.e., segments == dimensions) : to generate a RTT point
	{
		generated_point.x = count_in_segments[INDEX_X];
		generated_point.y = count_in_segments[INDEX_Y];
		generated_point.z = count_in_segments[INDEX_Z];
	}
	else if (segments > POINT_DIMENSIONS)                           // non-standard form : to generate a 3-d probability point
	{
		count_in_dimensions[INDEX_X] = count_in_segments[INDEX_X];
		count_in_dimensions[INDEX_Y] = count_in_segments[INDEX_Y];
		count_in_dimensions[INDEX_Z] = 0;
		for (index1 = INDEX_Z; index1 < segments; index1++)                // the fourth and latter segments will be merged into the third, so that a 3-d point can be generated
		{
			count_in_dimensions[INDEX_Z] += count_in_segments[index1];
		}

		generated_point.x = count_in_dimensions[INDEX_X];
		generated_point.y = count_in_dimensions[INDEX_Y];
		generated_point.z = count_in_dimensions[INDEX_Z];
	}
	else
	{
		EMCOM_LOGE(" : unknown error in get_probability_space_point, probably incorrect point dimension given");
	}

	kfree(count_in_segments);
	EMCOM_LOGD(" : obtained the current point (%d, %d, %d) in %d dimensional space", generated_point.x, generated_point.y, generated_point.z, POINT_DIMENSIONS);
	return generated_point;
}
EXPORT_SYMBOL(get_space_point);

/**
	 * @Function: cal_euclidiean_distance
	 * @Description : calculate the euclidiean distance between given two points
	 * @Input : point1, point2
	 * @Output : int32_t distance
*/
int32_t cal_euclidiean_distance_square(POINT point1, POINT point2)               // the drawback of this design is, only 3-d points can be calculated
{
	int32_t sum_square = INVALID_VALUE;

	if (point1.x < 0 || point1.y < 0 || point1.z < 0 || point2.x < 0 || point2.y < 0 || point2.z < 0)       // INVALID_VALUE == -1
	{
		EMCOM_LOGE(" : illegal point in cal_euclidiean_distance, point1.x = %d, point1.y = %d, point1.z = %d, point2.x = %d, point2.y = %d, point2.z = %d", \
						point1.x, point1.y, point1.z, point2.x, point2.y, point2.z);
		return INVALID_VALUE;
	}

	sum_square = (point1.x - point2.x)*(point1.x - point2.x) + (point1.y - point2.y)*(point1.y - point2.y) + (point1.z - point2.z)*(point1.z - point2.z);
	EMCOM_LOGD(" : calculated distance square between current point (%d, %d, %d) and reference point (%d, %d, %d) : %d", \
					point1.x, point1.y, point1.z, point2.x, point2.y, point2.z, sum_square);

	return sum_square;
}

/**
	 * @Function: find_nearest_reference_point
	 * @Description : find the reference point most adjacent to the current point, given distances
	 * @Input : distances, number
	 * @Output : index_of_nearest_point
*/
int8_t find_nearest_reference_point(int32_t* distance_squares, int8_t number)
{
	int8_t index_of_nearest_reference_point = INVALID_VALUE;
	int32_t min = MAX_VALUE;
	int index;
	if (NULL == distance_squares || number <= 0)
	{
		EMCOM_LOGE(" : illegal point in find_nearest_reference_point");
		return INVALID_VALUE;
	}

	/* find the shortest temporary distance */
	for (index = 0; index < number; index++)
	{
		if (distance_squares[index] < 0)
		{
			EMCOM_LOGE(" : illegal (negative) distance in find_nearest_reference_point");
			return INVALID_VALUE;
		}
		if (distance_squares[index] < min)
		{
			min = distance_squares[index];
		}

	}
	/* a loop to find the index of nearest reference point */
	for (index = number - 1; index >= 0; index--)
	{
		if (distance_squares[index] == min)
		{
			index_of_nearest_reference_point = index;
			EMCOM_LOGD(" : found the minimum distance square %d, nearest reference point %d", min, index);
			break;                                       // in case where multiple nearest reference points exist, return the "worst condition"
		}
	}

	return index_of_nearest_reference_point;
}

/**
	 * @Function: cal_statistics
	 * @Description : calculate avg and stdev values
	 * @Input : array, length
	 * @Output : avg, stdev
*/
void cal_statistics(const int32_t* array, const int8_t length, int32_t* avg, int32_t* sqrdev)
{
	int8_t actual_length = length;          // since data validation is introduced, the number of legal data may differ from the input parameter
	int32_t sum = 0;                       // integer yields higher calculation efficiency, however it is not clear if accuracy could be satisfied
	int32_t average = 0;
	int32_t square_deviation = 0;
	int index;

	if (NULL == array || length <= 0)
	{
		EMCOM_LOGE(" : illegal parameter in cal_statistics, length : %d", length);
		return;
	}

	/* calculate average value */
	for (index = 0; index < length; index++)
	{
		if (array[index] <= SIGNAL_ABNORMAL_THRESHOLD || array[index] >= RTT_ABNORMAL_THRESHOLD)         // exclude illegal data, with this, the function is not "pure and sole statistics calculation"
		{
			EMCOM_LOGD(" : illegal data (index %d) %d in average value calculation, either illegal signal strength or illgal rtt", index, array[index]);
			actual_length--;
			continue;
		}
		sum += array[index];
	}
	if (SS_COUNT == actual_length)                 // for efficiency
	{
		average = sum >> SS_SHIFT_BITS;
	}
	else if (RTT_COUNT == actual_length)
	{
		average = sum >> RTT_SHIFT_BITS;
	}
	else                                        // not standard length
	{
		average = sum / actual_length;
	}

	/* calculate standard deviation */
	sum = 0;
	for (index = 0; index < length; index++)
	{
		if (array[index] <= SIGNAL_ABNORMAL_THRESHOLD || array[index] >= RTT_ABNORMAL_THRESHOLD)         // exclude illegal data, with this, the function is not "pure and sole statistics calculation"
		{
			EMCOM_LOGE(" : illegal data (index %d) %d in deviation calculation, either illegal signal strength or illgal rtt", index, array[index]);
			// do NOT change actual_length here again, otherwise it will be incorrect
			continue;
		}
		sum += (average - array[index]) * (average - array[index]);
	}
	if (SS_COUNT == actual_length)                 // for efficiency
	{
		square_deviation = sum >> SS_SHIFT_BITS;
	}
	else if (RTT_COUNT == actual_length)
	{
		square_deviation = sum >> RTT_SHIFT_BITS;
	}
	else                                       // not standard length
	{
		square_deviation = sum / actual_length;
	}

	*avg = average;
	*sqrdev = square_deviation;
}
EXPORT_SYMBOL(cal_statistics);

/**
	 * @Function: cal_statistics
	 * @Description : calculate avg and stdev values
	 * @Input : array, length
	 * @Output : avg, stdev
*/
POINT_SET convert_probability_to_count(POINT_SET* probability_set, int8_t data_length)
{
	POINT_SET count_set = {INVALID_POINT, INVALID_POINT, INVALID_POINT};
	if (NULL == probability_set)
	{
		EMCOM_LOGE(" : null probability_set pointer");
		return count_set;
	}
	if (!VALIDATE_POINT(probability_set->good_point) || !VALIDATE_POINT(probability_set->normal_point) || !VALIDATE_POINT(probability_set->bad_point))
	{
		EMCOM_LOGE(" : illegal reference probability set");
		return count_set;
	}

	POINT_CONVERT_PERCENT_TO_COUNT(count_set.good_point, probability_set->good_point, data_length);
	POINT_CONVERT_PERCENT_TO_COUNT(count_set.normal_point, probability_set->normal_point, data_length);
	POINT_CONVERT_PERCENT_TO_COUNT(count_set.bad_point, probability_set->bad_point, data_length);

	EMCOM_LOGD(" : through conversion, the reference points are : good (%d, %d, %d), normal (%d, %d, %d), bad (%d, %d, %d)", \
							count_set.good_point.x, count_set.good_point.y, count_set.good_point.z, \
							count_set.normal_point.x, count_set.normal_point.y, count_set.normal_point.z, \
							count_set.bad_point.x, count_set.bad_point.y, count_set.bad_point.z);
	return count_set;
}