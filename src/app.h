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

    /* gui stuff */
	GtkTooltips	*tooltip;
	EggTrayIcon	*tray_icon;
	GtkImage    *image_icon;
    GtkEventBox *event_box;
	GtkMenu *menu;
    GladeXML    *ui;
    gint    iconstate;
    gboolean showing_message;
    gint    chars;
    GtkWidget *send_item;

    /* auxilliary controllers */
    GConfClient *client;
    GnomebtController   *btctl;
    PhonemgrListener    *listener;

    /* connection state */
    gchar   *devname;
    gboolean    reconnect;

    /* messages */
    GMutex  *message_mutex;
    GList   *messages;

    /* thread stuff for connecting and disconnecting */
    GThread *connecting_thread;
    GThread *disconnecting_thread;
    GMutex *connecting_mutex;
    gboolean connecting;

    /* signal handlers and timeouts */
    gulong  status_cb;
    gulong  message_cb;
    gulong   pollsource;
    gulong   reconnector;
} MyApp;

typedef struct _message {
    gchar   *sender;
    GTime   timestamp;
    gchar   *message;
} Message;

/* menu functions */
void construct_menu (MyApp *app);

/* ui functions */
void ui_init (MyApp *app);
void show_prefs_window (MyApp *app);
gboolean dequeue_message (MyApp *app);
void create_send_dialog (MyApp *app, gchar *recip);

/* connection functions */
void free_connection (MyApp *app);
void disconnect_signal_handlers (MyApp *app);
void initialise_connection (MyApp *app);
void reconnect_phone (MyApp *app);

/* icon functions */
void icon_init (MyApp *app);
void set_icon_state (MyApp *app);
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
