/*
 * odometer.c
 *
 *  Created on: May 13, 2021
 *      Author: Laura Gamba (laura.gamba@nttdata.com)
 */


#include "odometer.h"

#include <string.h>

#include "positioning_service_types.h"
#include "logger.h"



#define MILLISECOND_TO_HOUR 0.000000277777778


typedef struct _Odometer {
	gdouble       total_distance; //KM
	PositionData *last_position;
} Odometer;


Odometer *odometer_new(void) {

	Odometer* self = (Odometer*) g_malloc0(sizeof(Odometer));
	self->last_position = NULL;
	self->total_distance = 0;

	return self;
}

void odometer_destroy(Odometer *self) {

	if(self != NULL) {

		if(self->last_position != NULL)
			g_free(self->last_position);

		g_free(self);
	}
}

void odometer_reset(Odometer *self) {

	if(self != NULL) {
		//init distance and last position data
		self->total_distance = 0;
		if(self->last_position != NULL) {
			g_free(self->last_position);
			self->last_position = NULL;
		}
	}
}

void odometer_position_received(Odometer *self, const PositionData *position) {

	if(self != NULL && position != NULL) {

		if(self->last_position != NULL) {

			//calculate distance from received position and last position
			gdouble partial_dist = 0;

			//NOTE: unit of measurement for speed is KM/h - unit of measurement for time is millisecond
			gdouble average_speed = (position->speed + self->last_position->speed) * 0.5;
			gdouble delta_time = (position->date_time - self->last_position->date_time) * MILLISECOND_TO_HOUR;
			if(average_speed > 0 && delta_time > 0) partial_dist = average_speed * delta_time;

			/*logdbg("last speed = %f km/h - curr speed = %f km/h", self->last_position->speed, position->speed);
			logdbg("average_speed = %f km/h", average_speed);
			logdbg("last time = %f millisec - curr time = %f millisec", self->last_position->date_time, position->date_time);
			logdbg("delta_time = %f hour", delta_time);
			logdbg("total_distance = %f km", self->total_distance);
			logdbg("partial_dist = %f km", partial_dist);*/

			//add distance to trip total distance
			self->total_distance += partial_dist;
			logdbg("NEW total_distance = %f km", self->total_distance);

			//clear position data
			g_free(self->last_position);
		}

		//save received position data
		self->last_position = (PositionData*) g_malloc0(sizeof(PositionData));
		memcpy(self->last_position, position, sizeof(PositionData));
	}
	else {
		logerr("Invalid input data!");
	}
}

gdouble odometer_get_trip_distance(Odometer *self) {

	gdouble ret_val = 0;
	if(self != NULL)
		ret_val = self->total_distance;
	else {
		logerr("Invalid odometer pointer!");
	}

	return ret_val;
}
