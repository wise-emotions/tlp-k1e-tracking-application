/*
 * tolling-gnss-sm.h
 *
 *  Created on: Mar 16, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#ifndef TOLLING_GNSS_SM_H_
#define TOLLING_GNSS_SM_H_

#include <glib.h>

#include "application_events.h"
#include "state_iface.h"

typedef struct _TollingGnss TollingGnss;
typedef struct _ServiceActivationSm ServiceActivationSm;
typedef struct _GnssFixBuffer GnssFixBuffer;
typedef struct _Tolling_Gnss_Sm_Data Tolling_Gnss_Sm_Data;
typedef struct _TollingGnssSm TollingGnssSm;
typedef struct _PositionData PositionData;

TollingGnssSm *tolling_gnss_sm_new(Tolling_Gnss_Sm_Data* data);
void tolling_gnss_sm_destroy(TollingGnssSm *self);

void tolling_gnss_sm_stop(TollingGnssSm *self);
void tolling_gnss_sm_on_hold(TollingGnssSm *self);
void tolling_gnss_sm_run(TollingGnssSm *self);

//void tolling_gnss_sm_send_tolling_data(TollingGnssSm *self); // TODO: serve davvero?

#endif
