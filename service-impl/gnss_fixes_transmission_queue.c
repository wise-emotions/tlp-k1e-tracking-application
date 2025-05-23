/*  gnss_fixes_trasmission_queue.c
 */
#include "glib.h"
#include "logger.h"
#include "mqtt_client.h"

#include "gnss_fix_data.h"
#include "message_composer.h"
#include "gnss_fix_filter.h"
#include "tolling_gnss_sm_data.h"
#include "tolling_manager_proxy.h"
#include "application_events.h"
#include "application_events_common_names.h"
#include "resource_access_mediator.h"
#include "openssl/md5.h"
#include <string.h>


#include "gnss_fixes_transmission_queue.h"

#define FIX_SAVE_VERSION					  2

#define ESTIMATED_FIX_RATE                    1.05

#define FALLBACK_MAX_PACKET                   4
#define FALLBACK_TRANSMISSION_TIMEOUT        10
#define FALLBACK_NETWORK_ANOMALY_TIMEOUT     30

#define RM_FIXES_PATH	RM_USR_DATA

static GRecMutex  mutex;

static gboolean   connected = 0;
static gboolean   pending_message = 0;
static int        pending_message_id = 0;



struct _GnssFixesTransmissionQueuePriv  {
    Tolling_Gnss_Sm_Data                *context;

    gboolean                             cut_off;

    GQueue                              *txq;
    guint                                max_queue_size;
    GString								*domain_name;

    GThread                             *transmitter;
    gboolean                             terminate;

    guint                                max_packet_size;
    guint64                              packet_flush_timeout;

    guint64                              last_sent_time;

    GArray *(*pop)(
        GnssFixesTransmissionQueue       *self
    );


    void (*on_configuration_changed)(
        GnssFixesTransmissionQueue       *self
    );

    gpointer (*worker_thread)(
        GnssFixesTransmissionQueue       *self
    );

    void (*set_cut_off)(
        GnssFixesTransmissionQueue      *self,
		gboolean                         active
    );
};

static void                             GnssFixesTransmissionQueue_ctor(GnssFixesTransmissionQueue *, Tolling_Gnss_Sm_Data *,const gchar *domain_name);
static void                             GnssFixesTransmissionQueue_dtor(GnssFixesTransmissionQueue *);
static void                             GnssFixesTransmissionQueue_push(GnssFixesTransmissionQueue *, const PositionData *);
static GArray                          *GnssFixesTransmissionQueue_pop(GnssFixesTransmissionQueue *);
static GArray*                          GnssFixesTransmissionQueue_head(GnssFixesTransmissionQueue       *self);
static void                             GnssFixesTransmissionQueue_delayed_activation(GnssFixesTransmissionQueue *);
static void                             GnssFixesTransmissionQueue_on_configuration_changed(GnssFixesTransmissionQueue *);
static gpointer                         GnssFixesTransmissionQueue_worker_thread(GnssFixesTransmissionQueue *);

static void                             GnssFixesTransmissionQueue_set_cut_off(GnssFixesTransmissionQueue *, gboolean active);

static void                             on_network_connected(gpointer unused);
static void                             on_network_disconnected(gpointer unused);

static void                             on_publish_started(void *service, int message_id, const char *topic, int qos, int result);
static void                             on_publish_completed(void *service, int message_id);
//static GByteArray*						GnssFixesTransmissionQueue_fix_serializer(GnssFixesTransmissionQueue *);
static gboolean							GnssFixesTransmissionQueue_fix_writer(GnssFixesTransmissionQueue *);
static gboolean 						GnssFixesTransimissionQueue_fix_reader(GnssFixesTransmissionQueue *self);

GnssFixesTransmissionQueue *
GnssFixesTransmissionQueue_new()
{
GnssFixesTransmissionQueue *obj;

    obj = g_try_new0(GnssFixesTransmissionQueue, 1);
    g_return_val_if_fail(obj != NULL, NULL);

    obj->priv = g_try_new0(GnssFixesTransmissionQueuePriv,1);
    if (obj->priv == NULL) g_free(obj);
    g_return_val_if_fail(obj->priv != NULL, NULL);

    obj->ctor                              = GnssFixesTransmissionQueue_ctor;
    obj->dtor                              = GnssFixesTransmissionQueue_dtor;
    obj->push                              = GnssFixesTransmissionQueue_push;
    obj->delayed_activation                = GnssFixesTransmissionQueue_delayed_activation;
    obj->head							   = GnssFixesTransmissionQueue_head;


    obj->priv->pop                         = GnssFixesTransmissionQueue_pop;
    obj->priv->on_configuration_changed    = GnssFixesTransmissionQueue_on_configuration_changed;
    obj->priv->worker_thread               = GnssFixesTransmissionQueue_worker_thread;
    obj->priv->set_cut_off                 = GnssFixesTransmissionQueue_set_cut_off;

    return obj;
}


