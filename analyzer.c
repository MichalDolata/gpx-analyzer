#include "analyzer.h"
#include "structs.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define R       6371
#define M_PI	3.14159265358979323846
#define DEG_TO_RAD(x) (x) * M_PI / 180.0
#define RAD_TO_DEG(x) (x) * 180.0/M_PI
#define MICROSEC_IN_SEC 1000000
#define SEC_IN_HOUR 3600

typedef struct {
    double x, y;
} m_point;

static double min_longitude, max_longitude, mid_longitude;
static double def_reduction;

double square(double x)
{
    return x*x;
}

double vector_length(m_point v)
{
    return sqrt(square(v.x)+square(v.y));
}

double longitude_to_x(double longitude)
{
    return def_reduction*(longitude-mid_longitude);
}

double latitude_to_y(double latitude)
{
    return latitude;
}

// oblicza statystyki dla segmentów
void analyze_segments(SegListPtr list_ptr)
{
    SegmentPtr tmp_seg = get_first_segment(list_ptr);
    while(tmp_seg != NULL) {
        double lat2 = get_latitude(get_end_point(tmp_seg));
        double lat1 = get_latitude(get_start_point(tmp_seg));
        double dlon = DEG_TO_RAD(get_longitude(get_end_point(tmp_seg)) - get_longitude(get_start_point(tmp_seg)));
        double dlat = DEG_TO_RAD(lat2 - lat1);

        lat2 = DEG_TO_RAD(lat2);
        lat1 = DEG_TO_RAD(lat1);

        double a = square(sin(dlat/2))+square(sin(dlon/2))*cos(lat1)*cos(lat2);
        a = 2*atan2(sqrt(a), sqrt(1-a));
        double distance = R * a;

        double time = g_date_time_difference(get_time(get_end_point(tmp_seg)),get_time(get_start_point(tmp_seg)))/MICROSEC_IN_SEC;
        time /= SEC_IN_HOUR;

        set_distance(tmp_seg, distance);
        add_track_distance(list_ptr, distance);
        add_track_time(list_ptr, time);
        set_dtime(tmp_seg, time);
        double speed = distance/time;
        if(time == 0)
            speed = 0;
        set_speed(tmp_seg, speed);
        if(get_maxspeed(list_ptr) < speed)
            set_maxspeed(list_ptr, speed);
        if(get_minspeed(list_ptr) > speed)
            set_minspeed(list_ptr, speed);
        tmp_seg = get_next_segment(tmp_seg);
    }
}

// przygotowuje punkty do rysowania
// Odwzorowanie walcowe równoodległościowe
xy_scale* analyze_points(SegListPtr list_ptr)
{
    SegmentPtr tmp_seg = get_first_segment(list_ptr);
    PointPtr tmp_point = get_start_point(tmp_seg);

    min_longitude = get_minlongitude(list_ptr);
    max_longitude = get_maxlongitude(list_ptr);
    mid_longitude = (min_longitude + max_longitude)/2;
    def_reduction = cos((get_minlatitude(list_ptr)+get_maxlatitude(list_ptr))/2);
    xy_scale *transform_scale = (xy_scale*)malloc(sizeof(xy_scale));
    transform_scale->max_x =  transform_scale->max_y = -181;
    transform_scale->min_x = transform_scale->min_y = 181;

    double x,y;
    x = longitude_to_x(get_longitude(tmp_point));
    y = latitude_to_y(get_latitude(tmp_point));
    set_xy(tmp_point, x, y);
    if(x > transform_scale->max_x)
        transform_scale->max_x = x;
    if(x <transform_scale->min_x)
        transform_scale->min_x = x;
    if(y > transform_scale->max_y)
        transform_scale->max_y = y;
    if(y < transform_scale->min_y)
        transform_scale->min_y = y;

    while(tmp_seg != NULL) {
        tmp_point = get_end_point(tmp_seg);
        x = longitude_to_x(get_longitude(tmp_point));
        y = latitude_to_y(get_latitude(tmp_point));

        set_xy(tmp_point, x, y);
        if(x > transform_scale->max_x)
            transform_scale->max_x = x;
        if(x <transform_scale->min_x)
            transform_scale->min_x = x;
        if(y > transform_scale->max_y)
            transform_scale->max_y = y;
        if(y < transform_scale->min_y)
            transform_scale->min_y = y;
        tmp_seg = get_next_segment(tmp_seg);
    }
    return transform_scale;
}

