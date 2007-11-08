#ifndef __PHONEY_IM_CHANNEL_H__
#define __PHONEY_IM_CHANNEL_H__
/*
 * im-channel.h - PhoneyImChannel header
 * Copyright © 2007 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2007 Will Thompson
 * Copyright (C) 2007 Collabora Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <glib-object.h>

#include <telepathy-glib/text-mixin.h>

G_BEGIN_DECLS

typedef struct _PhoneyIMChannel PhoneyIMChannel;
typedef struct _PhoneyIMChannelClass PhoneyIMChannelClass;

struct _PhoneyIMChannelClass {
	GObjectClass parent_class;

	TpTextMixinClass text_class;
};

struct _PhoneyIMChannel {
	GObject parent;

	TpTextMixin text;

	gpointer priv;
};

GType phoney_im_channel_get_type (void);

/* TYPE MACROS */
#define PHONEY_TYPE_IM_CHANNEL \
	(phoney_im_channel_get_type ())
#define PHONEY_IM_CHANNEL(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), PHONEY_TYPE_IM_CHANNEL, \
				    PhoneyIMChannel))
#define PHONEY_IM_CHANNEL_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), PHONEY_TYPE_IM_CHANNEL, \
				 PhoneyIMChannelClass))
#define PHONEY_IS_IM_CHANNEL(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), PHONEY_TYPE_IM_CHANNEL))
#define PHONEY_IS_IM_CHANNEL_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), PHONEY_TYPE_IM_CHANNEL))
#define PHONEY_IM_CHANNEL_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), PHONEY_TYPE_IM_CHANNEL, \
				    PhoneyIMChannelClass))

typedef struct _PhoneyConversationUiData PhoneyConversationUiData;

struct _PhoneyConversationUiData
{
	TpHandle contact_handle;
};

#define PURPLE_CONV_GET_PHONEY_UI_DATA(conv) \
	((PhoneyConversationUiData *) conv->ui_data)

G_END_DECLS

#endif /* #ifndef __PHONEY_IM_CHANNEL_H__*/