static void
GnssFixesTransmissionQueue_ctor(
    GnssFixesTransmissionQueue       *self,
    Tolling_Gnss_Sm_Data            *context,
	const gchar *domain_name
)
{
    g_return_if_fail(self != NULL);

    self->priv->context   = context;
    self->priv->terminate = FALSE;
    self->priv->txq       = g_queue_new();
    self->priv->domain_name = g_string_new(domain_name);
    g_rec_mutex_lock(&mutex);
    	GnssFixesTransimissionQueue_fix_reader(self);
    g_rec_mutex_unlock(&mutex);
    self->priv->cut_off = TRUE;



    ApplicationEvents_register_event_ex(
        self->priv->context->application_events,
        EVENT_GNSS_FIXES_CUT_OFF,
        G_CALLBACK(self->priv->set_cut_off),
        self,
        1,
        G_TYPE_BOOLEAN
    );


    self->priv->on_configuration_changed(self);

    ApplicationEvents_register_event(
        self->priv->context->application_events,
        EVENT_TOLLING_MANAGER_PROXY_CONFIGURED_FROM_REMOTE,
        (ApplicationEvents_callback_type)self->priv->on_configuration_changed,
        self
    );

    ApplicationEvents_register_event(
        self->priv->context->application_events,
        EVENT_NETWORK_MANAGER_PROXY_NETWORK_CONNECTED,
        on_network_connected,
        NULL
    );

    ApplicationEvents_register_event(
        self->priv->context->application_events,
        EVENT_NETWORK_MANAGER_PROXY_NETWORK_NOT_CONNECTED,
        on_network_disconnected,
        NULL
    );
}


static void 
GnssFixesTransmissionQueue_dtor(
    GnssFixesTransmissionQueue *self
)
{
    g_return_if_fail(self != NULL);

    loginfo("shutting down gnss fixes transmitter");
    g_rec_mutex_lock(&mutex);
    self->priv->terminate = TRUE;
    g_rec_mutex_unlock(&mutex);

    if (self->priv->transmitter)
        g_thread_join(self->priv->transmitter);
    loginfo("terminated");

    g_queue_free_full(self->priv->txq, (GDestroyNotify)gnss_fix_data_destroy);
    g_string_free(self->priv->domain_name, TRUE);
    g_free(self->priv);
    g_free(self);
}

static GArray*
GnssFixesTransmissionQueue_head(
    GnssFixesTransmissionQueue       *self
)
{
	GArray   *packet;
	gpointer  fix;

	g_rec_mutex_lock(&mutex);

	packet = g_array_sized_new(FALSE, FALSE, sizeof(const gpointer), 1);

	if( g_queue_get_length(self->priv->txq) > 0){
		fix = g_queue_pop_head(self->priv->txq);
		g_array_append_val(packet, fix);
	}
	g_rec_mutex_unlock(&mutex);



	return packet;
}

static void
GnssFixesTransmissionQueue_push(
    GnssFixesTransmissionQueue       *self,
    const PositionData              *fix
)
{
    g_return_if_fail(self != NULL);

    if (self->priv->cut_off)
        return;

    GnssFixData *fix_data = gnss_fix_data_new();
    gnss_fix_data_copy_from_position_data(fix_data, fix, self->priv->context);

    if (GnssFixFilter_filter(self->priv->context->gnss_fix_filter, fix_data)) {
        g_rec_mutex_lock(&mutex);
        GnssFixFilter_set_last_fix(self->priv->context->gnss_fix_filter, fix_data);

        if (g_queue_get_length(self->priv->txq) < self->priv->max_queue_size) {

            g_queue_push_head(self->priv->txq,fix_data);

            if ( (g_queue_get_length(self->priv->txq) % self->priv->max_packet_size) == 1 ) {
                self->priv->last_sent_time = g_get_real_time();
            }
            g_rec_mutex_unlock(&mutex);

            return;
        }

        g_rec_mutex_unlock(&mutex);
        logwarn("GNSS fixes transmission queue full, discarding fixes");
    }
    gnss_fix_data_destroy(fix_data);
}


