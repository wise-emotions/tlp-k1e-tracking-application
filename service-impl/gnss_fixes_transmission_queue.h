/*  gnss_fixes_transmission_queue.h
 */
#ifndef __GNSS_FIXES_TRASMISSION_QUEUE_H__
#define __GNSS_FIXES_TRASMISSION_QUEUE_H__

#include <glib-object.h>

typedef struct _PositionData PositionData;
typedef struct _Tolling_Gnss_Sm_Data Tolling_Gnss_Sm_Data;

typedef struct _GnssFixesTransmissionQueue GnssFixesTransmissionQueue;
typedef struct _GnssFixesTransmissionQueuePriv  GnssFixesTransmissionQueuePriv;


struct _GnssFixesTransmissionQueue {
    GnssFixesTransmissionQueuePriv *priv;


    void (*delayed_activation)
    		 (GnssFixesTransmissionQueue *self);

    void (*ctor)(
        GnssFixesTransmissionQueue       *self,
        Tolling_Gnss_Sm_Data            *tolling,
        const gchar                    *domain_name
    );

    void (*dtor)(
        GnssFixesTransmissionQueue  *self
    );

    void (*push)(
        GnssFixesTransmissionQueue       *self,
        const PositionData              *fix
    );

    GArray *(*head)(
        GnssFixesTransmissionQueue       *self
    );
};

extern GnssFixesTransmissionQueue *GnssFixesTransmissionQueue_new();

#endif /*__GNSS_FIXES_TRASMISSION_QUEUE_H__*/
