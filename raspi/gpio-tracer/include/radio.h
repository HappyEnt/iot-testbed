#ifndef RADIO_H
#define RADIO_H

#include <glib.h>

#define RADIO_TARGET_FREQUENCY 433000000

GAsyncQueue *reference_timestamp_queue;

int radio_init();
int radio_deinit();
void radio_start_reception();

#endif /* RADIO_H */
