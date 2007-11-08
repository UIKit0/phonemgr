#ifndef __PHONEY_IM_CHANNEL_FACTORY_H__
#define __PHONEY_IM_CHANNEL_FACTORY_H__
/*
 * im-channel-factory.h - PhoneyImChannelFactory header
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

G_BEGIN_DECLS

#define PHONEY_TYPE_IM_CHANNEL_FACTORY \
	(phoney_im_channel_factory_get_type())
#define PHONEY_IM_CHANNEL_FACTORY(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), PHONEY_TYPE_IM_CHANNEL_FACTORY, \
				    PhoneyImChannelFactory))
#define PHONEY_IM_CHANNEL_FACTORY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), PHONEY_TYPE_IM_CHANNEL_FACTORY,GObject))
#define PHONEY_IS_IM_CHANNEL_FACTORY(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), PHONEY_TYPE_IM_CHANNEL_FACTORY))
#define PHONEY_IS_IM_CHANNEL_FACTORY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), PHONEY_TYPE_IM_CHANNEL_FACTORY))
#define PHONEY_IM_CHANNEL_FACTORY_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), PHONEY_TYPE_IM_CHANNEL_FACTORY, \
				   PhoneyImChannelFactoryClass))

typedef struct _PhoneyImChannelFactory      PhoneyImChannelFactory;
typedef struct _PhoneyImChannelFactoryClass PhoneyImChannelFactoryClass;

struct _PhoneyImChannelFactory {
	GObject parent;
};

struct _PhoneyImChannelFactoryClass {
	GObjectClass parent_class;
};

GType        phoney_im_channel_factory_get_type    (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PHONEY_IM_CHANNEL_FACTORY_H__ */