static GArray *
GnssFixesTransmissionQueue_pop(
    GnssFixesTransmissionQueue       *self
)
{
    guint     count;
    GArray   *packet;
    gpointer  fix;

    g_rec_mutex_lock(&mutex);


    count = MIN(self->priv->max_packet_size, g_queue_get_length(self->priv->txq));
    packet = g_array_sized_new(FALSE, FALSE, sizeof(const gpointer), count);

    while(count--) {
        fix = g_queue_pop_tail(self->priv->txq);
        g_array_append_val(packet, fix);
    }

    g_rec_mutex_unlock(&mutex);

    return packet;
}



static void
GnssFixesTransmissionQueue_delayed_activation(GnssFixesTransmissionQueue *self)
{
    g_rec_mutex_lock(&mutex);
    mqtt_client_register_publish_started_callback(
        self->priv->context->mqtt_client,
        on_publish_started
    );

    mqtt_client_register_publish_ended_callback(
        self->priv->context->mqtt_client,
        on_publish_completed
    );


    self->priv->transmitter = g_thread_new(
        "gnss fixes transmitter",
        (GThreadFunc)self->priv->worker_thread,
        self
    );
    g_rec_mutex_unlock(&mutex);

}



static void
GnssFixesTransmissionQueue_on_configuration_changed(
    GnssFixesTransmissionQueue       *self
)
{
    g_return_if_fail(self != NULL);

    guint     transmission_timeout     = 0;
    guint16   max_packet               = 0;
    guint     network_anomaly_timeout  = 0;


    g_rec_mutex_lock(&mutex);

    if (!TollingManagerProxy_get_transmission_timeout(self->priv->context->tolling_manager_proxy, &transmission_timeout)) {
        logwarn("cannot retrieve transmission_timeout configuration value, using fall back value %d", FALLBACK_TRANSMISSION_TIMEOUT);
        transmission_timeout = FALLBACK_TRANSMISSION_TIMEOUT;
    }
    if (!TollingManagerProxy_get_max_packet(self->priv->context->tolling_manager_proxy, &max_packet)) {
        logwarn("cannot retrieve max_packet configuration value, using fall back value %d", FALLBACK_MAX_PACKET);
        max_packet = FALLBACK_MAX_PACKET;
    }

    if (!TollingManagerProxy_get_network_anomaly_timeout(self->priv->context->tolling_manager_proxy, &network_anomaly_timeout)) {
        logwarn("cannot retrieve network_anomaly_timeout configuration value, using fall back value %d", FALLBACK_NETWORK_ANOMALY_TIMEOUT);
        network_anomaly_timeout = FALLBACK_NETWORK_ANOMALY_TIMEOUT;
    }

    self->priv->packet_flush_timeout = transmission_timeout * 1000000UL;
    self->priv->max_packet_size      = max_packet;
    self->priv->max_queue_size       = (guint)(network_anomaly_timeout * 60 * ESTIMATED_FIX_RATE);

    g_rec_mutex_unlock(&mutex);
}


