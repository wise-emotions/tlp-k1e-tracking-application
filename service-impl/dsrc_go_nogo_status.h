/*
 * dsrc_go_nogo_status.h
 *
 *  Created on: May 26, 2022
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#ifndef DSRC_GO_NOGO_STATUS_H_
#define DSRC_GO_NOGO_STATUS_H_

typedef enum {
	go,
	nogo
} GoNogo_t;

typedef struct _DsrcGoNogoStatus DsrcGoNogoStatus;
typedef struct _ApplicationEvents ApplicationEvents;
typedef struct _DsrcServiceProxy DsrcServiceProxy;

DsrcGoNogoStatus *DsrcGoNogoStatus_new(ApplicationEvents *application_events, DsrcServiceProxy *dsrc_service_proxy);
void DsrcGoNogoStatus_destroy(DsrcGoNogoStatus *self);

GoNogo_t DsrcGoNogoStatus_get_current_dsrc_status(const DsrcGoNogoStatus *self);
guint    DsrcGoNogoStatus_get_current_dsrc_status_flags(const DsrcGoNogoStatus *self);

GoNogo_t DsrcGoNogoStatus_get_current_app_status(const DsrcGoNogoStatus *self);

void DsrcGoNogoStatus_set_app_no_go_service(DsrcGoNogoStatus *self, gboolean is_active);
void DsrcGoNogoStatus_set_app_no_go_network(DsrcGoNogoStatus *self, gboolean is_active);
void DsrcGoNogoStatus_set_app_no_go_gnss(DsrcGoNogoStatus *self, gboolean is_active);

#endif /* DSRC_GO_NOGO_STATUS_H_ */
