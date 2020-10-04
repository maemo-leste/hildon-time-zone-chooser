#include <time.h>
#include <clockd/libtime.h>
#include <hildon/hildon.h>
#include <cityinfo.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#include <libintl.h>

#include "hildon-time-zone-chooser.h"
#include "hildon-time-zone-pannable-map.h"
#include "hildon-time-zone-search.h"

#include "config.h"

struct _HildonTimeZoneChooser
{
  /** Current city */
  Cityinfo *cityinfo;
  /** HildonStackbleWindow that contains #vbox and #toolbar */
  GtkWidget *window;
  /** GtkVBox containing #HildonPannableMap and the #label */
  GtkWidget *vbox;
  /** HildonEditToobar for typing the city name*/
  GtkWidget *toolbar;
  /** HildonButton that starts searching */
  GtkWidget *search_button;
  /** #HildonPannableMap instance */
  HildonPannableMap *map;
  /** Holds the result of #hildon_time_zone_chooser_run */
  FeedbackDialogResponse response;
  /** Contains curently selected city timezone */
  GtkWidget *label;
  /** Periodic update timer id */
  guint run_timer_id;
};

#define _(domainname, msgid) dgettext(domainname, msgid)

static gboolean
_window_key_press_event_cb(GtkWidget *widget, GdkEventKey *event,
                           gpointer user_data)
{
  HildonTimeZoneChooser *chooser = user_data;

  switch (event->keyval)
  {
    case HILDON_HARDKEY_INCREASE:
      hildon_pannable_map_zoom_in(chooser->map);
      return TRUE;
    case HILDON_HARDKEY_DECREASE:
      hildon_pannable_map_zoom_out(chooser->map);
      return TRUE;
    case HILDON_HARDKEY_LEFT:
    case HILDON_HARDKEY_RIGHT:
    case HILDON_HARDKEY_DOWN:
    case HILDON_HARDKEY_UP:
      hildon_pannable_map_accelerate(chooser->map, event->keyval, 3.0);
      return TRUE;
  }

  return FALSE;
}

static void
_map_update_cb(const Cityinfo *city, gpointer user_data)
{
  HildonTimeZoneChooser *chooser = user_data;

  if (city && chooser &&
      chooser->response != FEEDBACK_DIALOG_RESPONSE_CITY_CHOSEN)
  {
    gchar *tz;
    gchar *markup;
    int utc_offset = time_get_utc_offset(cityinfo_get_zone(city));
    int utc_offset_hours = utc_offset / -3600;
    gchar *city_name = cityinfo_get_name(city);
    gchar *country = cityinfo_get_country(city);

    if (utc_offset % 3600)
    {
      int utc_offset_minutes = (utc_offset % 3600) / 60;
      const char *fmt;

      utc_offset_hours = utc_offset / -3600;

      if (utc_offset_minutes < 0)
          utc_offset_minutes = -utc_offset_minutes;

      fmt = _("osso-clock", "cloc_fi_timezonefull_minutes");
      tz = g_strdup_printf(fmt, utc_offset_hours, utc_offset_minutes,
                           city_name, country);
    }
    else
    {
      const char *fmt = _("osso-clock", "cloc_fi_timezonefull");

      tz = g_strdup_printf(fmt, utc_offset_hours, city_name, country);
    }

    markup = g_strdup_printf("<span>%s</span>", tz);
    gtk_label_set_markup(GTK_LABEL(chooser->label), markup);
    g_free(tz);
    g_free(markup);
  }
}

static void
_search_button_clicked(HildonButton *button, HildonTimeZoneChooser *chooser)
{
  HildonTimeZoneSearch *tz_search =
      hildon_time_zone_search_new(chooser->window);
  Cityinfo *city = hildon_pannable_map_get_city(chooser->map);

  hildon_time_zone_search_set_city(tz_search, city);

  if (hildon_time_zone_search_run(tz_search) == TRUE &&
      (city = hildon_time_zone_search_get_city(tz_search)) != 0 )
  {
    hildon_time_zone_chooser_set_city(chooser, city);
    _map_update_cb(city, chooser);
  }

  hildon_time_zone_search_free(tz_search);
}

static void
_toolbar_arrow_clicked_cb(HildonEditToolbar *widget, gpointer user_data)
{
  HildonTimeZoneChooser *chooser = user_data;

  chooser->response = FEEDBACK_DIALOG_RESPONSE_CANCELLED;
  hildon_pannable_map_stop(chooser->map);
  g_source_remove(chooser->run_timer_id);
  chooser->run_timer_id = 0;
  gtk_widget_hide_all(chooser->window);
  gtk_main_quit();
}

static void
_window_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  _toolbar_arrow_clicked_cb((HildonEditToolbar *)widget, data);
}

void
_toolbar_button_clicked_cb(HildonEditToolbar *widget, gpointer user_data)
{
  HildonTimeZoneChooser *chooser = user_data;
  Cityinfo *city;

  chooser->response = FEEDBACK_DIALOG_RESPONSE_CITY_CHOSEN;
  gtk_widget_hide_all(chooser->window);
  hildon_pannable_map_stop(chooser->map);

  g_source_remove(chooser->run_timer_id);
  chooser->run_timer_id = 0;

  city = hildon_pannable_map_get_city(chooser->map);

  if (city)
  {
    hildon_time_zone_chooser_set_city(chooser, city);
    cityinfo_free(city);
  }

  gtk_main_quit();
}