static gpointer
GnssFixesTransmissionQueue_worker_thread(
    GnssFixesTransmissionQueue       *self
)
{
    guint   txq_length;
    guint64 now;
    gulong  sleep_time = 0;

    for(;;) {
        if (sleep_time > 0)
            g_usleep(sleep_time);

        g_rec_mutex_lock(&mutex);
        if (self->priv->terminate == TRUE) {
            g_rec_mutex_unlock(&mutex);
            break;
        }

        if ( (connected == FALSE) || (g_queue_get_length(self->priv->txq) == 0)) {
            sleep_time = 300000UL;
            g_rec_mutex_unlock(&mutex);
            continue;
        }

        if ((connected == TRUE) && (pending_message == TRUE)) {
            sleep_time = 10000UL;
            g_rec_mutex_unlock(&mutex);
            continue;
        }

        if ((txq_length = g_queue_get_length(self->priv->txq)) > 0) {
            now = g_get_real_time();

            if ((txq_length >= self->priv->max_packet_size)
             || ((now - self->priv->last_sent_time) > self->priv->packet_flush_timeout)
            ) {
                GArray *fixes = self->priv->pop(self);
                MessageComposer_send_message(self->priv->context->message_composer, fixes);

                for( int i=0; i < fixes->len; i++)
                    gnss_fix_data_destroy(g_array_index(fixes, GnssFixData *, i));
                g_array_free(fixes, TRUE);
            }
        }
        g_rec_mutex_unlock(&mutex);
    }


    g_rec_mutex_lock(&mutex);
    if (g_queue_get_length(self->priv->txq) > 0) {
        if (connected) {
            loginfo("there are pending fixes, draining");
            while(g_queue_get_length(self->priv->txq) > 0) {
                GArray *fixes = self->priv->pop(self);
                MessageComposer_send_message(self->priv->context->message_composer, fixes);

                for( int i=0; i < fixes->len; i++)
                    gnss_fix_data_destroy(g_array_index(fixes, GnssFixData *, i));
                g_array_free(fixes, TRUE);
            }
        } else {
            logwarn("there are pending fixes, saving (no connection)");
            GnssFixesTransmissionQueue_fix_writer(self);
        }
    }
    g_rec_mutex_unlock(&mutex);

    return NULL;
}

static void
GnssFixesTransmissionQueue_set_cut_off(
    GnssFixesTransmissionQueue      *self,
	gboolean                         active
)
{
    if ((self->priv->cut_off != active))
        logdbg("gnss fixes cut off %s", (active? "active" : "cleared"));

    self->priv->cut_off = active;
}




static void
on_network_connected(gpointer unused)
{
    g_rec_mutex_lock(&mutex);
    connected = TRUE;
    g_rec_mutex_unlock(&mutex);
}


static void
on_network_disconnected(gpointer unused)
{
    g_rec_mutex_lock(&mutex);
    connected = FALSE;
    g_rec_mutex_unlock(&mutex);
}



static void
on_publish_started(void *service, int message_id, const char *topic, int qos, int result)
{
    GRegex *re = NULL;
    GMatchInfo *match = NULL;
    re = g_regex_new("^com/telepass/k1/tolling/[^/][^/]/v\\d+/\\d+/posdata$",(GRegexCompileFlags)0, (GRegexMatchFlags)0, NULL);
    if(g_regex_match_full(re, topic, -1, 0, (GRegexMatchFlags)0, &match, NULL)) {
        g_rec_mutex_lock(&mutex);
        pending_message = TRUE;
        pending_message_id = message_id;
        g_rec_mutex_unlock(&mutex);
    }

    if (match) g_match_info_free(match);
    if (re)    g_regex_unref(re);
}



static void
on_publish_completed(void *service, int message_id)
{
    g_rec_mutex_lock(&mutex);
    pending_message = FALSE;
    pending_message_id = 0;
    g_rec_mutex_unlock(&mutex);
}

#define MAXIMUM_CHAR_FIX_ID_LEN 30

#define CHECK_LEN_READ(__VAR__,__TYPE__,__NUM__,__FILE__,__LABEL__){\
			gsize read_data = fread(__VAR__,1,__NUM__*sizeof(__TYPE__),__FILE__);\
			if (read_data!=(sizeof(__TYPE__)*__NUM__)){\
				logerr("READ %d -- BYTE REQUESTED %d",read_data,sizeof(__TYPE__)*__NUM__);\
				goto __LABEL__;\
			}\
		}
#define CHECK_LEN_READ_DIGEST(__VAR__,__TYPE__,__NUM__,__FILE__,__LABEL__,__MD5_CONTEXT__){\
		CHECK_LEN_READ(__VAR__,__TYPE__,__NUM__,__FILE__,__LABEL__)\
		MD5_Update(__MD5_CONTEXT__, __VAR__, __NUM__*sizeof(__TYPE__));\
	}
#define CLOSE_AND_DELETE_FILE(__FILE__,__RESOURCE_ID__){\
		if(__FILE__!=NULL)\
			fclose(__FILE__);\
		rm_delete_resource(RM_FIXES_PATH,__RESOURCE_ID__);\
	}
