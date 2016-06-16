#include <cairo.h>
#include <gtk/gtk.h>
#include "gtk_gui.h"
#include "structs.h"
#include "analyzer.h"
#include "xml.h"
#include <stdlib.h>

static SegListPtr list_ptr;
static xy_scale *transform_scale;
static guint start_select, end_select;
static GObject *map_area, *chart_area, *treeview, *spin_button_change, *spin_button_time,
    *combo_box, *window, *stats, *label_chart;

static char *time_to_string(double time)
{
    char *time_string = (char*)calloc(sizeof(char), 20);
    int hours = (int)time;
    int minutes = (int)((time - (double)hours)*60);
    int seconds = (int)(((time - (double)hours)*3600)-(double)minutes*60);
    if(hours)
        sprintf(time_string, "%d h %d min %d s", hours, minutes, seconds);
    else
        sprintf(time_string, "%d min %d s", minutes, seconds);
    return time_string;
}

static void time_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    double time;
    gtk_tree_model_get(tree_model, iter, TIME_COLUMN, &time, -1);
    char *str_time = time_to_string(time);
    g_object_set(cell, "text", str_time, NULL);
    g_free(str_time);
}

static void distance_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    gdouble distance;
    gchar buf[20];

    gtk_tree_model_get(tree_model, iter, DISTANCE_COLUMN, &distance, -1);
    g_snprintf(buf, sizeof(buf), "%.2lf km", distance);
    g_object_set(cell, "text", buf, NULL);
}

static void speed_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    gdouble speed;
    gchar buf[20];

    gtk_tree_model_get(tree_model, iter, SPEED_COLUMN, &speed, -1);
    g_snprintf(buf, sizeof(buf), "%.2lf km/h", speed);
    g_object_set(cell, "text", buf, NULL);
}

static void min_elevation_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    gdouble elevation;
    gchar buf[20];

    gtk_tree_model_get(tree_model, iter, MIN_ELEVATION_COLUMN, &elevation, -1);
    g_snprintf(buf, sizeof(buf), "%.2lf m", elevation);
    g_object_set(cell, "text", buf, NULL);
}

static void max_elevation_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    gdouble elevation;
    gchar buf[20];

    gtk_tree_model_get(tree_model, iter, MAX_ELEVATION_COLUMN, &elevation, -1);
    g_snprintf(buf, sizeof(buf), "%.2lf m", elevation);
    g_object_set(cell, "text", buf, NULL);
}

static double transform_x(double x)
{
    return (x-transform_scale->min_x)/(transform_scale->max_x-transform_scale->min_x);
}

static double transform_y(double y)
{
    return 1.0-(y-transform_scale->min_y)/(transform_scale->max_y-transform_scale->min_y);
}

static void draw_map(GtkWidget *widget, cairo_t *cr)
{
    cairo_scale (cr, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
    SegmentPtr tmp_seg = get_first_segment(list_ptr);
    PointPtr tmp_point = get_start_point(tmp_seg);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 0.003);
    cairo_move_to(cr, transform_x(get_x(tmp_point)), transform_y(get_y(tmp_point)));
    guint i = 0;
    while(tmp_seg != NULL) {
        if(i == start_select) {
            cairo_set_source_rgb(cr, 1, 0, 0);
        }
        tmp_point = get_end_point(tmp_seg);
        double x = transform_x(get_x(tmp_point));
        double y = transform_y(get_y(tmp_point));

        cairo_line_to(cr, x, y);
        tmp_seg = get_next_segment(tmp_seg);
        cairo_stroke(cr);
        if(i == end_select)
            cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_move_to(cr, x, y);
        i++;
    }
}

static void draw_chart(GtkWidget *widget, cairo_t *cr)
{
    double max_speed = get_maxspeed(list_ptr);
    double min_speed = get_minspeed(list_ptr);
    double track_time = get_track_time(list_ptr);
    double time = 0;

    cairo_scale (cr, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 0.003);
    cairo_move_to(cr, 0, 0);

    SegmentPtr tmp_seg = get_first_segment(list_ptr);
    guint i = 0;
    while(tmp_seg != NULL) {
        if(i == start_select)
            cairo_set_source_rgb(cr, 1, 0, 0);
        double y = 1.0 - (get_speed(tmp_seg)-min_speed)/(max_speed-min_speed);
        time += get_dtime(tmp_seg);
        double x = time/track_time;
        cairo_line_to(cr, x, y);
        cairo_stroke(cr);

        tmp_seg = get_next_segment(tmp_seg);
        if(i == end_select)
            cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_move_to(cr, x, y);
        i++;
    }
}

