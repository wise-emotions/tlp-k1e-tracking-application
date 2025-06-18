#pragma once

#include <glib.h>

#include "positioning_service_types.h"
#include "configuration_store.h"
#include "application_events.h"

typedef struct _PositioningServiceProxy PositioningServiceProxy;

PositionData *PositioningServiceProxy_get_current_position(PositioningServiceProxy *self);
PositionData PositioningServiceProxy_get_last_position(PositioningServiceProxy *self);
PositionData PositioningServiceProxy_reset_last_pos_odom(PositioningServiceProxy *self);
PositioningServiceProxy *PositioningServiceProxy_new(ConfigurationStore *configuration_store, ApplicationEvents *application_events);
void PositioningServiceProxy_destroy(PositioningServiceProxy *self);