#define CHECK_LEN_WRITE(__VAR__,__TYPE__,__NUM__,__FILE__,__LABEL__){\
		gsize write_data = fwrite(__VAR__,1,__NUM__*sizeof(__TYPE__),__FILE__);\
		if (write_data!=(sizeof(__TYPE__)*__NUM__)){\
			logerr("WROTE %d -- BYTE REQUESTED %d",write_data,sizeof(__TYPE__)*__NUM__);\
			goto __LABEL__;\
		}\
	}
#define CHECK_LEN_WRITE_DIGEST(__VAR__,__TYPE__,__NUM__,__FILE__,__LABEL__,__MD5_CONTEXT__){\
		CHECK_LEN_WRITE(__VAR__,__TYPE__,__NUM__,__FILE__,__LABEL__)\
		MD5_Update(__MD5_CONTEXT__, __VAR__, __NUM__*sizeof(__TYPE__));\
	}
static gboolean GnssFixesTransmissionQueue_fix_writer(GnssFixesTransmissionQueue *self){
	//TODO:obtain app_name
	guint queue_len = g_queue_get_length(self->priv->txq);
	guint version = FIX_SAVE_VERSION;
	guint no_id_size = 0;
	gboolean return_value = FALSE;
	if (queue_len == 0){
		loginfo("No fix must be saved");
		return TRUE;
	}

	MD5_CTX md5_context;
	guchar md5_digest[MD5_DIGEST_LENGTH];
	MD5_Init(&md5_context);

	GString *resource_id_str = g_string_new(self->priv->domain_name->str);
	GString *resource_id_hash_str = g_string_new(self->priv->domain_name->str);
	resource_id_str = g_string_append(resource_id_str,".fix");
	resource_id_hash_str = g_string_append(resource_id_hash_str,".hash");
	gchar *resource_id = resource_id_str->str;
	gchar *resource_id_hash = resource_id_hash_str->str;
	loginfo("%d fixes must be saved",queue_len);

	FILE *to_write = rm_get_resource_as_file(RM_FIXES_PATH, resource_id,"w");
	FILE *to_write_hash = rm_get_resource_as_file(RM_FIXES_PATH, resource_id_hash,"w");
	//Writing version
	CHECK_LEN_WRITE_DIGEST(&version,guint,1,to_write,closing,&md5_context);
	//Writing fix_num
	CHECK_LEN_WRITE_DIGEST(&queue_len,guint,1,to_write,closing,&md5_context);
	//Writing fix data
	GnssFixData* fix = g_queue_pop_head(self->priv->txq);
	while(fix!=NULL){
		//Writing Nth fix struct
		CHECK_LEN_WRITE_DIGEST(fix,GnssFixData,1,to_write,closing,&md5_context);
		//Writing Nth fix and status of the fix struct (Note that the fix and status will always be there because the new of the GNSSFixData always allocate it)
		if(fix->data_id != NULL && fix->data_id->len>0){
			//Writing string len is the size of the string withouth the null terminated. Remember to add that when reading
			CHECK_LEN_WRITE_DIGEST(&fix->data_id->len,gsize,1,to_write,closing,&md5_context);
			//Writing the dataid string
			CHECK_LEN_WRITE_DIGEST(fix->data_id->str,gchar,fix->data_id->len,to_write,closing,&md5_context);

		} else {
			//Writing string len
			CHECK_LEN_WRITE_DIGEST(&no_id_size,gsize,1,to_write,closing,&md5_context);
		}
		//TODO:free fix
		gnss_fix_data_destroy(fix);
		fix = g_queue_pop_head(self->priv->txq);
	}
	MD5_Final(md5_digest, &md5_context);
	CHECK_LEN_WRITE(md5_digest,guchar,MD5_DIGEST_LENGTH,to_write_hash,closing);
	return_value = TRUE;

	closing:
	fclose(to_write);
	fclose(to_write_hash);
	if (!return_value){
		logerr("Something went wrong while saving the fixes");
		rm_delete_resource(RM_FIXES_PATH,resource_id);
		rm_delete_resource(RM_FIXES_PATH,resource_id_hash);
	}
	g_string_free(resource_id_str,TRUE);
	g_string_free(resource_id_hash_str,TRUE);
	//TODO: check how to be sure to write the data at this point. the rm_get_resource_as_file is providing safe_open which enables that but
	//it's better to ask
	return return_value;
}

