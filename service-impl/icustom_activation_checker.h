#ifndef __ICUSTOM_ACTIVATION_CHECKER_H__
#define __ICUSTOM_ACTIVATION_CHECKER_H__

#include <glib-object.h>

typedef struct _ICustomActivationChecker ICustomActivationChecker;

typedef void (*activation_changed_callback_t)(
    gpointer            proxy,
    guint               service_id,
    const gchar        *service_name,
    guint               service_status,
    gpointer            client_data
);

struct _ICustomActivationChecker {

    void (*dtor)(
        ICustomActivationChecker        *self
    );

    gboolean (*get_service_activation)(
        ICustomActivationChecker        *self,
        gboolean                        *active
    );

    void (*set_service_activation_changed_callbak)(
        ICustomActivationChecker        *self,
        activation_changed_callback_t    callback,
        gpointer                         client_data
    );
};


#endif /*__ICUSTOM_ACTIVATION_CHECKER_H__*/
