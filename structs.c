#include <stdio.h>
#include <stdlib.h>
#include "structs.h"
#include <glib.h>

struct point {
    double latitude, longitude;
    double elevation;
    double x, y;
    GDateTime *time;
};

struct segment {
    PointPtr start_point, end_point;
    SegmentPtr next_segment;
    double distance;
    double dtime;
    double speed;
};

struct seglist {
    SegmentPtr first, last;
    double distance, time;
    double max_elevation, min_elevation;
    double longitude_min, longitude_max;
    double latitude_min, latitude_max;
    double max_speed, min_speed;
    int size;
};

// Tworzenie nowej listy
// zwraca wskaznik do nowej listy lub NULL jesli wystapil blad
SegListPtr new_list(void)
{
    SegListPtr list_ptr = (SegListPtr)malloc(sizeof(struct seglist));

    if(list_ptr == NULL)
        return NULL;

    list_ptr->first = list_ptr->last = NULL;
    list_ptr->longitude_min = 181;
    list_ptr->longitude_max = -181;
    list_ptr->max_elevation = DBL_MIN;
    list_ptr->min_elevation =  DBL_MAX;
    list_ptr->distance = list_ptr->time = list_ptr->size = 0;
    list_ptr->max_speed = DBL_MIN;
    list_ptr->min_speed = DBL_MAX;
    return list_ptr;
}

// dodaje punkt do listy
// zwraca 0 jesli wszystko ok, -1 jesli wystapil blad
int add_point(SegListPtr list_ptr)
{
    if(list_ptr->first == NULL) {
        list_ptr->first = list_ptr->last = (SegmentPtr)malloc(sizeof(struct segment));
        if(list_ptr->first == NULL) {
            free_list(list_ptr);
            return -1;
        }

        list_ptr->first->start_point = (PointPtr)malloc(sizeof(struct point));
        if(list_ptr->first->start_point == NULL) {
            free_list(list_ptr);
            return -1;
        }

        list_ptr->first->end_point = list_ptr->first->start_point;
        list_ptr->first->next_segment = NULL;
    }
    else if(list_ptr->first->start_point == list_ptr->first->end_point) {
        list_ptr->first->end_point = (PointPtr)malloc(sizeof(struct point));
        if(list_ptr->first->end_point == NULL) {
            free_list(list_ptr);
            return -1;
        }
        list_ptr->size++;
    }
    else {
        SegmentPtr tmp_ptr = (SegmentPtr)malloc(sizeof(struct segment));
        if(tmp_ptr == NULL) {
            free_list(list_ptr);
            return -1;
        }

        tmp_ptr->start_point = list_ptr->last->end_point;
        tmp_ptr->end_point = (PointPtr)malloc(sizeof(struct point));
        if(tmp_ptr->end_point == NULL) {
            free_segment(tmp_ptr);
            free_list(list_ptr);
            return -1;
        }
        tmp_ptr->next_segment = NULL;

        list_ptr->last->next_segment = tmp_ptr;
        list_ptr->last = tmp_ptr;
        list_ptr->size++;
    }

    return 0;
}

void set_distance(SegmentPtr seg_ptr, double distance)
{
    seg_ptr->distance = distance;
}

void set_speed(SegmentPtr seg_ptr, double speed)
{
    seg_ptr->speed = speed;
}

double get_maxspeed(SegListPtr list_ptr)
{
    return list_ptr->max_speed;
}

void set_maxspeed(SegListPtr list_ptr, double speed)
{
    list_ptr->max_speed = speed;
}

double get_minspeed(SegListPtr list_ptr)
{
    return list_ptr->min_speed;
}

void set_minspeed(SegListPtr list_ptr, double speed)
{
    list_ptr->min_speed = speed;
}

void set_coordinates(SegListPtr list_ptr, double lat, double lon)
{
    if(lat > list_ptr->latitude_max)
        list_ptr->latitude_max = lat;
    else if(lon < list_ptr->latitude_min)
        list_ptr->latitude_min = lat;
    list_ptr->last->end_point->latitude = lat;
    if(lon > list_ptr->longitude_max)
        list_ptr->longitude_max = lon;
    else if(lon < list_ptr->longitude_min)
        list_ptr->longitude_min = lon;
    list_ptr->last->end_point->longitude = lon;
}

