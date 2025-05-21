/*
 * trip_id_manager.h
 *
 * Created on: Mar 26, 2021
 * Author: Gianni Manzo (gianni.manzo@nttdata.com)
 */

#ifndef NEXT_APPLICATION_TRIP_ID_MANAGER_H_
#define NEXT_APPLICATION_TRIP_ID_MANAGER_H_


#include <glib.h>


typedef struct _TripIdManager TripIdManager;


TripIdManager *trip_id_manager_create();
void trip_id_manager_destroy(TripIdManager *self);

void trip_id_manager_generate_trip_id(TripIdManager *self, GString *obu_id);
GString *trip_id_manager_get_trip_id(TripIdManager *self);


#endif /* NEXT_APPLICATION_TRIP_ID_MANAGER_H_ */
