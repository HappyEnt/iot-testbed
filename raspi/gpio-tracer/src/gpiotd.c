 // reference https://github.com/joprietoe/gdbus/blob/master/gdbus-example-server.c

#include "gpiot.h"

#include <preprocess.h>
#include <postprocess.h>
#include <types.h>
#include <inttypes.h>
#include <radio-master.h>
#include <radio-slave.h>
#include <stdio.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <unistd.h>

static GDBusNodeInfo* introspection_data = NULL;
static GAsyncQueue *_trace_queue; // store queues so we can later safely unref them
static GAsyncQueue *_timestamp_unref_queue;
static GAsyncQueue *_timestamp_ref_queue;

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.cau.gpiot.ControllerInterface'>"
  "    <annotation name='org.cau.gpiot.Annotation' value='OnInterface'/>"
  "    <annotation name='org.cau.gpiot.Annotation' value='AlsoOnInterface'/>"
  "    <method name='Start'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='logPath' direction='in'/>"
  "      <arg type='s' name='nodeType' direction='in'/>"
  "      <arg type='s' name='result' direction='out'/>"
  "    </method>"
  "    <method name='Stop'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='result' direction='out'/>"
  "    </method>"
  "    <method name='getState'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='i' name='result' direction='out'/>"
  "    </method>"
  "    <signal name='Something'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='Onsignal'/>"
  "      <arg type='d' name='speed_in_mph'/>"
  "      <arg type='s' name='speed_as_string'>"
  "        <annotation name='org.cau.gpiot.Annotation' value='OnArg_NonFirst'/>"
  "      </arg>"
  "    </signal>"
  "    <property type='s' name='Status' access='read'/>"
  "  </interface>"
  "</node>";

static int start_tasks(GVariant* channel_modes, const gchar* nodeType) {
  _trace_queue           = g_async_queue_new();
  _timestamp_unref_queue = g_async_queue_new();
  _timestamp_ref_queue   = g_async_queue_new();

  preprocess_set_type(nodeType); // TODO coordination required. Setting after preprocess_init could result in deadlock

  // TODO it is not a particular nice design that with master/slave support the preprocess needs both a reference to the ref and unref queue
  // while only when in master mode the module needs both references.

  if (!(strcmp("master", nodeType))) {
    preprocess_init(channel_modes, _trace_queue, _timestamp_unref_queue, _timestamp_ref_queue);
    postprocess_init("/usr/testbed/sample_data/test1.csv", _trace_queue, _timestamp_ref_queue);
    radio_master_init(_timestamp_unref_queue, _timestamp_ref_queue);
  } else if (!(strcmp("slave", nodeType))) {
    radio_slave_init(_timestamp_unref_queue, _timestamp_ref_queue);
    preprocess_init(channel_modes, _trace_queue, _timestamp_unref_queue, _timestamp_ref_queue);
    postprocess_init("/usr/testbed/sample_data/test1.csv", _trace_queue, _timestamp_ref_queue);
    radio_slave_start_reception();
  }

}

static int stop_tasks() {
  preprocess_stop_instance();
  g_async_queue_unref(_trace_queue          );
  g_async_queue_unref(_timestamp_unref_queue);
  g_async_queue_unref(_timestamp_ref_queue  );
}

