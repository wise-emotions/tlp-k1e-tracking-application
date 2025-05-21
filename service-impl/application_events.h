#pragma once

#include <glib-object.h>

typedef struct _ApplicationEvents ApplicationEvents;

typedef void (*ApplicationEvents_callback_type)(gpointer callback_context);

void ApplicationEvents_register_event(ApplicationEvents *self, const gchar *event_name, ApplicationEvents_callback_type callback, gpointer callback_context);
void ApplicationEvents_emit_event(ApplicationEvents *self, const gchar *event_name);

void ApplicationEvents_register_event_ex(ApplicationEvents *self, const gchar *event_name, GCallback callback, gpointer callback_context, guint nparams, ... /* param types */);
void ApplicationEvents_emit_event_ex(ApplicationEvents *self, const gchar *event_name, ... /* params */);

ApplicationEvents *ApplicationEvents_new();
void ApplicationEvents_destroy(ApplicationEvents *self);
