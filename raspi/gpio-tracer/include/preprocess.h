#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <glib.h>
#include <libsigrok/libsigrok.h>
#include <output_module.h>
#include <types.h>

#define ANALYZER_FREQUENCY 8000000

typedef enum clock_state {
  WAIT, // wait for reference signal
  OFFSET, // clock has received at least one reference signal
  FREQ, // clock has received at least two reference signals
} clock_state_t;

typedef enum process_state {
  STOPPED,
  RUNNING,
  ERROR,
} process_state_t;

struct channel_mode {
  guint8 channel;
  gint8 mode;
};

struct lclock {
  clock_state_t state;

  gint64 nom_freq; // nominal frequency
  /* struct precision_time curr_t; */
  guint64 freq;

  guint64 nom_frequency;
  guint64 adjusted_frequency;

  /* struct precision_time prev_ref; */
  guint64 closed_seq;
  guint64 prev_time;

  guint64 free_seq;

  gint64 res_error;
  gint64 offset_adj;
  gint64 offset; // last offset from reference clock

  /* guint64 freq_offset; */
  gint64 freq_offset;
};

typedef struct preprocess_instance {
  process_state_t state;
  struct sr_context *sr_cntxt;
  struct sr_session *sr_session;
  struct sr_dev_inst *fx2ladw_dvc_instc;

  guint8 channel_count;
  char active_channel_mask;
  struct channel_mode *channels;

  GAsyncQueue *trace_queue;
  GAsyncQueue *timestamp_unref_queue;
  GAsyncQueue *timestamp_ref_queue;

  output_module_t *output;

  struct lclock local_clock;
} preprocess_instance_t;

/* --- proto --- */
int preprocess_init(preprocess_instance_t *process, output_module_t *output, GVariant *channel_modes, GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue);
int preprocess_stop_instance(preprocess_instance_t *process);

// TODO rename to something more meaninfull
gboolean preprocess_running(preprocess_instance_t* process);

static int preprocess_init_sigrok(preprocess_instance_t *process, guint32 samplerate);
static int preprocess_kill_instance(preprocess_instance_t *process);

#endif /* PREPROCESS_H */
