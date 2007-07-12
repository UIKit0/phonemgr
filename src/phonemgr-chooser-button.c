/*
 * (C) Copyright 2007 Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gnomebt-controller.h>
#include <gnomebt-chooser.h>

#include "phonemgr-chooser-button.h"

struct _PhonemgrChooserButton {
	GtkButton          parent;

	GtkWidget         *image;
	GnomebtController *btctl;
	char              *bdaddr;
};

enum {
	PROP_0,
	PROP_DEVICE,
};

static void	phonemgr_chooser_button_class_init	(PhonemgrChooserButtonClass * klass);
static void	phonemgr_chooser_button_init		(PhonemgrChooserButton      * button);

static GtkButtonClass *parent_class;

G_DEFINE_TYPE(PhonemgrChooserButton, phonemgr_chooser_button, GTK_TYPE_BUTTON);

static char *
get_current_btname (PhonemgrChooserButton *button, const char *bdaddr)
{
	char *name = NULL;
	if (bdaddr != NULL && bdaddr[0] != '\0')
		name = gnomebt_controller_get_device_preferred_name (button->btctl, bdaddr);

	return name;
}

static void
set_btdevname (PhonemgrChooserButton *button, const char *bdaddr)
{
	GtkWidget *w;
	char *c;

	if (bdaddr != NULL && bdaddr[0] != '\0') {
		c = get_current_btname (button, bdaddr);
		gtk_button_set_label (GTK_BUTTON (button), c ? c : bdaddr);
		g_free (c);
		if (button->bdaddr == NULL || strcmp (bdaddr, button->bdaddr) != 0) {
			g_free (button->bdaddr);
			button->bdaddr = g_strdup (bdaddr);
			gtk_image_set_from_icon_name (GTK_IMAGE (button->image), "phone", GTK_ICON_SIZE_MENU);
			g_object_notify (G_OBJECT (button), "device");
		}
	} else {
		gtk_button_set_label (GTK_BUTTON (button), _("Click to select device..."));
		if (button->bdaddr != NULL) {
			g_free (button->bdaddr);
			button->bdaddr = NULL;
			gtk_image_clear (GTK_IMAGE (button->image));
			g_object_notify (G_OBJECT (button), "device");
		}
	}
}

static void
choose_bdaddr (PhonemgrChooserButton *button, gpointer user_data)
{
	char *bdaddr = NULL;
	GnomebtChooser *btchooser;
	int result;

	//FIXME need to destroy the dialogue

	btchooser = gnomebt_chooser_new (button->btctl);
	result = gtk_dialog_run (GTK_DIALOG (btchooser));

	if (result == GTK_RESPONSE_OK)
		bdaddr = gnomebt_chooser_get_bdaddr (btchooser);

	gtk_widget_destroy (GTK_WIDGET (btchooser));

	set_btdevname (button, bdaddr);

	g_free (bdaddr);
}

static void
phonemgr_chooser_button_finalize (GObject *object)
{
	PhonemgrChooserButton *button = PHONEMGR_CHOOSER_BUTTON (object);

	if (button->btctl != NULL) {
		g_object_unref (button->btctl);
		button->btctl = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
phonemgr_chooser_button_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PhonemgrChooserButton *button;

	g_return_if_fail (PHONEMGR_IS_CHOOSER_BUTTON (object));
	button = PHONEMGR_CHOOSER_BUTTON (object);

	switch (property_id)
	case PROP_DEVICE: {
		set_btdevname (button, g_value_get_string (value));
		g_message ("bdaddr %s", g_value_get_string (value));
		//FIXME check for address
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
phonemgr_chooser_button_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	PhonemgrChooserButton *button;

	g_return_if_fail (PHONEMGR_IS_CHOOSER_BUTTON (object));
	button = PHONEMGR_CHOOSER_BUTTON (object);

	switch (property_id) {
	case PROP_DEVICE:
		g_value_set_string (value, button->bdaddr);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
phonemgr_chooser_button_class_init (PhonemgrChooserButtonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = phonemgr_chooser_button_finalize;
	object_class->set_property = phonemgr_chooser_button_set_property;
	object_class->get_property = phonemgr_chooser_button_get_property;

	g_object_class_install_property (object_class, PROP_DEVICE,
					 g_param_spec_string ("device", "Device", "The Bluetooth address of the selected device.",
							      NULL, G_PARAM_READWRITE));
}

static void
phonemgr_chooser_button_init (PhonemgrChooserButton *button)
{
	gtk_button_set_label (GTK_BUTTON (button), _("Click to select device..."));
	button->btctl = gnomebt_controller_new ();
	if (button->btctl == NULL)
		gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (choose_bdaddr), NULL);
	button->image = gtk_image_new ();
	gtk_button_set_image (GTK_BUTTON (button), button->image);
}

GtkWidget *
phonemgr_chooser_button_new (void)
{
	return g_object_new (PHONEMGR_TYPE_CHOOSER_BUTTON, NULL);
}

GtkWidget *
phonemgr_chooser_button_create (void)
{
	GtkWidget *widget;

	widget = phonemgr_chooser_button_new ();
	gtk_widget_show (widget);

	return widget;
}

gboolean
phonemgr_chooser_button_available (PhonemgrChooserButton *button)
{
	g_return_val_if_fail (PHONEMGR_IS_CHOOSER_BUTTON (button), FALSE);

	return button->btctl != NULL;
}