static gboolean on_draw_map(GtkWidget *widget, cairo_t *cr,
    gpointer user_data)
{
    if(list_ptr != NULL)
        draw_map(widget, cr);

  return FALSE;
}

static gboolean on_draw_chart(GtkWidget *widget, cairo_t *cr,
    gpointer user_data)
{
    if(list_ptr != NULL)
        draw_chart(widget, cr);

  return FALSE;
}

static void tree_selection_changed_cb(GtkTreeSelection *selection, gpointer data)
{
        GtkTreeIter iter;
        GtkTreeModel *model;

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
                gtk_tree_model_get(model, &iter, START_COLUMN, &start_select, END_COLUMN, &end_select, -1);
                gtk_widget_queue_draw (GTK_WIDGET(map_area));
                gtk_widget_queue_draw (GTK_WIDGET(chart_area));
        }
}

static void button_clicked(GtkButton *button, gpointer user_data)
{
    if(list_ptr != NULL) {
        double parameter_change = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_change));
        double parameter_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_time));
        gchar *active_type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_box));

        GtkTreeModel *to_clear = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
        gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), NULL);
        if(to_clear != NULL)
            gtk_list_store_clear(GTK_LIST_STORE(to_clear));

        if(g_strcmp0(active_type, "Zakręty") == 0) {
            gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(divide_by_turns(list_ptr,
                parameter_change, parameter_time)));
            gtk_widget_queue_draw (GTK_WIDGET(map_area));
            gtk_widget_queue_draw (GTK_WIDGET(chart_area));
        }
        else if(g_strcmp0(active_type, "Prędkość") == 0) {
            gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(divide_by_speed(list_ptr,
            parameter_change, parameter_time)));
            gtk_widget_queue_draw (GTK_WIDGET(map_area));
            gtk_widget_queue_draw (GTK_WIDGET(chart_area));
        }
        else {
            gtk_widget_queue_draw (GTK_WIDGET(map_area));
            gtk_widget_queue_draw (GTK_WIDGET(chart_area));
        }

        g_free(active_type);
    }
}

static void error_dialog(const char* filename)
{
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    GtkMessageDialog *dialog_error = GTK_MESSAGE_DIALOG(gtk_message_dialog_new (GTK_WINDOW(window),
                                            flags,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE,
                                            "Bład odczytu pliku “%s”", filename));
    gtk_dialog_run (GTK_DIALOG (dialog_error));
    gtk_widget_destroy (GTK_WIDGET(dialog_error));
}

static void select_file(GtkMenuItem *menuitem, gpointer user_data)
{

    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new ("Open File",
                                          GTK_WINDOW(window),
                                          action,
                                          "Cancel",
                                          GTK_RESPONSE_CANCEL,
                                          "Open",
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);

    res = gtk_dialog_run (GTK_DIALOG (dialog));

    if (res == GTK_RESPONSE_ACCEPT)
    {
        GtkTreeModel *to_clear = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
        gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), NULL);
        if(to_clear != NULL)
            gtk_list_store_clear(GTK_LIST_STORE(to_clear));

        start_select = end_select = -1;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
        char *filename = gtk_file_chooser_get_filename (chooser);

        GError *file_error = NULL;
        GIOChannel *file = g_io_channel_new_file(filename, "r", &file_error);

        if(file_error == NULL) {
            if(list_ptr != NULL) {
                free_list(list_ptr);
                g_free(transform_scale);
            }
            list_ptr = parse(file);

            if(list_ptr == NULL) {
                error_dialog(filename);
            }
            else {
                analyze_segments(list_ptr);
                transform_scale = analyze_points(list_ptr);

                char buffer[1000];
                char *str_time = time_to_string(get_track_time(list_ptr));
                sprintf(buffer, "Odległość: %.2lf km\nCzas: %s\nŚrednia prędkość: %.1lf km/h\nMinimalna wysokość: %.0lf m\nMaksymalna wysokość: %.0lf m",
                    get_track_distance(list_ptr), str_time, get_track_distance(list_ptr)/get_track_time(list_ptr),
                    get_minelevation(list_ptr), get_maxelevation(list_ptr));
                gtk_label_set_text(GTK_LABEL(stats), buffer);

                sprintf(buffer, "Wykres prędkości od czasu: Vmax = %.2lf km/h | Vmin = %.2f km/h",
                    get_maxspeed(list_ptr), get_minspeed(list_ptr));
                gtk_label_set_text(GTK_LABEL(label_chart), buffer);
                g_free(str_time);
            }
        }
        else
            error_dialog(filename);
        g_free (filename);
    }
    gtk_widget_destroy (dialog);
}

