#ifndef HILDON_TIME_ZONE_CHOOSER_H
#define HILDON_TIME_ZONE_CHOOSER_H

typedef struct _HildonTimeZoneChooser HildonTimeZoneChooser;

typedef enum
{
  /** A city was chosen */
  FEEDBACK_DIALOG_RESPONSE_CITY_CHOSEN = 1,

  /** The HildonTimeZoneChooser window was closed, without choosing a city */
  FEEDBACK_DIALOG_RESPONSE_CANCELLED
} FeedbackDialogResponse;

/**
 * @returns A new HildonTimeZoneChooser instance, or NULL. Free with
 *          #hildon_time_zone_chooser_free().
 */
HildonTimeZoneChooser *
hildon_time_zone_chooser_new(void);

/**
 * @brief Display the HildonTimeZoneChooser.
 *
 * @param chooser A HildonTimeZoneChooser instance.
 *
 * @returns A value in the #FeedbackDialogResponse enum.
 */
FeedbackDialogResponse
hildon_time_zone_chooser_run(HildonTimeZoneChooser *chooser);

/**
 * @brief Sets the city to be displayed in the HildonTimeZoneChooser.
 *
 * @param chooser A HildonTimeZoneChooser instance.
 * @param cityinfo A Cityinfo instance.
 */
void
hildon_time_zone_chooser_set_city (HildonTimeZoneChooser *chooser,
                                   Cityinfo *cityinfo);

/**
 * @brief - Gets the currently displayed city in the HildonTimeZoneChooser.
 *
 * @param chooser A HildonTimeZoneChooser instance.
 *
 * @returns A newly-allocated Cityinfo instance. Free with cityinfo_free().
 */
Cityinfo *
hildon_time_zone_chooser_get_city (HildonTimeZoneChooser *chooser);

/**
 * @brief Frees the allocated HildonTimeZoneChooser instance returned
 *        #from hildon_time_zone_chooser_new().
 *
 * @param chooser A HildonTimeZoneChooser instance.
 */
void
hildon_time_zone_chooser_free (HildonTimeZoneChooser *chooser);

#endif /* HILDON_TIME_ZONE_CHOOSER_H */
