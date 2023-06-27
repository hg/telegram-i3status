#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

static char label_cache[100][sizeof("99")];

static void init_label_cache(void) {
  for (size_t i = 0; i < ARRAY_SIZE(label_cache); ++i) {
    snprintf(label_cache[i], sizeof(label_cache[0]), "%ld", i);
  }
}

static void check_sd_error(int err, const char *msg) {
  if (err < 0) {
    fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(-err));
    exit(err);
  }
}

static const char *status_color(int64_t count) {
  if (count == 0) {
    return "Idle";
  }
  if (count < 10) {
    return "Info";
  }
  if (count < 50) {
    return "Warning";
  }
  return "Critical";
}

static void set_status(sd_bus *bus, int64_t count) {
  const char *color = status_color(count);

  const char *text;
  char buf[20];

  if (count >= 0 && count < ARRAY_SIZE(label_cache)) {
    text = label_cache[count];
  } else {
    snprintf(buf, sizeof(buf), "%ld", count);
    text = buf;
  }

  int err =
      sd_bus_call_method(bus, "i3.status.rs", "/telegram", "i3.status.rs",
                         "SetStatus", NULL, NULL, "sss", text, "bell", color);
  check_sd_error(err, "call SetStatus");
}

static int on_count_changed(sd_bus_message *msg, void *userdata,
                            sd_bus_error *ret_error) {
  const char *sender;

  int err = sd_bus_message_read_basic(msg, SD_BUS_TYPE_STRING, &sender);
  check_sd_error(err, "get sender name");

  if (!sender || strcmp(sender, "application://org.telegram.desktop.desktop")) {
    return 0;
  }

  err = sd_bus_message_enter_container(msg, SD_BUS_TYPE_ARRAY, "{sv}");
  check_sd_error(err, "open container");

  int64_t count = 0;

  while ((err = sd_bus_message_enter_container(msg, SD_BUS_TYPE_DICT_ENTRY,
                                               "sv")) > 0) {
    const char *field;

    err = sd_bus_message_read_basic(msg, SD_BUS_TYPE_STRING, &field);
    check_sd_error(err, "read field name");

    const char *contents;
    err = sd_bus_message_peek_type(msg, NULL, &contents);
    check_sd_error(err, "peek message type");

    err = sd_bus_message_enter_container(msg, SD_BUS_TYPE_VARIANT, contents);
    check_sd_error(err, "enter variant container");

    if (contents[0] != SD_BUS_TYPE_INT64 || contents[1] != '\0') {
      continue;
    }

    if (strcmp(field, "count")) {
      continue;
    }

    err = sd_bus_message_read_basic(msg, SD_BUS_TYPE_INT64, &count);
    check_sd_error(err, "read count");
  }

  if (count >= 0) {
    set_status((sd_bus *)userdata, count);
  }

  return 0;
}

int main(int argc, char **argv) {
  init_label_cache();

  sd_bus *bus;

  int err = sd_bus_default_user(&bus);
  check_sd_error(err, "open user bus");

  err = sd_bus_match_signal(bus, NULL, NULL, NULL,
                            "com.canonical.Unity.LauncherEntry", "Update",
                            on_count_changed, bus);
  check_sd_error(err, "attach signal handler");

  sd_event *event;
  err = sd_event_default(&event);
  check_sd_error(err, "get event loop");

  err = sd_bus_attach_event(bus, event, 0);
  check_sd_error(err, "attach event");

  err = sd_event_loop(event);
  check_sd_error(err, "run event loop");

  return 0;
}