void gtk_gui(int argc, char *argv[])
{
    GtkBuilder *builder;
    GObject *quit_menu_item, *treeview_selection, *button, *open_menu_item;
    GObject *time_column, *time_renderer, *distance_column, *distance_renderer, *speed_column, *speed_renderer;
    GObject *max_elevation_column, *max_elevation_render, *min_elevation_render, *min_elevation_column;
    list_ptr = NULL;

    gtk_init(&argc, &argv);
    builder = gtk_builder_new_from_file("ui.glade");

    window = gtk_builder_get_object (builder, "window");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    // MENU
    quit_menu_item = gtk_builder_get_object (builder, "quit_menu_item");
    g_signal_connect (quit_menu_item, "activate", G_CALLBACK (gtk_main_quit), NULL);

    open_menu_item = gtk_builder_get_object(builder, "open_menu_item");
    g_signal_connect(open_menu_item, "activate", G_CALLBACK(select_file), NULL);

    // Powierzchnie do rysowania
    map_area = gtk_builder_get_object(builder, "drawing_area_map");
    g_signal_connect(map_area, "draw", G_CALLBACK(on_draw_map), NULL);

    chart_area = gtk_builder_get_object(builder, "drawing_area_chart");
    g_signal_connect(chart_area, "draw", G_CALLBACK(on_draw_chart), NULL);

    label_chart = gtk_builder_get_object(builder, "label_chart");

    // TreeView
    treeview = gtk_builder_get_object(builder, "treeview");

    treeview_selection = gtk_builder_get_object(builder, "treeview_selection");
    g_signal_connect (treeview_selection, "changed", G_CALLBACK (tree_selection_changed_cb), NULL);

    time_column = gtk_builder_get_object(builder, "time_column");
    time_renderer = gtk_builder_get_object(builder, "time_renderer");
    gtk_tree_view_column_set_cell_data_func(GTK_TREE_VIEW_COLUMN(time_column),
        GTK_CELL_RENDERER(time_renderer), time_cell_data_func, NULL, NULL);

    distance_column = gtk_builder_get_object(builder, "distance_column");
    distance_renderer = gtk_builder_get_object(builder, "distance_renderer");
    gtk_tree_view_column_set_cell_data_func(GTK_TREE_VIEW_COLUMN(distance_column),
        GTK_CELL_RENDERER(distance_renderer), distance_cell_data_func, NULL, NULL);

    speed_column = gtk_builder_get_object(builder, "speed_column");
    speed_renderer = gtk_builder_get_object(builder, "speed_renderer");
    gtk_tree_view_column_set_cell_data_func(GTK_TREE_VIEW_COLUMN(speed_column),
        GTK_CELL_RENDERER(speed_renderer), speed_cell_data_func, NULL, NULL);

    min_elevation_column = gtk_builder_get_object(builder, "min_elevation_column");
    min_elevation_render = gtk_builder_get_object(builder, "min_elevation_render");
    gtk_tree_view_column_set_cell_data_func(GTK_TREE_VIEW_COLUMN(min_elevation_column),
        GTK_CELL_RENDERER(min_elevation_render), min_elevation_cell_data_func, NULL, NULL);

    max_elevation_column = gtk_builder_get_object(builder, "max_elevation_column");
    max_elevation_render = gtk_builder_get_object(builder, "max_elevation_render");
    gtk_tree_view_column_set_cell_data_func(GTK_TREE_VIEW_COLUMN(max_elevation_column),
        GTK_CELL_RENDERER(max_elevation_render), max_elevation_cell_data_func, NULL, NULL);

    // Statystyki
    stats = gtk_builder_get_object(builder, "stats_label");

    // Podzial sciezki
    spin_button_change = gtk_builder_get_object(builder, "spin_button_change");
    spin_button_time = gtk_builder_get_object(builder, "spin_button_time");
    combo_box = gtk_builder_get_object(builder, "combo_box");

    button = gtk_builder_get_object(builder, "button");
    g_signal_connect(button, "clicked", G_CALLBACK(button_clicked), NULL);

    gtk_main();
}