void set_elevation(SegListPtr list_ptr, double ele)
{
    list_ptr->last->end_point->elevation = ele;
    if(ele > list_ptr->max_elevation)
        list_ptr->max_elevation = ele;
    if(ele < list_ptr->min_elevation)
        list_ptr->min_elevation = ele;
}

double get_minelevation(SegListPtr list_ptr)
{
    return list_ptr->min_elevation;
}

double get_maxelevation(SegListPtr list_ptr)
{
    return list_ptr->max_elevation;
}

void set_time(SegListPtr list_ptr, const char *time)
{
    gint year, month, day, hour, minute;
    gdouble seconds;
    sscanf(time, "%d-%d-%dT%d:%d:%lfZ", &year, &month, &day, &hour, &minute, &seconds);

    list_ptr->last->end_point->time = g_date_time_new_utc(year, month, day, hour, minute, seconds);
}

SegmentPtr get_first_segment(SegListPtr list_ptr)
{
    return list_ptr->first;
}

SegmentPtr get_next_segment(SegmentPtr seg_ptr)
{
    return seg_ptr->next_segment;
}

double get_minlongitude(SegListPtr list_ptr)
{
    return list_ptr->longitude_min;
}

double get_maxlongitude(SegListPtr list_ptr)
{
    return list_ptr->longitude_max;
}

double get_minlatitude(SegListPtr list_ptr)
{
    return list_ptr->latitude_min;
}

double get_maxlatitude(SegListPtr list_ptr)
{
    return list_ptr->latitude_max;
}

double get_distance(SegmentPtr seg_ptr)
{
    return seg_ptr->distance;
}

void set_dtime(SegmentPtr seg_ptr, double dtime)
{
    seg_ptr->dtime = dtime;
}

double get_speed(SegmentPtr seg_ptr)
{
    return seg_ptr->speed;
}

double get_dtime(SegmentPtr seg_ptr)
{
    return seg_ptr->dtime;
}

PointPtr get_start_point(SegmentPtr seg_ptr)
{
    return seg_ptr->start_point;
}

PointPtr get_end_point(SegmentPtr seg_ptr)
{
    return seg_ptr->end_point;
}

double get_latitude(PointPtr point_ptr)
{
    return point_ptr->latitude;
}

double get_longitude(PointPtr point_ptr)
{
    return point_ptr->longitude;
}

double get_elevation(PointPtr point_ptr)
{
    return point_ptr->elevation;
}

GDateTime *get_time(PointPtr point_ptr)
{
    return point_ptr->time;
}

double get_x(PointPtr point_ptr)
{
    return point_ptr->x;
}

double get_y(PointPtr point_ptr)
{
    return point_ptr->y;
}

int get_size(SegListPtr list_ptr)
{
    return list_ptr->size;
}

void set_xy(PointPtr point_ptr, double x, double y)
{
    point_ptr->x = x;
    point_ptr->y = y;
}

double get_track_distance(SegListPtr list_ptr)
{
    return list_ptr->distance;
}

double get_track_time(SegListPtr list_ptr)
{
    return list_ptr->time;
}

void add_track_distance(SegListPtr list_ptr, double distance)
{
    list_ptr->distance += distance;
}

void add_track_time(SegListPtr list_ptr, double time)
{
    list_ptr->time += time;
}

void free_segment(SegmentPtr seg_ptr)
{
    if(get_end_point(seg_ptr) != NULL)
        free(get_end_point(seg_ptr));
    free(seg_ptr);
}

void free_list(SegListPtr list_ptr)
{
    SegmentPtr seg_to_delete = get_first_segment(list_ptr);
    SegmentPtr tmp;
    if(seg_to_delete != NULL) {
        if(get_start_point(seg_to_delete) != NULL) {
            free(get_start_point(seg_to_delete));
        }
    }
    while(seg_to_delete != NULL)
    {
        tmp = get_next_segment(seg_to_delete);
        free_segment(seg_to_delete);
        seg_to_delete = tmp;
    }

}
