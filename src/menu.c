

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gnome.h>

#include "app.h"

static gboolean
prefs_activated (GtkMenuItem *item, MyApp *app)
{
	show_prefs_window (app);
	return TRUE;
}

static gboolean
about_activated(GtkMenuItem *item, gpointer data)
{
    static gpointer about = NULL;
    const gchar *authors[] = { "Edd Dumbill <edd@usefulinc.com>", NULL };
    const gchar *documenters[] = { NULL };
    const gchar *translator_credits = _("translator_credits");

    if (about != NULL) {
		gdk_window_raise (GTK_WIDGET(about)->window);
		gdk_window_show (GTK_WIDGET(about)->window);
		return TRUE;
    }

    about = (gpointer)gnome_about_new(_("Phone Manager"), VERSION,
					"Copyright \xc2\xa9 2003-4 Edd Dumbill",
					_("Send and receive messages from your mobile phone."),
					(const char **)authors,
					(const char **)documenters,
					strcmp (translator_credits,
							"translator_credits") != 0 ?
					translator_credits : NULL,
					program_icon());

    g_signal_connect (G_OBJECT (about), "destroy",
				G_CALLBACK (gtk_widget_destroyed), &about);

    g_object_add_weak_pointer (G_OBJECT (about), &about);

    gtk_widget_show (GTK_WIDGET(about));

    return TRUE;
}

static gboolean
quit_activated (GtkMenuItem *item, gpointer data)
{
	MyApp *app = (MyApp *)data;
	disconnect_signal_handlers (app);
	gtk_main_quit ();
    return TRUE;
}

static gboolean
send_activated (GtkMenuItem *item, gpointer data)
{
	MyApp *app = (MyApp *)data;
	GtkWidget *w;

	gtk_widget_set_sensitive (GTK_WIDGET (app->send_item), FALSE);
	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "send_dialog"));
	gtk_widget_show_all (w);
	return TRUE;
}

void
construct_menu (MyApp *app)
{
	GtkWidget *item;

	app->menu = GTK_MENU (gtk_menu_new ());

	item = gtk_image_menu_item_new_with_mnemonic (_("_Send Message"));
	g_signal_connect (G_OBJECT(item), "activate",
			G_CALLBACK (send_activated), (gpointer) app);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
			GTK_WIDGET (gtk_image_new_from_stock ("gnome-stock-mail",
					GTK_ICON_SIZE_MENU)));
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (app->menu), item);
	app->send_item = GTK_WIDGET (item);

	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES,
			 NULL);
    g_signal_connect (G_OBJECT(item), "activate",
			G_CALLBACK (prefs_activated), (gpointer) app);

    gtk_widget_show (item);
    gtk_menu_shell_append(GTK_MENU_SHELL(app->menu), item);

	item = gtk_image_menu_item_new_from_stock (GNOME_STOCK_ABOUT,
			 NULL);

    g_signal_connect (G_OBJECT(item), "activate",
			G_CALLBACK (about_activated), (gpointer) app);

    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL(app->menu), item);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT,
                                             NULL);
    g_signal_connect (G_OBJECT(item), "activate",
			G_CALLBACK (quit_activated), (gpointer) app);

    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL(app->menu), item);
}