HildonTimeZoneChooser *
hildon_time_zone_chooser_new()
{
  GdkPixbuf *icon;
  HildonTimeZoneChooser *chooser = g_try_new0(HildonTimeZoneChooser, 1);

  if (!chooser)
    return NULL;

  chooser->response = FEEDBACK_DIALOG_RESPONSE_UNKNOWN;

  chooser->window = hildon_stackable_window_new();
  hildon_program_add_window(hildon_program_get_instance(),
                            HILDON_WINDOW(chooser->window));
  g_signal_connect(G_OBJECT(chooser->window), "key-press-event",
                   G_CALLBACK(_window_key_press_event_cb), chooser);

  chooser->label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(chooser->label), 0.5, 0.0);

  chooser->toolbar = hildon_edit_toolbar_new_with_text(
        _("osso-clock", "cloc_ia_choose_time_zone"),
        _("hildon-libs", "wdgt_bd_done"));
  hildon_window_set_edit_toolbar(HILDON_WINDOW(chooser->window),
                                 HILDON_EDIT_TOOLBAR(chooser->toolbar));

  gtk_window_set_title(GTK_WINDOW(chooser->window),
                       _("osso-clock", "cloc_ia_choose_time_zone"));

  chooser->vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(chooser->window), chooser->vbox);

  chooser->search_button =
      hildon_button_new(HILDON_SIZE_AUTO_WIDTH,
                        HILDON_BUTTON_ARRANGEMENT_VERTICAL);

  g_signal_connect(G_OBJECT(chooser->search_button), "clicked",
                   G_CALLBACK(_search_button_clicked), chooser);

  icon = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                  "general_search", 32, 0, 0);
  hildon_button_set_image(HILDON_BUTTON(chooser->search_button),
                          GTK_WIDGET(gtk_image_new_from_pixbuf(icon)));
  gtk_box_pack_start(
        GTK_BOX(chooser->toolbar), chooser->search_button, FALSE, FALSE, 0);

  gtk_box_reorder_child(GTK_BOX(chooser->toolbar), chooser->search_button, 1);

  g_signal_connect(G_OBJECT(chooser->toolbar), "arrow_clicked",
                   G_CALLBACK(_toolbar_arrow_clicked_cb), chooser);
  g_signal_connect(G_OBJECT(chooser->toolbar), "button_clicked",
                   G_CALLBACK(_toolbar_button_clicked_cb), chooser);
  g_signal_connect(G_OBJECT(chooser->window), "delete_event",
                   G_CALLBACK(_window_delete_event_cb), chooser);

  chooser->map = hildon_pannable_map_new_default();
  gtk_box_pack_start(GTK_BOX(chooser->vbox),
                     hildon_pannable_map_get_top_widget(chooser->map), TRUE,
                     TRUE, 2);
  hildon_pannable_map_set_update_cb(chooser->map, _map_update_cb, chooser);
  gtk_box_pack_start(GTK_BOX(chooser->vbox), chooser->label, FALSE, FALSE, 2);

  g_object_unref(icon);

  return chooser;
}

void
hildon_time_zone_chooser_set_city(HildonTimeZoneChooser *chooser,
                                  Cityinfo *cityinfo)
{
  if (cityinfo)
  {
    if (chooser->cityinfo)
      cityinfo_free(chooser->cityinfo);

    chooser->cityinfo = cityinfo_clone(cityinfo);
    hildon_pannable_map_set_city(chooser->map, chooser->cityinfo);
  }
}

Cityinfo *
hildon_time_zone_chooser_get_city(HildonTimeZoneChooser *chooser)
{
  return cityinfo_clone(chooser->cityinfo);
}

void
hildon_time_zone_chooser_free(HildonTimeZoneChooser *chooser)
{
  gtk_widget_hide_all(chooser->window);
  hildon_pannable_map_free(chooser->map);
  gtk_widget_destroy(chooser->window);
  cityinfo_free(chooser->cityinfo);
  g_free(chooser);
}

static gboolean
run_timeout_cb(gpointer user_data)
{
  HildonTimeZoneChooser *chooser = user_data;

  if (chooser)
  {
    static struct tm local_time;
    Cityinfo *city;

    time_get_local(&local_time);
    chooser->run_timer_id =
        gdk_threads_add_timeout(1000 * (60 - local_time.tm_sec) + 500,
                                run_timeout_cb, chooser);
    city = hildon_pannable_map_get_city(chooser->map);

    if (city)
    {
      _map_update_cb(city, chooser);
      cityinfo_free(city);
    }
  }

  return FALSE;
}

FeedbackDialogResponse
hildon_time_zone_chooser_run(HildonTimeZoneChooser *chooser)
{
  GdkWindow *window;
  GdkDisplay *dpy;
  unsigned long val = 1;

  gtk_widget_show_all(chooser->window);
  gtk_window_fullscreen(GTK_WINDOW(chooser->window));

  window = gtk_widget_get_window (chooser->window);
  dpy = gdk_window_get_display(window);

  XChangeProperty(gdk_x11_display_get_xdisplay(dpy),
                  gdk_x11_drawable_get_xid(window),
                  gdk_x11_get_xatom_by_name_for_display(
                    dpy, "_HILDON_ZOOM_KEY_ATOM"),
                  XA_INTEGER, 32, PropModeReplace, (unsigned char *)&val, 1);

  run_timeout_cb(chooser);
  gtk_main();

  hildon_pannable_map_clear_cache();

  return chooser->response;
}
