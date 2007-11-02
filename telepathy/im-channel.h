#ifndef __SMS_IM_CHANNEL_H__
#define __SMS_IM_CHANNEL_H__
/*
 * im-channel.h - SmsImChannel header
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

typedef struct _SmsIMChannel SmsIMChannel;
typedef struct _SmsIMChannelClass SmsIMChannelClass;

struct _SmsIMChannelClass {
    GObjectClass parent_class;

    TpTextMixinClass text_class;
};

struct _SmsIMChannel {
    GObject parent;

    TpTextMixin text;

    gpointer priv;
};

GType sms_im_channel_get_type (void);

/* TYPE MACROS */
#define SMS_TYPE_IM_CHANNEL \
  (sms_im_channel_get_type ())
#define SMS_IM_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), SMS_TYPE_IM_CHANNEL, \
                              SmsIMChannel))
#define SMS_IM_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), SMS_TYPE_IM_CHANNEL, \
                           SmsIMChannelClass))
#define SMS_IS_IM_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), SMS_TYPE_IM_CHANNEL))
#define SMS_IS_IM_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), SMS_TYPE_IM_CHANNEL))
#define SMS_IM_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SMS_TYPE_IM_CHANNEL, \
                              SmsIMChannelClass))

typedef struct _SmsConversationUiData SmsConversationUiData;

struct _SmsConversationUiData
{
    TpHandle contact_handle;

//    PurpleTypingState active_state;
//    guint resend_typing_timeout_id;
};

#define PURPLE_CONV_GET_SMS_UI_DATA(conv) \
    ((SmsConversationUiData *) conv->ui_data)

G_END_DECLS

#endif /* #ifndef __SMS_IM_CHANNEL_H__*/
