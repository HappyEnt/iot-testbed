#ifndef TYPES_H
#define TYPES_H

#include <time.h>
#include <sys/time.h>
#include <glib.h>

typedef enum match {
  MATCH_FALLING = 1,
  MATCH_RISING = 2,
} match_t;

typedef double timestamp_t;
#define TIMESTAMP_FMT PRIu64

// structure to represent trace data that is generated by the preprocess thread
// on the raw sigrok data from the logic analyzer
typedef struct trace {
  guint8 channel; // channel for which the state is recorded
  char state;
  timestamp_t timestamp_ns; // timestamp in nanoseconds
} trace_t;

typedef struct timestamp_pair {
  timestamp_t local_timestamp_ns;
  timestamp_t reference_timestamp_ns;
} timestamp_pair_t;

#define G_VARIANT_TIMESTAMP_PAIR "(tt)"


#endif /* TYPES_H */
