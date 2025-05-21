/*
 * axles_change_manager.h
 *
 *  Created on: Mar 2, 2022
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#ifndef AXLES_CHANGE_MANAGER_H_
#define AXLES_CHANGE_MANAGER_H_

#include <glib.h>

typedef struct _AxlesChangeManager AxlesChangeManager;
typedef struct _Tolling_Gnss_Sm_Data Tolling_Gnss_Sm_Data;

AxlesChangeManager *AxlesChangeManager_new(Tolling_Gnss_Sm_Data *data);
void AxlesChangeManager_destroy(AxlesChangeManager *self);

void AxlesChangeManager_notify_axles_change(AxlesChangeManager *self, const guint new_axles, const gchar *trn_id);
void AxlesChangeManager_axles_change_requested(AxlesChangeManager *self, const guint new_axles, const gchar *trn_id);
void AxlesChangeManager_axles_change_ack_callback(AxlesChangeManager *self, const char *message);
void     AxlesChangeManager_set_and_persist_current_total_axles(AxlesChangeManager *self, guint new_axles);

#endif /* AXLES_CHANGE_MANAGER_H_ */
