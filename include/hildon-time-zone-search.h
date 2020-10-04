typedef struct _HildonTimeZoneSearch HildonTimeZoneSearch;

HildonTimeZoneSearch *
hildon_time_zone_search_new(GtkWidget *parent);

void
hildon_time_zone_search_set_city(HildonTimeZoneSearch *tz_search,
                                 const Cityinfo *city);

gboolean
hildon_time_zone_search_run(HildonTimeZoneSearch *tz_search);

Cityinfo *
hildon_time_zone_search_get_city(HildonTimeZoneSearch *tz_search);

void
hildon_time_zone_search_free(HildonTimeZoneSearch *tz);
