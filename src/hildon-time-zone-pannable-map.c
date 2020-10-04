#include <cityinfo.h>
#include <hildon/hildon.h>
#include <clock/clockcore-cities.h>
#include <math.h>

#include "hildon-time-zone-pannable-map.h"

struct _HildonPannableMap
{
  GtkWidget *canvas;
  GdkRegion *region;
  int view_width;
  int view_height;
  Cityinfo *city;
  gboolean interactive;
  gboolean transparent;
  float step;
  gint zoom_factor;
  gint line_width;
  float width;
  float height;
  float scale;
  float cross_x;
  float cross_y;
  guint motion_timeout_id;
  guint stop_timeout_id;
  float dest_x;
  float dest_y;
  guint32 button_motion_time;
  guint32 button_press_time;
  float button_press_x;
  float button_press_y;
  float button_press_x_f;
  float button_press_y_f;
  hildon_pannable_map_update_fn update_cb;
  gpointer update_cb_data;
};

enum {
  ZOOM_HALF,
  ZOOM_NOR,
  ZOOM_DOUBLE,
  ZOOM_LAST
};

static GdkPixbuf *maps_images[ZOOM_LAST] = {};
static GdkPixbuf *cross_image = NULL;

static void
stop_motion_timer(HildonPannableMap *map)
{
  if (map->interactive)
  {
    if (map->motion_timeout_id)
    {
      g_source_remove(map->motion_timeout_id);
      map->motion_timeout_id = 0;
      map->dest_x = 0.0;
      map->dest_y = 0.0;
    }
  }
}

void
hildon_pannable_map_stop(HildonPannableMap *map)
{
  if (map)
    stop_motion_timer(map);
}

Cityinfo *
hildon_pannable_map_get_city(HildonPannableMap *map)
{
  if (map)
    return cityinfo_clone(map->city);

  return NULL;
}

static void
do_callback(HildonPannableMap *map)
{
  double y;
  double x;
  Cityinfo *city;

  for (x = map->width / -1500.0; x >= 1.0; x -= 1.0)
    ;

  while (x < 0.0)
    x += 1.0;

  for (y = map->height / -919.0; y >= 1.0; y -= 1.0)
    ;

  while ( y < 0.0 )
    y += 1.0;

  city = clock_find_nearest_location(x, y, cityinfo_get_id(map->city));

  if (city)
  {
    cityinfo_free(map->city);
    map->city = city;

    if (map->interactive && map->update_cb)
      map->update_cb(city, map->update_cb_data);
  }
}

static void
hildon_pannable_map_redraw(HildonPannableMap *map)
{
  gtk_widget_queue_draw(map->canvas);
}

static gboolean
do_redraw(gpointer user_data)
{
  HildonPannableMap *map = user_data;
  double dest_x;
  double dest_y;

  if (!map->interactive || !map->motion_timeout_id)
    return FALSE;

  map->width = map->width + map->dest_x;
  map->height = map->height + map->dest_y;
  map->dest_x = map->dest_x / map->step;
  map->dest_y = map->dest_y / map->step;

  do_callback(map);
  hildon_pannable_map_redraw(map);

  dest_x = map->dest_x;
  dest_y = map->dest_x;

  if (dest_x <= 0.0)
    dest_x = -dest_x;

  if (dest_x < 0.24)
  {
    dest_y = 0.0;
    map->dest_x = 0.0;
  }

  if (fabsf(map->dest_y) < 0.24)
    map->dest_y = 0.0;

  if (fabs(dest_y) >= 0.24)
    return TRUE;

  if (fabsf(map->dest_y) < 0.24)
  {
    stop_motion_timer(map);
    return FALSE;
  }

  return TRUE;
}

static void
schedule_redraw(HildonPannableMap *map)
{
  if (map->interactive)
  {
    if (!map->motion_timeout_id)
      map->motion_timeout_id = g_timeout_add(40, do_redraw, map);
  }
}

void
hildon_pannable_map_accelerate(HildonPannableMap *map, gint keyval,
                               float factor)
{
  if (!map || !map->interactive)
    return;

  switch (keyval)
  {
    case GDK_KEY_Up:
    {
      map->dest_y += factor;
      break;
    }
    case GDK_KEY_Down:
    {
      map->dest_y -= factor;
      break;
    }
    case GDK_KEY_Right:
    {
      map->dest_x -= factor;
      break;
    }
    case GDK_KEY_Left:
    {
      map->dest_x += factor;
      break;
    }
    case GDK_KEY_Return:
    {
        stop_motion_timer(map);
        return;
    }
  }

  schedule_redraw(map);
}

void
hildon_pannable_map_set_update_cb(HildonPannableMap *map,
                                  hildon_pannable_map_update_fn cb,
                                  gpointer user_data)
{
  map->update_cb_data = user_data;
  map->update_cb = cb;
}

