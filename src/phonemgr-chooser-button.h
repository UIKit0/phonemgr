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

#ifndef __PHONEMGR_CHOOSER_BUTTON_H__
#define __PHONEMGR_CHOOSER_BUTTON_H__

#include <gtk/gtkbutton.h>

G_BEGIN_DECLS

#define PHONEMGR_TYPE_CHOOSER_BUTTON     (phonemgr_chooser_button_get_type ())
#define PHONEMGR_CHOOSER_BUTTON(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHONEMGR_TYPE_CHOOSER_BUTTON, PhonemgrChooserButton))
#define PHONEMGR_IS_CHOOSER_BUTTON(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHONEMGR_TYPE_CHOOSER_BUTTON))

typedef struct _PhonemgrChooserButton PhonemgrChooserButton;

typedef struct _PhonemgrChooserButtonClass {
  GtkButtonClass parent_class;

  gpointer __bla[4];
} PhonemgrChooserButtonClass;

GType		phonemgr_chooser_button_get_type	(void);

GtkWidget *	phonemgr_chooser_button_new		(void);
GtkWidget *	phonemgr_chooser_button_create		(void);
gboolean	phonemgr_chooser_button_available	(PhonemgrChooserButton *button);

G_END_DECLS

#endif /* __PHONEMGR_CHOOSER_BUTTON_H__ */
