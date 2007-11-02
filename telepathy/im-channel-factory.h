#ifndef __SMS_IM_CHANNEL_FACTORY_H__
#define __SMS_IM_CHANNEL_FACTORY_H__
/*
 * im-channel-factory.h - SmsImChannelFactory header
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

G_BEGIN_DECLS

#define SMS_TYPE_IM_CHANNEL_FACTORY \
    (sms_im_channel_factory_get_type())
#define SMS_IM_CHANNEL_FACTORY(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), SMS_TYPE_IM_CHANNEL_FACTORY, \
                                SmsImChannelFactory))
#define SMS_IM_CHANNEL_FACTORY_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), SMS_TYPE_IM_CHANNEL_FACTORY,GObject))
#define SMS_IS_IM_CHANNEL_FACTORY(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), SMS_TYPE_IM_CHANNEL_FACTORY))
#define SMS_IS_IM_CHANNEL_FACTORY_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), SMS_TYPE_IM_CHANNEL_FACTORY))
#define SMS_IM_CHANNEL_FACTORY_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), SMS_TYPE_IM_CHANNEL_FACTORY, \
                               SmsImChannelFactoryClass))

typedef struct _SmsImChannelFactory      SmsImChannelFactory;
typedef struct _SmsImChannelFactoryClass SmsImChannelFactoryClass;

struct _SmsImChannelFactory {
    GObject parent;
};

struct _SmsImChannelFactoryClass {
    GObjectClass parent_class;
};

GType        sms_im_channel_factory_get_type    (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __SMS_IM_CHANNEL_FACTORY_H__ */