static gboolean GnssFixesTransimissionQueue_fix_reader(GnssFixesTransmissionQueue *self){
	GString *resource_id_str = g_string_new(self->priv->domain_name->str);
	GString *resource_id_hash_str = g_string_new(self->priv->domain_name->str);
	resource_id_str = g_string_append(resource_id_str,".fix");
	resource_id_hash_str = g_string_append(resource_id_hash_str,".hash");
	gchar *resource_id = resource_id_str->str;
	gchar *resource_id_hash = resource_id_hash_str->str;
	gboolean return_value = FALSE;
	GnssFixData *fix_Data = NULL;
	//TODO: check if the file exists
	FILE *to_read = rm_get_resource_as_file(RM_FIXES_PATH, resource_id,"r");
	FILE *to_read_hash = rm_get_resource_as_file(RM_FIXES_PATH, resource_id_hash,"r");
	if (to_read == NULL || to_read_hash == NULL){
		loginfo("No Fixes/hash saved File found");
		CLOSE_AND_DELETE_FILE(to_read,resource_id);
		CLOSE_AND_DELETE_FILE(to_read_hash,resource_id_hash);
		g_string_free(resource_id_str,TRUE);
		g_string_free(resource_id_hash_str,TRUE);
		return TRUE;
	}
	MD5_CTX md5_context;
	guchar md5_digest[MD5_DIGEST_LENGTH],md5_digest_read[MD5_DIGEST_LENGTH];
	CHECK_LEN_READ(md5_digest_read,guchar,MD5_DIGEST_LENGTH,to_read_hash,closing);
	MD5_Init(&md5_context);
	guint version = 0; guint num_fixes = 0;
	CHECK_LEN_READ_DIGEST(&version,guint,1,to_read,closing,&md5_context);
	loginfo("Version found %d",version);
	if (version == FIX_SAVE_VERSION){
		loginfo("Version found %d",version);
	} else {
		//TODO: delete the file
		logerr("Version not equal");
		goto closing;
	}
	CHECK_LEN_READ_DIGEST(&num_fixes,guint,1,to_read,closing,&md5_context);
	if (num_fixes == 0){
		logerr("No fixes found");
		goto closing;
	}

	loginfo("Found %d fixes",num_fixes);
	gchar data_id[200];
	for (int i=0;i<num_fixes;i++){
		fix_Data = gnss_fix_data_new();
		GString *fix_data_id = fix_Data->data_id;
		CHECK_LEN_READ_DIGEST(fix_Data,GnssFixData,1,to_read,closing,&md5_context);
		fix_Data->data_id = fix_data_id;

		gsize string_len = 0;
		CHECK_LEN_READ_DIGEST(&string_len,gsize,1,to_read,closing,&md5_context);
		if(string_len>0){
			if (string_len>MAXIMUM_CHAR_FIX_ID_LEN){
				logerr("String len corrupted. Discarding fixes");
				goto closing;
			}
			CHECK_LEN_READ_DIGEST(data_id,gchar,string_len,to_read,closing,&md5_context);
			data_id[string_len] = '\0';
			g_string_printf(fix_Data->data_id, "%s",data_id);
		}
		g_queue_push_tail(self->priv->txq,fix_Data);
		fix_Data = NULL;

	}
	//CHECK HASH
	MD5_Final(md5_digest, &md5_context);
	return_value = memcmp(md5_digest,md5_digest_read,MD5_DIGEST_LENGTH)==0;
	loginfo("HASH IS %s VALID",return_value?"":"NOT");
//	return_value = TRUE;
	//THIS PART CHECK
	closing: if (fix_Data){
		gnss_fix_data_destroy(fix_Data);
	}
	//CHECK se return value == TRUE se non è così allora bisogna ripulire la coda
	if(!return_value){
		loginfo("destroying previously read fixes because the file was probably corrupted");
		while(g_queue_get_length(self->priv->txq) > 0)
			gnss_fix_data_destroy(g_queue_pop_head(self->priv->txq));
	}
	CLOSE_AND_DELETE_FILE(to_read,resource_id);
	CLOSE_AND_DELETE_FILE(to_read_hash,resource_id_hash);
	g_string_free(resource_id_str,TRUE);
	g_string_free(resource_id_hash_str, TRUE);
	return return_value;

}

