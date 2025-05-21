/*
 *  Created on: Nov 5, 2021
 *  Author: Fabio Turati (fabio.turati@nttdata.com)
 */

#pragma once

#include "message_composer.h"

typedef struct _MessageComposer MessageComposer;

void MessageComposer_send_message(MessageComposer *self, const GArray* fixes_to_send); // == message_composer.h

MessageComposer *ApplicationMessageComposer_new(Tolling_Gnss_Sm_Data* data);
void ApplicationMessageComposer_destroy(MessageComposer *self);