GtkListStore *divide_by_turns(SegListPtr list_ptr, double min_angle, double min_time)
{
    GtkListStore *to_return = gtk_list_store_new(NUMBER_OF_COLUMNS, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE,
        G_TYPE_DOUBLE, G_TYPE_UINT, G_TYPE_UINT);
    GtkTreeIter iter;
    guint start, i;
    start = 0;
    i = 0;
    gdouble distance, time, angle_sum, elev_min, elev_max, elev1, elev2;
    angle_sum = time = distance = elev1 = elev2 = elev_max = 0;
    gdouble min_elevation, max_elevation;
    min_elevation = DBL_MAX;
    max_elevation = DBL_MIN;

    SegmentPtr tmp_seg = get_first_segment(list_ptr);
    PointPtr tmp_point;
    m_point p1, p2, p3;
    m_point v1, v2;
    double prev_cross_product, cross_product;
    prev_cross_product = cross_product = 0;
    while(get_next_segment(tmp_seg) != NULL) {
        // obliczanie kąta skrętu
        tmp_point = get_start_point(tmp_seg);
        p1.x = get_x(tmp_point); p1.y = get_y(tmp_point);
        elev1 = get_elevation(tmp_point);
        tmp_point = get_end_point(tmp_seg);
        p2.x = get_x(tmp_point); p2.y = get_y(tmp_point);
        elev2 = get_elevation(tmp_point);

        distance += get_distance(tmp_seg);
        time += get_dtime(tmp_seg);
        if(elev1 > elev2) {
            elev_max = elev1;
            elev_min = elev2;
        }
        else {
            elev_max = elev2;
            elev_min = elev1;
        }

        if(elev_max > max_elevation)
            max_elevation = elev_max;
        if(elev_min < min_elevation)
            min_elevation = elev_min;

        tmp_seg = get_next_segment(tmp_seg);
        tmp_point = get_end_point(tmp_seg);
        p3.x = get_x(tmp_point); p3.y = get_y(tmp_point);

        v1.x = p2.x - p1.x; v1.y = p2.y - p1.y;
        v2.x = p3.x - p2.x; v2.y = p3.y - p2.y;
        cross_product = v1.x * v2.y - v1.y * v2.x;

        // iloczny wektorowy
        if(cross_product * prev_cross_product < 0) {
            angle_sum = 0;
            prev_cross_product = 0;
        }
        else
            prev_cross_product = cross_product;

        // ze wzoru na iloczyn skalarny
        // http://matematyka.pisz.pl/strona/1630.html
        angle_sum += RAD_TO_DEG(acos((v1.x*v2.x+v1.y*v2.y)/(vector_length(v1)*vector_length(v2))));
        if(angle_sum >= min_angle && time*SEC_IN_HOUR >= min_time) {
            gtk_list_store_append(to_return, &iter);
            gtk_list_store_set(to_return, &iter, DISTANCE_COLUMN, distance,
                TIME_COLUMN, time, SPEED_COLUMN, distance/time, MIN_ELEVATION_COLUMN, min_elevation,
                MAX_ELEVATION_COLUMN, max_elevation, START_COLUMN, start, END_COLUMN, i, -1);

            angle_sum = 0;
            distance = time = 0;
            prev_cross_product = cross_product = 0;
            min_elevation = DBL_MAX;
            max_elevation = DBL_MIN;
            start = i+1;
        }
        i++;
    }
    gtk_list_store_append(to_return, &iter);
    gtk_list_store_set(to_return, &iter, DISTANCE_COLUMN, distance,
                TIME_COLUMN, time, SPEED_COLUMN, distance/time, MIN_ELEVATION_COLUMN, min_elevation,
                MAX_ELEVATION_COLUMN, max_elevation, START_COLUMN, start, END_COLUMN, i, -1);

    return to_return;
}

GtkListStore *divide_by_speed(SegListPtr list_ptr, double min_diff, double min_time)
{
    GtkListStore *to_return = gtk_list_store_new(NUMBER_OF_COLUMNS, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE,
        G_TYPE_DOUBLE, G_TYPE_UINT, G_TYPE_UINT);
    GtkTreeIter iter;
    guint start, i;
    start = 0;
    i = 0;
    gdouble distance, time, elev_min, elev_max, elev1, elev2;
    gdouble speed1, speed2, speed_difference_sum, speed_diference;
    speed_difference_sum = distance = elev1 = elev2 = elev_max = 0;
    gdouble min_elevation, max_elevation;
    min_elevation = DBL_MAX;
    max_elevation = DBL_MIN;

    SegmentPtr tmp_seg = get_first_segment(list_ptr);
    PointPtr tmp_point;

    while(get_next_segment(tmp_seg) != NULL) {
        tmp_point = get_start_point(tmp_seg);
        elev1 = get_elevation(tmp_point);
        tmp_point = get_end_point(tmp_seg);
        elev2 = get_elevation(tmp_point);

        distance += get_distance(tmp_seg);
        time += get_dtime(tmp_seg);
        speed1 = get_speed(tmp_seg);

        if(elev1 > elev2) {
            elev_max = elev1;
            elev_min = elev2;
        }
        else {
            elev_max = elev2;
            elev_min = elev1;
        }

        if(elev_max > max_elevation)
            max_elevation = elev_max;
        if(elev_min < min_elevation)
            min_elevation = elev_min;

        tmp_seg = get_next_segment(tmp_seg);
        speed2 = get_speed(tmp_seg);

        speed_diference = speed1 - speed2;

        // wykrywanie skrętu w przeciwną stronę
        if(speed_difference_sum * speed_diference < 0.0) {
            speed_difference_sum = speed_diference;
        }
        else {
            speed_difference_sum += speed_diference;
        }

        if(ABS(speed_difference_sum) >= min_diff && time*SEC_IN_HOUR >= min_time) {
            gtk_list_store_append(to_return, &iter);
            gtk_list_store_set(to_return, &iter, DISTANCE_COLUMN, distance,
                TIME_COLUMN, time, SPEED_COLUMN, distance/time, MIN_ELEVATION_COLUMN, min_elevation,
                MAX_ELEVATION_COLUMN, max_elevation, START_COLUMN, start, END_COLUMN, i+1, -1);
            speed_difference_sum = 0;
            distance = time = 0;
            min_elevation = DBL_MAX;
            max_elevation = DBL_MIN;
            start = i+2;
        }
        i++;
    }

    gtk_list_store_append(to_return, &iter);
    gtk_list_store_set(to_return, &iter, DISTANCE_COLUMN, distance,
                TIME_COLUMN, time, SPEED_COLUMN, distance/time, MIN_ELEVATION_COLUMN, min_elevation,
                MAX_ELEVATION_COLUMN, max_elevation, START_COLUMN, start, END_COLUMN, i, -1);

    return to_return;
}
