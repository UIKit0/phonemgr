#ifndef _APP_H
#define _APP_H

#include <gtk/gtk.h>
#include <gnome.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include "eggtrayicon.h"

#include "phonemgr-listener.h"

#include <gnomebt-controller.h>

typedef struct _appinfo {
	GnomeProgram	*program;
	GtkTooltips	*tooltip;
	EggTrayIcon	*tray_icon;
	GtkImage    *image_icon;
    GtkEventBox *event_box;
	GtkMenu *menu;
    GladeXML    *ui;
    GConfClient *client;
    GnomebtController   *btctl;
    PhonemgrListener    *listener;
    gchar   *devname;
    gboolean    reconnect;
    guint   pollsource;
} MyApp;

/* menu functions */
void construct_menu (MyApp *app);

/* ui functions */
void ui_init (MyApp *app);
void show_prefs_window (MyApp *app);

/* connection functions */
void free_connection (MyApp *app);
void initialise_connection (MyApp *app);
void reconnect_phone (MyApp *app);

/* icon functions */
void icon_init (MyApp *app);
void tray_icon_set (MyApp *app, gint which);
gboolean tray_destroy_cb (GtkObject *obj, MyApp *app);
#define tray_icon_init(x) tray_destroy_cb (NULL, x)
GdkPixbuf *program_icon (void);

enum {
    ICON_IDLE,
    ICON_CONNECTING,
    ICON_MESSAGE,
    ICON_ERROR
};

#define CONFBASE "/apps/gnome-phone-manager/prefs"

enum {
    CONNECTION_BLUETOOTH,
    CONNECTION_SERIAL1,
    CONNECTION_SERIAL2,
    CONNECTION_IRCOMM,
    CONNECTION_OTHER,
    CONNECTION_LAST
};


#endif
