/*
 * connection_sm.h
 *
 * Created on: Dec 4, 2019
 * Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#ifndef CONNECTION_SM_H_
#define CONNECTION_SM_H_

#include "state_iface.h"
//#include "connection.h"


/*
 * forward declaration
 */
typedef struct _ConnectionSm ConnectionSm;
typedef struct _Tolling_Gnss_Sm_Data Tolling_Gnss_Sm_Data;

ConnectionSm *connection_sm_new(Tolling_Gnss_Sm_Data* data);
void connection_sm_destroy(ConnectionSm *self);

void connection_sm_not_connected(ConnectionSm *self);
void connection_sm_connected(ConnectionSm *self);
void connection_sm_send_tolling_data(ConnectionSm *self);


#endif /* CONNECTION_SM_H_ */
