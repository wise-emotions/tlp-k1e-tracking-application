/*  vas_activation_checker.c
 */
#include "glib.h"
#include "logger.h"
#include "vasmanagement.h"
#include "vas_manager_types.h"
#include "vas_activation_checker.h"

struct _VasActivationCheckerPriv  {
    guint                           service_id;
    char                           *service_name;
    gboolean                        is_active;

    activation_changed_callback_t   callback;
    gpointer                        client_data;

    VasManagement                  *vas_manager;
    gint                            vas_manager_watcher;

    void (*on_vas_manager_appeared)(
        GDBusConnection             *connection,
        const gchar                 *name,
        const gchar                 *name_owner,
        VasActivationChecker        *self
    );

    void (*on_service_activation) (
        VasManagement               *vas_manager,
        guint                        service_id,
        const gchar                 *service_name,
        guint                        service_status,
        VasActivationChecker        *self
    );
};


static void                             VasActivationChecker_ctor(VasActivationChecker *, guint);
static void                             VasActivationChecker_dtor(VasActivationChecker *);
static gboolean                         VasActivationChecker_get_service_activation(VasActivationChecker *,gboolean *);
static void                             VasActivationChecker_set_service_activation_changed_callbak(VasActivationChecker *,activation_changed_callback_t,gpointer);


static void                             VasActivationChecker_on_vas_manager_appeared(GDBusConnection *, const gchar *, const gchar *, VasActivationChecker *);
static void                             VasActivationChecker_on_service_activation(VasManagement *, guint, const gchar *, guint, VasActivationChecker *);


VasActivationChecker *
VasActivationChecker_new(void)
{
VasActivationChecker *obj;

    obj = g_try_new0(VasActivationChecker, 1);
    g_return_val_if_fail(obj != NULL, NULL);

    obj->priv = g_try_new0(VasActivationCheckerPriv,1);
    if (obj->priv == NULL) {
        g_return_if_fail_warning(NULL, "VasActivationChecker_new()", "obj->priv != NULL");
        g_free(obj);
        return NULL;
    }
    obj->impl.dtor                                    = (void *)VasActivationChecker_dtor;
    obj->impl.get_service_activation                  = (void *)VasActivationChecker_get_service_activation;
    obj->impl.set_service_activation_changed_callbak  = (void *)VasActivationChecker_set_service_activation_changed_callbak;

    obj->ctor                                         = VasActivationChecker_ctor;
    obj->dtor                                         = VasActivationChecker_dtor;
    obj->priv->on_vas_manager_appeared                = VasActivationChecker_on_vas_manager_appeared;
    obj->priv->on_service_activation                  = VasActivationChecker_on_service_activation;


    return obj;
}



static void
VasActivationChecker_ctor(
    VasActivationChecker            *self,
    guint                            service_id
)
{
    GError *error = NULL;

    self->priv->service_id = service_id;
    self->priv->service_name = g_strdup("");

    self->priv->vas_manager = vas_management_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        VAS_MANAGER_BUS_NAME,
        VAS_MANAGER_OBJ_PATH,
        NULL,
        &error
    );
    if (error == NULL) {
        g_signal_connect(self->priv->vas_manager, "service_activation_status_changed", G_CALLBACK(self->priv->on_service_activation), self);

        self->priv->vas_manager_watcher = g_bus_watch_name(
            G_BUS_TYPE_SYSTEM,
            VAS_MANAGER_BUS_NAME,
            G_BUS_NAME_WATCHER_FLAGS_NONE,
            (GBusNameAppearedCallback)self->priv->on_vas_manager_appeared,
            NULL,
            self,
            NULL
        );
    }
    else {
        logerr("error creating vas-manager proxy: error code: %d, error message: %s", error->code, error->message);
        g_error_free(error);
    }
}

static void
VasActivationChecker_dtor(
    VasActivationChecker            *self
)
{
    g_return_if_fail(self != NULL);

    if (self->priv->vas_manager_watcher > 0)
        g_bus_unwatch_name(self->priv->vas_manager_watcher);

    if (self->priv->vas_manager != NULL)
        g_object_unref(self->priv->vas_manager);


    g_free(self->priv->service_name);
    g_free(self->priv);
    g_free(self);


}

static gboolean
VasActivationChecker_get_service_activation(
    VasActivationChecker            *self,
    gboolean                        *active
)
{
    guint   status = 0U;
    GError *error  = NULL;

    vas_management_call_get_service_activation_status_sync(
        self->priv->vas_manager,
        self->priv->service_id,
        self->priv->service_name,
        &status,
        NULL,
        &error
     );
    if (error == NULL) {
        *active = self->priv->is_active = status != 0U;
        return TRUE;
    }
    else {
        logerr("error calling vas-manager get_service_activation_status(): error code: %d, error message: %s", error->code, error->message);
        g_error_free(error);
        return FALSE;
    }
}

static void
VasActivationChecker_set_service_activation_changed_callbak(
    VasActivationChecker            *self,
    activation_changed_callback_t    callback,
    gpointer                         client_data
)
{
    self->priv->callback = callback;
    self->priv->client_data = client_data;
}

static void VasActivationChecker_on_vas_manager_appeared(
    GDBusConnection            *connection,
    const gchar                *name,
    const gchar                *name_owner,
    VasActivationChecker       *self
)
{
    gboolean service_status = 0U;
    gboolean curr_status    = self->priv->is_active;

    if (self->impl.get_service_activation((ICustomActivationChecker *)self, &service_status)) {
        self->priv->is_active = service_status != 0;

        if (curr_status != self->priv->is_active) {
            loginfo("service %sactivated", (service_status? "":"de-"));
            if (self->priv->callback)
                self->priv->callback(
                    NULL,
                    self->priv->service_id,
                    self->priv->service_name,
                    service_status,
                    self->priv->client_data
                );
        }

    }
}


static void VasActivationChecker_on_service_activation(
    VasManagement              *vas_manager,
    guint                       service_id,
    const gchar                *service_name,
    guint                       service_status,
    VasActivationChecker       *self
)
{
    if (self->priv->service_id == service_id) {
        gboolean curr_status = self->priv->is_active;
        self->priv->is_active = service_status != 0;

        if (curr_status != self->priv->is_active) {
            loginfo("service %sactivated", (service_status? "":"de-"));
            if (self->priv->callback)
                self->priv->callback(
                    NULL,
                    self->priv->service_id,
                    self->priv->service_name,
                    service_status,
                    self->priv->client_data
                );
        }
        else {
            loginfo("service already %sactivated", (service_status? "":"de-"));
        }
    }
}

