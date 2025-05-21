/*
 * message_composer.h
 *
 *  Created on: Oct 27, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#ifndef MESSAGE_COMPOSER_H_
#define MESSAGE_COMPOSER_H_

#include "gnss_fix_data.h"
#include "positioning_service_types.h"

typedef struct _Tolling_Gnss_Sm_Data Tolling_Gnss_Sm_Data;
typedef struct _GArray GArray;
typedef struct _JsonMapper JsonMapper;
typedef struct _GString GString;

typedef struct _MessageComposer {
	Tolling_Gnss_Sm_Data  *tolling_gnss_sm_data;
} MessageComposer;


MessageComposer* MessageComposer_new(Tolling_Gnss_Sm_Data* data);
void MessageComposer_destroy(MessageComposer *self);

JsonMapper *MessageComposer_create_payload(const MessageComposer *self, const gchar* msg_type);
JsonMapper *MessageComposer_create_fixdata_json_mapper(const GArray* fixes_to_send);
JsonMapper *MessageComposer_create_payload_json_mapper(const MessageComposer *self, const GArray* fixes_to_send, const gchar* msg_type);
GString    *MessageComposer_create_message_from_fixes(MessageComposer *self, const GArray* fixes_to_send);
GString *MessageComposer_create_event_message_pos(const MessageComposer *self, const PositionData fix, const gchar* msg_type);

void MessageComposer_send_message(MessageComposer *self, const GArray* fixes_to_send);

#endif /* MESSAGE_COMPOSER_H_ */
