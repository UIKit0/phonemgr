

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gnome.h>

#include "app.h"

static GdkPixbuf *pb_idle, *pb_connecting, *pb_error, *pb_message, *pb_program;

static gboolean
tray_icon_press (GtkWidget *widget, GdkEventButton *event, MyApp *app)
{
	if (event->button == 3)
	{
		gtk_menu_popup (GTK_MENU (app->menu), NULL, NULL, NULL,
						NULL, event->button, event->time);
		return TRUE;
	}
	return FALSE;
}

static gboolean
tray_icon_release (GtkWidget *widget, GdkEventButton *event, MyApp *app)
{
	if (event->button == 3) {
		gtk_menu_popdown (GTK_MENU (app->menu));
		return FALSE;
	}

	return TRUE;
}

gboolean
tray_destroy_cb (GtkObject *obj, MyApp *app)
{
	/* When try icon is destroyed, recreate it.  This happens
	   when the notification area is removed. */

	app->tray_icon = egg_tray_icon_new (_("Phone Manager"));

	app->event_box = GTK_EVENT_BOX (gtk_event_box_new ());
	app->image_icon = GTK_IMAGE (gtk_image_new ());

	tray_icon_set (app, app->iconstate, NULL);

	gtk_container_add (GTK_CONTAINER (app->event_box),
			GTK_WIDGET (app->image_icon));
	gtk_container_add (GTK_CONTAINER (app->tray_icon),
			GTK_WIDGET (app->event_box));

	g_signal_connect (G_OBJECT (app->tray_icon), "destroy",
		G_CALLBACK (tray_destroy_cb), (gpointer) app);

	g_signal_connect (GTK_OBJECT (app->event_box), "button_press_event",
		G_CALLBACK (tray_icon_press), (gpointer) app);

	g_signal_connect (GTK_OBJECT (app->event_box), "button_release_event",
		G_CALLBACK (tray_icon_release), (gpointer) app);


	gtk_widget_show_all (GTK_WIDGET (app->tray_icon));

	return TRUE;
}

static
GdkPixbuf *load_icon (MyApp *app, const gchar *iconname, int size)
{
	gchar *fname;
	GdkPixbuf *buf;
	GError *err=NULL;

	fname = gnome_program_locate_file (app->program,
				GNOME_FILE_DOMAIN_APP_PIXMAP,
				iconname,
				TRUE, NULL);

	if (fname == NULL)
		fname = g_strdup_printf ("../ui/pixmaps/%s", iconname);

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

void
tray_icon_set (MyApp *app, gint which, gchar *tooltip)
{
	app->iconstate = which;
	switch (which) {
		case ICON_ERROR:
			gtk_image_set_from_pixbuf (app->image_icon, pb_error);
			break;
		case ICON_CONNECTING:
			gtk_image_set_from_pixbuf (app->image_icon, pb_connecting);
			break;
		case ICON_MESSAGE:
			gtk_image_set_from_pixbuf (app->image_icon, pb_message);
			break;
		case ICON_IDLE:
			gtk_image_set_from_pixbuf (app->image_icon, pb_idle);
			break;
		default:
			g_warning ("Unknown icon state %d", which);
	}
	if (tooltip) {
		gtk_tooltips_set_tip (app->tooltip,
				GTK_WIDGET (app->event_box),
				tooltip, NULL);
	}
}

GdkPixbuf *
program_icon ()
{
	return pb_program;
}