static void
create_maps_image(HildonPannableMap *map, int zoom_factor)
{
  float scale;
  float w;
  float h;

  if (zoom_factor == ZOOM_DOUBLE)
  {
    scale = 2.0;
    map->scale = scale;
  }
  else if (zoom_factor == ZOOM_NOR)
  {
    map->scale = 1.0;
    return;
  }
  else
    map->scale = 0.444;

  if (!maps_images[zoom_factor])
  {
    w = gdk_pixbuf_get_width(maps_images[ZOOM_NOR]);
    h = gdk_pixbuf_get_height(maps_images[ZOOM_NOR]);

    maps_images[zoom_factor] = gdk_pixbuf_scale_simple(
          maps_images[ZOOM_NOR], w * map->scale, h * map->scale,
          GDK_INTERP_BILINEAR);
  }
}

void
hildon_pannable_map_zoom_out(HildonPannableMap *map)
{
  if (map)
  {
    int zoom_factor = map->zoom_factor - 1;

    map->zoom_factor = zoom_factor;

    if (zoom_factor >= 0)
    {
      create_maps_image(map, zoom_factor);
      hildon_pannable_map_redraw(map);
    }
    else
      map->zoom_factor = ZOOM_HALF;
  }
}

void
hildon_pannable_map_zoom_in(HildonPannableMap *map)
{
  if (map)
  {
    int zoom_factor = map->zoom_factor + 1;

    map->zoom_factor = zoom_factor;

    if (zoom_factor > ZOOM_DOUBLE)
      map->zoom_factor = ZOOM_DOUBLE;
    else
    {
      if (zoom_factor == ZOOM_NOR)
      {
        g_object_unref(maps_images[ZOOM_HALF]);
        maps_images[ZOOM_HALF] = 0;
        zoom_factor = map->zoom_factor;
      }

      create_maps_image(map, zoom_factor);
      hildon_pannable_map_redraw(map);
    }
  }
}

HildonPannableMap *
hildon_pannable_map_new_default()
{
  return hildon_pannable_map_new(1.2, TRUE, 1, 0);
}

GtkWidget *
hildon_pannable_map_get_top_widget(HildonPannableMap *map)
{
  if (map)
    return map->canvas;

  return NULL;
}

static void
_draw_transaprent_background(HildonPannableMap *map)
{
  GtkWidget *parent;

  if (!map->transparent)
    return;

  parent = gtk_widget_get_parent(map->canvas);

  if (!parent)
    return;

  if (map->transparent)
  {
    cairo_t *cr = gdk_cairo_create(GDK_DRAWABLE(parent));
    cairo_rectangle(cr, map->canvas->allocation.x + 2,
                    map->canvas->allocation.y + map->line_width + 2,
                    map->view_width + 2, map->view_height);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.25);
    cairo_fill(cr);
    cairo_destroy(cr);
  }
}

static void
_load_data(HildonPannableMap *map)
{
  if (!cross_image || !maps_images[ZOOM_NOR])
  {
    gchar *filename;

    cross_image = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                           "clock_destination", 48, 0, NULL);

    g_assert(NULL != cross_image);

    filename = g_build_filename("/usr/share/icons/hicolor/scalable/hildon",
                                "clock_worldmap_time_chooser.jpg", NULL);
    maps_images[ZOOM_NOR] = gdk_pixbuf_new_from_file(filename, NULL);

    g_assert(NULL != maps_images[ZOOM_NOR]);
    g_free(filename);
    map->cross_x =
        0.5f * (float)(map->view_width - gdk_pixbuf_get_width(cross_image));
    map->cross_y =
        0.5f * (float)(map->view_height - gdk_pixbuf_get_height(cross_image));
  }
}

