#ifndef _STRUCTS_H_
#define _STRUCTS_H_
#include <glib.h>

typedef struct {
    double min_x, max_x;
    double min_y, max_y;
} xy_scale;

typedef struct point *PointPtr;
typedef struct segment *SegmentPtr;
typedef struct seglist *SegListPtr;

// obsługa punktów
PointPtr get_start_point(SegmentPtr seg_ptr);
PointPtr get_end_point(SegmentPtr seg_ptr);

double get_latitude(PointPtr point_ptr);
double get_longitude(PointPtr point_ptr);
double get_elevation(PointPtr point_ptr);
GDateTime *get_time(PointPtr point_ptr);
double get_x(PointPtr point_ptr);
double get_y(PointPtr point_ptr);

void set_xy(PointPtr point_ptr, double x, double y);

// obsługa segmentów
SegmentPtr get_next_segment(SegmentPtr seg_ptr);
double get_distance(SegmentPtr seg_ptr);
double get_speed(SegmentPtr seg_ptr);
double get_dtime(SegmentPtr seg_ptr);

void set_distance(SegmentPtr seg_ptr, double distance);
void set_dtime(SegmentPtr seg_ptr, double dtime);
void set_speed(SegmentPtr seg_ptr, double speed);


// obsługa listy segmentów
SegListPtr new_list(void);
int add_point(SegListPtr list_ptr);
SegmentPtr get_first_segment(SegListPtr list_ptr);

void free_segment(SegmentPtr seg_ptr);
void free_list(SegListPtr list_ptr);

double get_minlongitude(SegListPtr list_ptr);
double get_maxlongitude(SegListPtr list_ptr);
double get_minlatitude(SegListPtr list_ptr);
double get_maxlatitude(SegListPtr list_ptr);
double get_minelevation(SegListPtr list_ptr);
double get_maxelevation(SegListPtr list_ptr);
double get_minspeed(SegListPtr list_ptr);
double get_maxspeed(SegListPtr list_ptr);
double get_track_distance(SegListPtr list_ptr);
double get_track_time(SegListPtr list_ptr);
int get_size(SegListPtr list_ptr);

void add_track_distance(SegListPtr list_ptr, double distance);
void add_track_time(SegListPtr list_ptr, double time);
void set_elevation(SegListPtr list_ptr, double ele);
void set_time(SegListPtr list_ptr, const char *time);
void set_coordinates(SegListPtr list_ptr, double lat, double lon);
void set_maxspeed(SegListPtr list_ptr, double speed);
void set_minspeed(SegListPtr list_ptr, double speed);


#endif // _STRUCTS_H_
