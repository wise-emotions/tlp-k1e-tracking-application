/*
 * service_activation_sm.h
 *
 *  Created on: Mar 22, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#ifndef SERVICE_ACTIVATION_SM_H_
#define SERVICE_ACTIVATION_SM_H_

//#include <glib.h>

#include "state_iface.h"

typedef struct _ConnectionSm ConnectionSm;
typedef struct _TollingGnss TollingGnss;
typedef struct _GnssFixBuffer GnssFixBuffer;

/*
typedef struct _ServiceActivationSmData {
	ConnectionSm  *connection_sm;
	TollingGnss   *tolling_gnss;
	GnssFixBuffer *gnss_fix_buffer;
} ServiceActivationSmData;
*/

typedef struct _Tolling_Gnss_Sm_Data Tolling_Gnss_Sm_Data;

typedef struct _ServiceActivationSm ServiceActivationSm;
typedef struct _PositionData PositionData;

ServiceActivationSm *service_activation_sm_new(Tolling_Gnss_Sm_Data* data);
void service_activation_sm_destroy(ServiceActivationSm *self);

void service_activation_sm_stop(ServiceActivationSm *self);
void service_activation_sm_start(ServiceActivationSm *self);

void service_activation_sm_not_active(ServiceActivationSm *self);
void service_activation_sm_no_go(ServiceActivationSm *self);
void service_activation_sm_active(ServiceActivationSm *self);

void service_activation_sm_not_connected(ServiceActivationSm *self);
void service_activation_sm_connected(ServiceActivationSm *self);

void service_activation_sm_anomaly(ServiceActivationSm *self);
void service_activation_sm_no_anomaly(ServiceActivationSm *self);

void service_activation_sm_position_updated(ServiceActivationSm *self, const PositionData *position);
void service_activation_sm_send_tolling_data(ServiceActivationSm *self);

#endif /* SERVICE_ACTIVATION_SM_H_ */
