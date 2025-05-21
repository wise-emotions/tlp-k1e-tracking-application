/*
 * odometer.h
 *
 *  Created on: May 13, 2021
 *      Author: Laura Gamba (laura.gamba@nttdata.com)
 */

#ifndef ODOMETER_H_
#define ODOMETER_H_

#include <glib.h>

typedef struct _Odometer Odometer;
typedef struct _PositionData PositionData;

Odometer *odometer_new(void);
void odometer_destroy(Odometer *self);

void odometer_reset(Odometer *self);

void odometer_position_received(Odometer *self, const PositionData *position);

gdouble odometer_get_trip_distance(Odometer *self);


#endif /* ODOMETER_H_ */
