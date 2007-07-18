#ifndef _APP_H
#define _APP_H

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include <gst/gst.h>
#include <gnomebt-controller.h>

#include "phonemgr-listener.h"
#include "phonemgr-object.h"

typedef struct _appinfo {
	/* gui stuff */
	GtkStatusIcon *tray_icon;
	GtkMenu *menu;
	GladeXML    *ui;
	int    iconstate;
	int    chars;
	GtkWidget *send_item;
	gboolean flashon;

	/* auxilliary controllers */
	GConfClient *client;
	PhonemgrListener    *listener;
	PhonemgrObject      *object;
	GstElement *playbin;

	/* connection state */
	char   *devname;
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
	gulong  reconnector;
	gulong  popup_cb;
	gulong  battery_cb;
} MyApp;

typedef struct _message {
	char   *sender;
	time_t   timestamp;
	char   *message;
} Message;

/* menu functions */
void construct_menu (MyApp *app);

/* ui functions */
void ui_init (MyApp *app);
void show_prefs_window (MyApp *app);
gboolean dequeue_message (MyApp *app);
void create_send_dialog (MyApp *app, GtkDialog *parent, const char *recip);
void enable_flasher (MyApp *app);
void play_alert (MyApp *app);
void ui_hide (MyApp *app);

/* connection functions */
void free_connection (MyApp *app);
void disconnect_signal_handlers (MyApp *app);
void initialise_connection (MyApp *app);
void reconnect_phone (MyApp *app);

/* icon functions */
void set_icon_state (MyApp *app);
void tray_icon_init (MyApp *app);
void tray_icon_hide (MyApp *app);

enum {
    ICON_IDLE,
    ICON_CONNECTING,
    ICON_MESSAGE,
    ICON_ERROR
};

#define CONFBASE "/apps/gnome-phone-manager"

enum {
	CONNECTION_BLUETOOTH,
	CONNECTION_SERIAL1,
	CONNECTION_SERIAL2,
	CONNECTION_IRCOMM,
	CONNECTION_OTHER,
	CONNECTION_LAST
};


#endif
