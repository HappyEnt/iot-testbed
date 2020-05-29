// https://www.man7.org/linux/man-pages/man2/clock_gettime.2.html
#include "timestamp.h"
#include <stdio.h>

unsigned long b_size; // block size
uint32_t sample_count;

time_t start_time; // TODO a start time is also passed by the first datafeed header packet.
time_t last_time_stamp;
 // Use unsigned long or uint64_t? Is it better to let the program fail on compile while being explicit on how long a time capture may take
 // or let the system run on a best effort basis -> e.g. 32 bit raspberyr pi overflowing after ~70 minutes?
/* unsigned long last_time_stamps_micros; */

uint64_t samples_per_second;

int init_clock(test_configuration_t* configuration, unsigned long block_size) {
  start_time = time(NULL);
  last_time_stamp = 0;
  samples_per_second = configuration->samplerate;
  b_size = block_size;
  return 1;
}

void next_system_timestamp() {
  last_time_stamp = time(NULL) - start_time;
}

// TODO split micro_precision_timestamp_t into struct containing seconds and micro seconds
micro_precision_timestamp_t get_timestamp() { // this should be handled some other way
  // microsecond timestamp
  return 1000000 * ((double)last_time_stamp + (double) sample_count / samples_per_second);
}
