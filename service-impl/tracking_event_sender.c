/*  tracking_event_sender.c
 */
#include "logger.h"
#include "data_store.h"
#include "mqtt_client.h"
#include "configuration.h"
#include "tracking_event_sender.h"

typedef struct _TrackingEventSender TrackingEventSender;

struct _TrackingEventSender {
    Mqtt_Client                         *mqtt_client;
    char                                *topic_param;
    char                                *topic;
    guint                                completion_timeout;
    gulong                               publish_started_callback_id;
    gulong                               publish_ended_callback_id;
    guint                                wait_publish_timed_out_timer_id;
    int                                  pending_message;
    gboolean                             stop_waiting;
    gboolean                             expired;


    void (*ctor)(
        TrackingEventSender             *self,
        Mqtt_Client                     *mqtt_client,
        const char                      *topic_param,
        guint                            completion_timeout
    );

    void (*dtor)(
        TrackingEventSender             *self
    );

    gboolean (*send)(
        TrackingEventSender             *self,
        const char                      *message
    );

    void (*on_publish_started)(
        TrackingEventSender             *self,
        int                              message_id,
        const char                      *topic,
        int                              qos,
        int                              result
    );

    void (*on_publish_completed)(
        TrackingEventSender             *self,
        int                              message_id
    );

    gboolean (*on_publish_timeout_expired)(
        TrackingEventSender             *self
    );
};


static TrackingEventSender             *TrackingEventSender_new(void);
static void                             TrackingEventSender_ctor(TrackingEventSender *, Mqtt_Client *mqtt_client, const char *, guint);
static void                             TrackingEventSender_dtor(TrackingEventSender *);
static gboolean                         TrackingEventSender_send_impl(TrackingEventSender *, const char *);
static void                             TrackingEventSender_on_publish_started(TrackingEventSender *,int,const char *,int,int);
static void                             TrackingEventSender_on_publish_completed(TrackingEventSender *,int);
static gboolean                         TrackingEventSender_on_publish_timeout_expired(TrackingEventSender *);

static gchar                           *expand_topic_param(const char *topic_param);

extern gboolean
TrackingEventSender_send(
    Mqtt_Client                     *mqtt_client,
    const char                      *message,
    const char                      *topic_param,
    guint                            completion_timeout
)
{
    gboolean confirmed = FALSE;

    TrackingEventSender *self = TrackingEventSender_new();
    self->ctor(self, mqtt_client, topic_param, completion_timeout);

    confirmed = self->send(self, message);

    self->dtor(self);

    return confirmed;
}


static TrackingEventSender *
TrackingEventSender_new(void)
{
TrackingEventSender *obj;

    obj = g_try_new0(TrackingEventSender, 1);
    g_return_val_if_fail(obj != NULL, NULL);

    obj->ctor                                 = TrackingEventSender_ctor;
    obj->dtor                                 = TrackingEventSender_dtor;
    obj->send                                 = TrackingEventSender_send_impl;
    obj->on_publish_started                   = TrackingEventSender_on_publish_started;
    obj->on_publish_completed                 = TrackingEventSender_on_publish_completed;
    obj->on_publish_timeout_expired           = TrackingEventSender_on_publish_timeout_expired;

    return obj;
}



static void
TrackingEventSender_ctor(
    TrackingEventSender             *self,
    Mqtt_Client                     *mqtt_client,
    const char                      *topic_param,
    guint                            completion_timeout

)
{
    self->mqtt_client          = mqtt_client;
    self->completion_timeout   = completion_timeout;
    self->topic_param          = g_strdup(topic_param);
    self->topic                = expand_topic_param(topic_param);

    self->publish_started_callback_id = mqtt_client_add_publish_started_callback(
        self->mqtt_client,
        (mqtt_client_publish_started_callback_t)self->on_publish_started,
        self
    );

    self->publish_ended_callback_id = mqtt_client_add_publish_ended_callback(
        self->mqtt_client,
        (mqtt_client_publish_ended_callback_t)self->on_publish_completed,
        self
    );

    self->wait_publish_timed_out_timer_id = g_timeout_add_seconds(
        self->completion_timeout,
        (GSourceFunc)self->on_publish_timeout_expired,
        self
    );
}

static void
TrackingEventSender_dtor(
    TrackingEventSender             *self
)
{
    if (self->wait_publish_timed_out_timer_id)
        g_source_remove(self->wait_publish_timed_out_timer_id);

    mqtt_client_remove_publish_ended_callback(self->mqtt_client, self->publish_ended_callback_id);
    mqtt_client_remove_publish_started_callback(self->mqtt_client, self->publish_started_callback_id);

    g_free(self->topic);
    g_free(self->topic_param);

    g_free(self);


}

gboolean
TrackingEventSender_send_impl(
    TrackingEventSender             *self,
    const char                      *message
)
{

    mqtt_client_publish_message_on_topic_from_param(self->mqtt_client, message, self->topic_param);
    while(!self->stop_waiting) {
        if (!g_main_context_iteration(NULL, FALSE)) {
            if(self->stop_waiting) break;
            g_usleep(250000UL);
        }
    }
    return !self->expired;
}

static void
TrackingEventSender_on_publish_started(
    TrackingEventSender             *self,
    int                              message_id,
    const char                      *topic,
    int                              qos,
    int                              result
)
{
    if (g_strcmp0(topic, self->topic) == 0) {
        self->pending_message = message_id;
    }
}

static void
TrackingEventSender_on_publish_completed(
    TrackingEventSender             *self,
    int                              message_id
)
{
    if (message_id == self->pending_message) {
        self->stop_waiting = TRUE;
        g_timeout_add_full(G_PRIORITY_HIGH, 0,NULL, NULL, NULL);

        if (self->wait_publish_timed_out_timer_id) {
            g_source_remove(self->wait_publish_timed_out_timer_id);
            self->wait_publish_timed_out_timer_id = 0;
        }
    }
}

static gboolean
TrackingEventSender_on_publish_timeout_expired(
    TrackingEventSender             *self
)
{
    if(!self->stop_waiting) {
        self->stop_waiting = TRUE;
        self->expired = TRUE;
    }

    return FALSE;
}


static gchar *expand_topic_param(const char *topic_param)
{
gchar *obu_id, *topic_template, *topic;
GRegex *re = g_regex_new("\\{obu_id\\}", 0, 0, NULL);

    data_store_get_text_data((guchar*)OBU_ID_DATA_STORE_KEY, (guchar**)&obu_id);
    get_parameter(topic_param, (const void**)&topic_template);
    topic = g_regex_replace_literal(re, topic_template, -1, 0, obu_id, 0, NULL);

    g_regex_unref(re);
    g_free(obu_id);

    return topic;
}
