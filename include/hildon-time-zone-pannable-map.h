typedef struct _HildonPannableMap HildonPannableMap;

typedef void (*hildon_pannable_map_update_fn)(const Cityinfo *city,
                                              gpointer user_data);

HildonPannableMap *
hildon_pannable_map_new(float step, gboolean interactive, gboolean border,
                        gboolean transparent);

HildonPannableMap *
hildon_pannable_map_new_default(void);

GtkWidget *
hildon_pannable_map_get_top_widget(HildonPannableMap *map);

void
hildon_pannable_map_set_update_cb(HildonPannableMap *map,
                                  hildon_pannable_map_update_fn cb,
                                  gpointer user_data);

void
hildon_pannable_map_accelerate(HildonPannableMap *result, gint keyval,
                               float factor);

void
hildon_pannable_map_zoom_in(HildonPannableMap *map);

void
hildon_pannable_map_zoom_out(HildonPannableMap *map);

Cityinfo *
hildon_pannable_map_get_city(HildonPannableMap *map);

void
hildon_pannable_map_stop(HildonPannableMap *map);