static void
_draw_map_image(HildonPannableMap *map)
{
  gint view_w;
  gint view_h;
  gint width;
  gint height;
  gint dest_y;
  gint dest_x;
  gint src_x;
  gint src_y;
  gint h;
  gint w;

  if (!maps_images[ZOOM_NOR])
    return;

  w = gdk_pixbuf_get_width(maps_images[map->zoom_factor]);
  h = gdk_pixbuf_get_height(maps_images[map->zoom_factor]);

  view_w = map->view_width;
  view_h = map->view_height;

  src_x = (map->scale * -map->width) - view_w / 2;
  src_y = (map->scale * -map->height) - view_h / 2;

  if (src_x < 0)
  {
    gint i = src_x + w;

    do
    {
      src_x = i;
      i += w;
    }
    while (i - w < 0);
  }

  if (src_y < 0)
  {
    gint i = src_y + h;

    do
    {
      src_y = i;
      i += h;
    }
    while (i - h < 0);
  }

  if (w < src_x)
  {
    gint i = src_x - w;

    do
    {
      src_x = i;
      i -= w;
    }
    while (w < i + w);
  }

  if (h < src_y)
  {
    gint i = src_y - h;

    do
    {
      src_y = i;
      i -= h;
    }
    while (h < i + h);
  }

  dest_y = 0;

  do
  {
    if (h + dest_y > view_h)
      height = view_h - dest_y;
    else
      height = h;

    if (h < src_y + height)
      height = h - src_y;

    dest_x = 0;

    do
    {
      if (w + dest_x <= view_w)
        width = w;
      else
        width = view_w - dest_x;

      if (w < width + src_x)
        width = w - src_x;

      gdk_draw_pixbuf(GDK_DRAWABLE(map->canvas->window), NULL,
                      maps_images[map->zoom_factor],
                      src_x, src_y, dest_x, dest_y, width, height,
                      GDK_RGB_DITHER_NONE, 0, 0);

      dest_x += width;
      view_w = map->view_width;
      src_x = 0;
    }
    while (dest_x < view_w);

    dest_y += height;
    view_h = map->view_height;
    src_y = 0;
  }
  while (dest_y < view_h);
}

static void
_draw_cross_image(HildonPannableMap *map)
{
  if (!cross_image)
    return;

  gdk_draw_pixbuf(GDK_DRAWABLE(map->canvas->window), NULL, cross_image,
                  0, 0, map->cross_x, map->cross_y,
                  gdk_pixbuf_get_width(cross_image),
                  gdk_pixbuf_get_height(cross_image),
                  GDK_RGB_DITHER_NONE, 0, 0);
}

static void
_draw_border(HildonPannableMap *map)
{
  if (map->line_width)
  {
    static GdkGC *gc = NULL;

    if (!gc )
    {
      GdkColor color = {};

      gc = gdk_gc_new(GDK_DRAWABLE(map->canvas->window));
      gdk_gc_set_foreground(gc, &color);
    }

    gdk_gc_set_line_attributes(gc, map->line_width, GDK_LINE_SOLID,
                               GDK_CAP_BUTT, GDK_JOIN_MITER);
    gdk_draw_rectangle(GDK_DRAWABLE(map->canvas->window), gc, FALSE, 0, 0,
                       map->view_width - 1, map->view_height - 1);
  }
}

static gboolean
_canvas_expose_cb(GtkWidget *widget, GdkEventExpose *event,
                  HildonPannableMap *map)
{
  _load_data(map);

  gdk_window_begin_paint_region(map->canvas->window, map->region);

  _draw_map_image(map);
  _draw_cross_image(map);
  _draw_border(map);
  _draw_transaprent_background(map);

  gdk_window_end_paint(map->canvas->window);

  return 0;
}

static gboolean
_canvas_configure_cb(GtkWidget *widget, GdkEventConfigure *event,
                     HildonPannableMap *map)
{
  gint w;
  gint h;
  GdkRectangle rectangle;

  if (map->region)
  {
    gdk_region_destroy(map->region);
    map->region = NULL;
  }

  rectangle.x = 0;
  rectangle.y = 0;
  rectangle.width = widget->allocation.width;
  rectangle.height = widget->allocation.height;

  map->region = gdk_region_rectangle(&rectangle);

  w = widget->allocation.width;
  map->view_width = w;
  h = widget->allocation.height;
  map->view_height = h;

  if (cross_image)
  {
    map->cross_x = 0.5f * (float)(w - gdk_pixbuf_get_width(cross_image));
    map->cross_y = 0.5f * (float)(h - gdk_pixbuf_get_height(cross_image));
  }
  else
  {
    map->cross_x = 0.5f * (float)w;
    map->cross_y = 0.5f * (float)h;
  }

  return FALSE;
}

static gboolean
_canvas_button_press_cb(GtkWidget *widget, GdkEventButton *event,
                        HildonPannableMap *map)
{
  if (map->stop_timeout_id)
  {
    g_source_remove(map->stop_timeout_id);
    map->stop_timeout_id = 0;
  }

  stop_motion_timer(map);

  map->button_motion_time = event->time;
  map->button_press_time = event->time;
  map->button_press_x = event->x;
  map->button_press_y = event->y;
  map->button_press_x_f = map->width -
      (map->button_press_x - (float)(map->view_width / 2)) / map->scale;
  map->button_press_y_f = map->height -
      (map->button_press_y - (float)(map->view_height / 2)) / map->scale;

  return FALSE;
}

static gboolean
stop_redraw(gpointer user_data)
{
  HildonPannableMap *map = user_data;

  stop_motion_timer(map);
  map->width = map->button_press_x_f;
  map->height = map->button_press_y_f;
  do_callback(map);
  hildon_pannable_map_redraw(map);
  map->stop_timeout_id = 0;

  return FALSE;
}