// dbus handlers
static void handle_method_call(GDBusConnection *connnection,
                               const gchar *sender, const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name, GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data) {

  // TODO should the error case return an error value?
  gchar *result;
  g_printf("handle Method invocation with %s\n", method_name);
  if (!g_strcmp0(method_name, "Start")) {
    int ret;
    const gchar *logpath;
    const gchar *nodeType;

    g_printf("Invocation: Start\n");

    g_variant_get(parameters, "(&s&s)", &logpath, &nodeType);
    g_printf("Parameters: %s %s\n", logpath, nodeType);

    GVariantBuilder* channel_modes_builder = g_variant_builder_new(G_VARIANT_TYPE("a(yy)"));
    GVariant* channel_modes;
    g_variant_builder_add(channel_modes_builder, "(yy)", 0, MATCH_FALLING | MATCH_RISING);
    g_variant_builder_add(channel_modes_builder, "(yy)", 1, MATCH_FALLING | MATCH_RISING);
    g_variant_builder_add(channel_modes_builder, "(yy)", 2, MATCH_FALLING | MATCH_RISING);
    g_variant_builder_add(channel_modes_builder, "(yy)", 3, MATCH_FALLING | MATCH_RISING);
    g_variant_builder_add(channel_modes_builder, "(yy)", 4, MATCH_FALLING | MATCH_RISING);
    g_variant_builder_add(channel_modes_builder, "(yy)", 5, MATCH_FALLING | MATCH_RISING);
    g_variant_builder_add(channel_modes_builder, "(yy)", 6, MATCH_FALLING | MATCH_RISING);
    g_variant_builder_add(channel_modes_builder, "(yy)", 7, MATCH_FALLING | MATCH_RISING);

    channel_modes = g_variant_new("a(yy)", channel_modes_builder);
    g_variant_builder_unref(channel_modes_builder);

    g_print(g_variant_print(channel_modes,TRUE));
    if ((ret = start_tasks(channel_modes, nodeType)) < 0) {
      result = g_strdup_printf("Unable to run instance");
    } else if (ret > 0) {
      result = g_strdup_printf("Instance already running");
    } else {
      result = g_strdup_printf("Started collection on device");
    }

    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", result));

  } else if (!g_strcmp0(method_name, "Stop")) {
    g_printf("Invocation: Stop\n");

    if (preprocess_running()) {
      int ret;
      if ((ret = stop_tasks()) >= 0) {
        result = g_strdup_printf("Stopped");
      } else {
        result = g_strdup_printf("Could not stop instance");
      }
    } else {
      result = g_strdup_printf("No instance running");
    }
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", result));
  } else if (!g_strcmp0(method_name, "getState")) {
    gpiot_daemon_state_t state;
    if(preprocess_running())
      state = GPIOTD_COLLECTING;
    else state = GPIOTD_IDLE;

    g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", state));
  }
}

static GVariant *handle_get_property(GDBusConnection *connection,
                                     const gchar *sender,
                                     const gchar *object_path,
                                     const gchar *interface_name,
                                     const gchar *property_name, GError **error,
                                     gpointer user_data) {
}

static gboolean handle_set_property(GDBusConnection *connection,
                                     const gchar *sender,
                                     const gchar *object_path,
                                     const gchar *interface_name,
                                     const gchar *property_name,
                                     GVariant* value,
                                     GError **error,
                                     gpointer user_data) {
}

static const GDBusInterfaceVTable interface_vtable = {
  handle_method_call,
  handle_get_property,
  handle_set_property,
};

static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
  // export objects from here
  guint registration_id;

  registration_id = g_dbus_connection_register_object(
      connection, "/org/cau/gpiot/Controller",
      introspection_data->interfaces[0], &interface_vtable, NULL, NULL, NULL);

  g_assert(registration_id > 0);
}

static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
  g_printf("acquired dbus name %s\n", name);
}

static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data) {
  g_printf("lost dbus name %s\n", name);
}


int main(int argc, char *argv[])
{
  GMainContext* main_context;
  guint owner_id;
  GBusNameOwnerFlags flags;

  g_printf("Started gpiot daemon!\n");

  main_context  =  g_main_context_new();
  g_main_context_push_thread_default(main_context);


  introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
  g_assert(introspection_data != NULL);

  // take name from other connection, but also allow others to take this connection
  flags = G_BUS_NAME_OWNER_FLAGS_REPLACE | G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT;
  owner_id = g_bus_own_name(G_BUS_TYPE_SESSION, "org.cau.gpiot", flags, on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL);
  g_printf("Setup gpio pin for gps flooding!\n");


  /* if(preprocess_init_sigrok(8000000) < 0) { */
  /*   g_printf("Could not initialize sigrok gpiot instance!\n"); */
  /*   goto cleanup; */
  /* } */

  while (1) { // TODO main loop
    g_main_context_iteration(main_context, TRUE);
  }

  // ------ destruction ------
cleanup:
    g_bus_unown_name(owner_id);
    g_dbus_node_info_unref(introspection_data);
    g_main_context_pop_thread_default(main_context);

  return 0;
}
