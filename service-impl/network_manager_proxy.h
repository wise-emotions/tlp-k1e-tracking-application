#pragma once

#include <glib.h>

#include "configuration_store.h"
#include "application_events.h"

typedef struct _NetworkManagerProxy NetworkManagerProxy;

NetworkManagerProxy *NetworkManagerProxy_new(ConfigurationStore *configuration_store, ApplicationEvents *application_events);
void NetworkManagerProxy_destroy(NetworkManagerProxy *self);

void NetworkManagerProxy_on_hold(NetworkManagerProxy *self);
void NetworkManagerProxy_resume(NetworkManagerProxy *self);
