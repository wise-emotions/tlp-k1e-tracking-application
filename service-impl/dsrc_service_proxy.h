#pragma once

#include <glib.h>

#include "configuration_store.h"
#include "application_events.h"

typedef struct _DsrcServiceProxy DsrcServiceProxy;

gboolean DsrcServiceProxy_get_other_anomalies_present(const DsrcServiceProxy *self);

DsrcServiceProxy *DsrcServiceProxy_new(ConfigurationStore *configuration_store, ApplicationEvents *application_events);

void DsrcServiceProxy_destroy(DsrcServiceProxy *self);