static gboolean
_canvas_button_release_cb(GtkWidget *widget, GdkEventButton *event,
                          HildonPannableMap *map)
{
  guint32 button_motion_time = map->button_motion_time;

  if (event->time - map->button_press_time < 200)
  {
    if (map->stop_timeout_id)
      g_source_remove(map->stop_timeout_id);

    map->stop_timeout_id = g_timeout_add(100, stop_redraw, map);
  }

  map->button_motion_time = 0;
  map->button_press_x = 0.0;
  map->button_press_y = 0.0;

  if ((event->time - button_motion_time < 200) &&
      (fabsf(map->dest_x) > 0.24 || fabsf(map->dest_y) > 0.24))
  {
      schedule_redraw(map);
  }

  return FALSE;
}

static gboolean
_canvas_motion_notify_cb(GtkWidget *widget, GdkEventMotion *event,
                         HildonPannableMap *map)
{
  if (event->state & GDK_BUTTON1_MASK)
  {
    float dx = (event->x - map->button_press_x) / map->scale;
    float dy = (event->y - map->button_press_y) / map->scale;
    float dt = event->time - map->button_motion_time;

    if (dt <= 0.0)
      dt = 1.0;

    map->width = map->width + dx;
    map->height = map->height + dy;
    map->dest_x = dx * 40.0 / dt * 0.3 + 0.7 * map->dest_x;
    map->dest_y = 0.7 * map->dest_y + 0.3 * (40.0 * dy / dt);

    map->button_motion_time = event->time;
    map->button_press_x = event->x;
    map->button_press_y = event->y;

    do_callback(map);
    hildon_pannable_map_redraw(map);
  }

  return FALSE;
}

HildonPannableMap *
hildon_pannable_map_new(float step, gboolean interactive, gboolean border,
                        gboolean transparent)
{
  HildonPannableMap *map = g_try_new0(HildonPannableMap, 1);

  if (!map)
    return NULL;

  map->dest_y = 0.0;

  if (border)
    map->line_width = 2;

  map->dest_x = 0.0;
  map->zoom_factor = ZOOM_NOR;
  map->scale = 1.0;
  map->interactive = interactive;
  map->transparent = transparent;
  map->step = step;
  map->motion_timeout_id = 0;
  map->stop_timeout_id = 0;
  map->width = 750.0;
  map->height = 459.5;
  map->canvas = gtk_drawing_area_new();

  g_assert(NULL != map->canvas);

  gtk_widget_add_events(GTK_WIDGET(map->canvas), 0x8304);

  g_signal_connect(G_OBJECT(map->canvas), "expose_event",
                   G_CALLBACK(_canvas_expose_cb), map);
  g_signal_connect(G_OBJECT(map->canvas), "configure_event",
                   G_CALLBACK(_canvas_configure_cb), map);

  if (map->interactive)
  {
    g_signal_connect(G_OBJECT(map->canvas), "button-press-event",
                     G_CALLBACK(_canvas_button_press_cb), map);
    g_signal_connect(G_OBJECT(map->canvas), "button-release-event",
                     G_CALLBACK(_canvas_button_release_cb), map);
    g_signal_connect(G_OBJECT(map->canvas), "motion-notify-event",
                     G_CALLBACK(_canvas_motion_notify_cb), map);
  }

  return map;
}

void
hildon_pannable_map_set_city(HildonPannableMap *map, const Cityinfo *city)
{
  if (map && city)
  {
    if (map->city)
      cityinfo_free(map->city);

    map->city = cityinfo_clone(city);

    map->width = cityinfo_get_xpos(map->city) * -1500.0;
    map->height = cityinfo_get_ypos(map->city) * -919.0;

    hildon_pannable_map_redraw(map);
  }
}

void
hildon_pannable_map_clear_cache()
{
  if (maps_images[ZOOM_HALF])
  {
    g_object_unref(maps_images[ZOOM_HALF]);
    maps_images[ZOOM_HALF] = NULL;
  }

  if (maps_images[ZOOM_DOUBLE])
  {
    g_object_unref(maps_images[ZOOM_DOUBLE]);
    maps_images[ZOOM_DOUBLE] = NULL;
  }
}

void
hildon_pannable_map_free(HildonPannableMap *map)
{
  if (!map)
    return;

  stop_motion_timer(map);

  if (map->region)
  {
    gdk_region_destroy(map->region);
    map->region = NULL;
  }

  gtk_widget_hide_all(map->canvas);
  gtk_widget_destroy(map->canvas);
  cityinfo_free(map->city);

  hildon_pannable_map_clear_cache();

  g_free(map);
}
