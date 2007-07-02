

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gnome.h>

#include "app.h"

static GdkPixbuf *pb_idle, *pb_connecting, *pb_error, *pb_message, *pb_program;

static void
tray_icon_activate_cb (GtkStatusIcon *status_icon, MyApp *app)
{
	/* if not popping up messages automatically, popup a
	   message on button 1 click */
	if (app->messages && (! app->popup_cb)) {
		dequeue_message (app);
	}
}

static void
tray_icon_popup_menu_cb (GtkStatusIcon *status_icon,
			 guint          button,
			 guint          activate_time,
			 MyApp         *app)
{
	gtk_menu_popup (GTK_MENU (app->menu), NULL, NULL, NULL,
			NULL, button, activate_time);
}

static
GdkPixbuf *load_icon (MyApp *app, const gchar *iconname, int size)
{
	gchar *fname;
	GdkPixbuf *buf;
	GError *err=NULL;
	fname = gnome_program_locate_file (app->program,
					   GNOME_FILE_DOMAIN_APP_DATADIR,
					   iconname,
					   TRUE, NULL);

	if (fname == NULL)
		fname = gnome_program_locate_file (app->program,
						   GNOME_FILE_DOMAIN_DATADIR,
						   iconname,
						   TRUE, NULL);

	if (fname == NULL)
		fname = g_strdup_printf ("../ui/%s", iconname);

	buf = gdk_pixbuf_new_from_file_at_size (fname, size, size, &err);

	if (buf == NULL) {
		g_error ("Unable to load pixmap %s", iconname);
	}

	g_free (fname);

	return buf;
}

void
icon_init (MyApp *app)
{
	pb_idle = load_icon (app, "cellphone.png", 24);
	pb_connecting = load_icon (app, "cellphone-connecting.png", 24);
	pb_message = load_icon (app, "cellphone-message.png", 24);
	pb_error = load_icon (app, "cellphone-error.png", 24);
	pb_program = load_icon (app, "cellphone.png", 48);
}

static gboolean
flash_icon (MyApp *app)
{
	if (app->listener && phonemgr_listener_connected (app->listener)
	    && app->messages) {
		return TRUE;
	} else {
		/* disable flasher if we disconnect or have no messages left */
		return FALSE;
	}
}
void
enable_flasher (MyApp *app)
{
	gtk_status_icon_set_blinking (app->tray_icon, TRUE);
	//FIXME we shouldn't rely on a timeout
	app->flasher_cb = g_timeout_add (500, (GSourceFunc) flash_icon,
					 (gpointer) app);
}

void
set_icon_state (MyApp *app)
{
	if (app->listener && phonemgr_listener_connected (app->listener)) {
		gtk_widget_set_sensitive (app->send_item, TRUE);
		if (app->messages) {
			gtk_status_icon_set_from_pixbuf (app->tray_icon, pb_message);
			gtk_status_icon_set_tooltip (app->tray_icon, _("Message arrived"));
		} else {
			gtk_status_icon_set_from_pixbuf (app->tray_icon, pb_idle);
			gtk_status_icon_set_tooltip (app->tray_icon, _("Connected"));
		}
	} else if (app->connecting) {
		gtk_status_icon_set_from_pixbuf (app->tray_icon, pb_connecting);
		gtk_status_icon_set_tooltip (app->tray_icon, _("Connecting to phone"));
		gtk_widget_set_sensitive (app->send_item, FALSE);
	} else {
		gtk_status_icon_set_from_pixbuf (app->tray_icon, pb_error);
		gtk_status_icon_set_tooltip (app->tray_icon, _("Not connected"));
		gtk_widget_set_sensitive (app->send_item, FALSE);
	}
}

GdkPixbuf *
program_icon (void)
{
	return pb_program;
}

void
tray_icon_init (MyApp *app)
{
	app->tray_icon = gtk_status_icon_new ();
	set_icon_state (app);

	g_signal_connect (G_OBJECT (app->tray_icon), "activate",
			  G_CALLBACK (tray_icon_activate_cb), (gpointer) app);

	g_signal_connect (G_OBJECT (app->tray_icon), "popup-menu",
			  G_CALLBACK (tray_icon_popup_menu_cb), (gpointer) app);
}

void
tray_icon_hide (MyApp *app)
{
	gtk_status_icon_set_visible (app->tray_icon, FALSE);
}

