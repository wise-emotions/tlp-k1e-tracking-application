#include <stdarg.h>
#include <glib/gprintf.h>

#include "logger.h"

#include "application_events.h"

struct _ApplicationEvents
{
	GObject *g_object;
};

void ApplicationEvents_register_event(ApplicationEvents *self, const gchar *event_name, ApplicationEvents_callback_type callback, gpointer callback_context)
{
	if (self != NULL)
	{
		g_signal_new(event_name, G_TYPE_OBJECT, G_SIGNAL_RUN_LAST || G_SIGNAL_NO_RECURSE, 0, NULL, NULL, NULL, G_TYPE_NONE, 0, G_TYPE_POINTER);
		g_signal_connect_swapped(self->g_object, event_name, G_CALLBACK(callback), callback_context);
	}
}

void ApplicationEvents_emit_event(ApplicationEvents *self, const gchar *event_name)
{
	if (self != NULL)
	{
		g_signal_emit_by_name(self->g_object, event_name);
	}
}


void ApplicationEvents_register_event_ex(ApplicationEvents *self, const gchar *event_name, GCallback callback, gpointer callback_context, guint nparams, ... /* param types */)
{
	if (self != NULL)
	{
		va_list args;
		va_start(args, nparams );
		g_signal_new_valist(event_name, G_TYPE_OBJECT, G_SIGNAL_RUN_LAST || G_SIGNAL_NO_RECURSE, 0, NULL, NULL, NULL, G_TYPE_NONE, nparams, args);
		g_signal_connect_swapped(self->g_object, event_name, G_CALLBACK(callback), callback_context);

		va_end(args);
	}
}

void ApplicationEvents_emit_event_ex(ApplicationEvents *self, const gchar *event_name, ...)
{
	if (self != NULL)
	{
		va_list args;
		guint signal_id;

		va_start(args, event_name );

		if ((signal_id = g_signal_lookup(event_name, G_TYPE_OBJECT)) != 0)
			g_signal_emit_valist(self->g_object, signal_id, 0, args);
		else
			logerr("unknown event %s", event_name);

		va_end(args);
	}
}


ApplicationEvents* ApplicationEvents_new()
{
	loginfo("");
	ApplicationEvents *self = g_try_new0(ApplicationEvents, 1);
	g_return_val_if_fail(self != NULL, NULL);
	self->g_object = g_object_new(G_TYPE_OBJECT, NULL);
	return self;
}

void ApplicationEvents_destroy(ApplicationEvents *self)
{
	loginfo("");
	if (self != NULL)
	{
		g_free(self);
	}
}
