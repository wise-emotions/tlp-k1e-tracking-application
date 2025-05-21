#pragma once

#include <glib.h>

#include "configuration_store.h"
#include "application_events.h"

typedef struct _IccServiceProxy IccServiceProxy;

gboolean IccServiceProxy_get_tampering_status(const IccServiceProxy *self, guint *value);

IccServiceProxy *IccServiceProxy_new(ConfigurationStore *configuration_store, ApplicationEvents *application_events);

void IccServiceProxy_destroy(IccServiceProxy *self);

