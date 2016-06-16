#include "expat.h"
#include "structs.h"
#include <stdio.h>
#include <glib.h>
#include <stdlib.h>

typedef struct {
    int in_trkseg;
    int in_trkpt;
    char tag[10];
    char tmp_value[1000];
    SegListPtr list_ptr;
} ParserInfo;

int info_init(ParserInfo *info)
{
    info->in_trkseg = 0;
    info->in_trkpt = 0;
    g_stpcpy(info->tag, "");
    g_stpcpy(info->tmp_value, "");
    info->list_ptr = new_list();
    if(info->list_ptr == NULL)
        return -1;
    return 0;
}

static void XMLCALL startElement(void *userData, const char *name, const char **atts)
{
    ParserInfo *info = (ParserInfo *)userData;

    if(info->in_trkseg) {
        if(info->in_trkpt) {
            g_stpcpy(info->tag, name);
        }
        else if(g_strcmp0(name, "trkpt") == 0) {
            info->in_trkpt = 1;
            add_point(info->list_ptr);  // TODO ERROR HANDLE
            double lat = 0, lon = 0;
            for (int i = 0; atts[i]; i += 2) {
                if(g_strcmp0(atts[i], "lat") == 0) {
                    lat = g_ascii_strtod (atts[i+1], NULL);
                }
                else if(g_strcmp0(atts[i], "lon") == 0) {
                    lon = g_ascii_strtod (atts[i+1], NULL);
                }
            }
            set_coordinates(info->list_ptr, lat, lon);
        }
    }
    else if(g_strcmp0(name, "trkseg") == 0){
        info->in_trkseg = 1;
    }
}

static void XMLCALL endElement(void *userData, const char *name)
{
    ParserInfo *info = (ParserInfo *)userData;
    if(g_strcmp0(name, "trkpt") == 0) {
        info->in_trkpt = 0;
        g_stpcpy(info->tag, "");
    }
    else if(g_strcmp0(name, "trkseg") == 0) {
        info->in_trkseg = 0;
        //new_point(info->list_ptr); // TODO ERROR HANDLE
    }
    else if(g_strcmp0(info->tag, "ele") == 0) {
        set_elevation(info->list_ptr, g_ascii_strtod(info->tmp_value, NULL));
        g_stpcpy(info->tmp_value, "");
    }
    else if(g_strcmp0(info->tag, "time") == 0) {
        set_time(info->list_ptr, info->tmp_value);
        g_stpcpy(info->tmp_value, "");
    }
    g_stpcpy(info->tag, "");
}

static void XMLCALL stringElement(void *userData, const char *txt, int len)
{
    ParserInfo *info = (ParserInfo *)userData;
    if(g_strcmp0(info->tag, "ele") == 0 || g_strcmp0(info->tag, "time") == 0) {
        //fwrite(txt, len, sizeof(char), stdout);
        g_strlcat(info->tmp_value, txt, len+1);
    }
}

// Funkcja parsuje podany plik i zwraca wskaźnik do listy segmentów lub NULL, gdy wystąpił błąd
SegListPtr parse(GIOChannel *xml_file)
{
    char buf[BUFSIZ];
    XML_Parser parser = XML_ParserCreate(NULL);
    int done;

    ParserInfo *info = (ParserInfo *)malloc(sizeof(ParserInfo));
    if(info_init(info) == -1)
        return NULL;

    XML_SetUserData(parser, info);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, stringElement);
    do {
        unsigned len;
        g_io_channel_read_chars(xml_file, buf, sizeof(buf), &len, NULL);
        done = len < sizeof(buf);
        if(XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR) {
            if(info->list_ptr != NULL)
                free_list(info->list_ptr);
            free(info);
            return NULL;
        }
    } while(!done);
    XML_ParserFree(parser);
    SegListPtr to_return = info->list_ptr;
    free(info);
    return to_return;
}
