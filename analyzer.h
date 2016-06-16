#ifndef _ANALYZER_H_
#define _ANALYZER_H_

#include <gtk/gtk.h>
#include "structs.h"

enum {
    DISTANCE_COLUMN,
    TIME_COLUMN,
    SPEED_COLUMN,
    MIN_ELEVATION_COLUMN,
    MAX_ELEVATION_COLUMN,
    START_COLUMN,
    END_COLUMN,
    NUMBER_OF_COLUMNS
};

void analyze_segments(SegListPtr list_ptr);
xy_scale *analyze_points(SegListPtr list_ptr);
GtkListStore *divide_by_turns(SegListPtr list_ptr, double min_angle, double min_time);
GtkListStore *divide_by_speed(SegListPtr list_ptr, double min_diff, double min_time);

#endif // _ANALYZER_H_
