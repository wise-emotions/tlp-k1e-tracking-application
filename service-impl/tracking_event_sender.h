/*  tracking_event_sender.h
 */
#ifndef __TRACKING_EVENT_SENDER_H__
#define __TRACKING_EVENT_SENDER_H__

#include <glib.h>

typedef struct _Mqtt_Client Mqtt_Client;

extern gboolean TrackingEventSender_send(Mqtt_Client *mqtt_client, const char *message, const char *topic_param, guint completion_timeout);

#endif /*__TRACKING_EVENT_SENDER_H__*/
