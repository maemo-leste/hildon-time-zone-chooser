/*
 * hildon-time-zone-search.c
 *
 * Copyright (C) 2020 Ivaylo Dimitrov <ivo.g.dimitrov.75@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <libintl.h>
#include <time.h>

#include <clockd/libtime.h>

#include "hildon-time-zone-search.h"

struct _HildonTimeZoneSearch
{
  Cityinfo *city;
  GtkWidget *parent;
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *selector;
  GtkTreeModel *tree_model;
  Cityinfo **cities;
  gboolean changed;
};

static void
_selector_value_changed(HildonPickerButton *widget,
                        gpointer user_data)
{
  HildonTimeZoneSearch *search = user_data;
  GtkTreeIter iter; // [esp+18h] [ebp-20h]
  Cityinfo *city = NULL;

  g_assert(NULL != search);

  if (hildon_touch_selector_get_selected(
        HILDON_TOUCH_SELECTOR(search->selector), 0, &iter))
  {
    gtk_tree_model_get(search->tree_model, &iter, 1, &city, -1);

    if (city)
      cityinfo_free(search->city);

    search->city = cityinfo_clone(city);
  }

  search->changed = TRUE;
  gtk_widget_hide_all(search->dialog);
}

HildonTimeZoneSearch *
hildon_time_zone_search_new(GtkWidget *parent)
{
  GtkCellRenderer *cr;
  HildonTouchSelectorColumn *col;
  HildonTimeZoneSearch *search;
  GtkWidget *vbox;
  GtkListStore *list_store;
  GtkTreeIter iter;
  Cityinfo **c;

  g_assert(NULL != parent);

  search = g_try_new0(HildonTimeZoneSearch, 1);

  if (!search)
    return NULL;

  search->changed = FALSE;
  search->parent = parent;
  search->city = NULL;
  search->cities = cityinfo_get_all();

  search->dialog = gtk_dialog_new_with_buttons(
        dgettext("osso-clock", "cloc_ti_search_city_title"),
        GTK_WINDOW(search->parent),
        GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT |
        GTK_DIALOG_MODAL,
        NULL);
  gtk_widget_set_size_request(GTK_WIDGET(search->dialog), 750, 350);

  vbox = gtk_vbox_new(FALSE, 16);
  search->button = hildon_picker_button_new(HILDON_SIZE_AUTO_WIDTH,
                                            HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  search->selector = GTK_WIDGET(hildon_touch_selector_new());
  gtk_box_pack_start(GTK_BOX(vbox), search->selector, TRUE, TRUE, 0);

  search->tree_model = GTK_TREE_MODEL(
        gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT,
                           G_TYPE_INT));
  list_store = GTK_LIST_STORE(search->tree_model);

  for (c = search->cities; *c; c++)
  {
    Cityinfo *city = *c;
    int utc_offset = time_get_utc_offset(cityinfo_get_zone(city));
    int utc_offset_m = utc_offset % 3600;
    gchar *country = cityinfo_get_country(city);
    gchar *name = cityinfo_get_name(city);
    gchar *tz;
    gchar *tmp;

    gtk_list_store_append(list_store, &iter);


    if (utc_offset_m)
    {
      const char *format =
          dgettext("osso-clock", "cloc_fi_timezonefull_minutes");

      tz = g_strdup_printf(
            format, utc_offset / -3600,
            utc_offset_m <= 59 ? utc_offset_m / -60 : utc_offset_m / 60, name,
            country);
    }
    else
    {
      const char *format = dgettext("osso-clock", "cloc_fi_timezonefull");

      tz = g_strdup_printf(format, utc_offset / -3600, name, country);
    }

    gtk_list_store_set(list_store, &iter, 0, tz, 1, city, 2, 5, -1);

    tmp = g_utf8_casefold(city->name, -1);
    g_free(city->name);
    city->name = tmp;

    tmp = g_utf8_casefold(city->country, -1);
    g_free(city->country);
    city->country = tmp;

    g_free(city->locale);
    city->locale = g_utf8_casefold(tz, -1);

    g_free(tz);
  }

  if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter))
  {
    int i = 0;

    do
    {
      gtk_list_store_set(list_store, &iter,
                         3, ++i,
                         2, 4,
                         -1);
    }
    while (gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter));
  }

  cr = gtk_cell_renderer_text_new();
  gtk_cell_renderer_set_fixed_size(cr, 355, -1);
  col = hildon_touch_selector_append_column(
        HILDON_TOUCH_SELECTOR(search->selector), search->tree_model,
        cr, "text", 0,
        NULL);

  hildon_touch_selector_column_set_text_column(col, 0);

  hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(search->button),
                                    HILDON_TOUCH_SELECTOR(search->selector));

  g_signal_connect(G_OBJECT(search->button), "value-changed",
                   G_CALLBACK(_selector_value_changed), search);

  gtk_box_pack_start(
        GTK_BOX(GTK_DIALOG(search->dialog)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show(vbox);

  return search;
}

gboolean
hildon_time_zone_search_run(HildonTimeZoneSearch *tz_search)
{
  if (tz_search->city)
  {
    GtkTreeIter iter;
    Cityinfo *city = NULL;
    gint id = cityinfo_get_id(tz_search->city);

    if (id != -1)
    {
      if (gtk_tree_model_get_iter_first(tz_search->tree_model, &iter))
      {
        gint index = 0;

        gtk_tree_model_get(tz_search->tree_model, &iter,
                           1, &city,
                           -1);

        while (id != cityinfo_get_id(city))
        {
          if (!gtk_tree_model_iter_next(tz_search->tree_model, &iter))
            goto out;

          index++;
          gtk_tree_model_get(tz_search->tree_model, &iter,
                             1, &city,
                             -1);
        }

        hildon_touch_selector_select_iter(
              HILDON_TOUCH_SELECTOR(tz_search->selector), 0, &iter, TRUE);
        hildon_touch_selector_set_active(
              HILDON_TOUCH_SELECTOR(tz_search->selector), 0, index);
      }
    }
  }

out:
  gtk_widget_show_all(tz_search->dialog);
  gtk_dialog_run(GTK_DIALOG(tz_search->dialog));

  return tz_search->changed;
}

Cityinfo *
hildon_time_zone_search_get_city(HildonTimeZoneSearch *tz_search)
{
  return cityinfo_from_id(cityinfo_get_id(tz_search->city));
}

void
hildon_time_zone_search_free(HildonTimeZoneSearch *tz_search)
{
  gtk_widget_hide_all(tz_search->dialog);
  gtk_widget_destroy(tz_search->dialog);
  cityinfo_free(tz_search->city);
  cityinfo_free_all(tz_search->cities);
  g_free(tz_search);
}

void
hildon_time_zone_search_set_city(HildonTimeZoneSearch *tz_search,
                                 const Cityinfo *city)
{
  if (tz_search->city)
  {
    cityinfo_free(tz_search->city);
    tz_search->city = NULL;
  }

  tz_search->city = cityinfo_clone(city);
}
