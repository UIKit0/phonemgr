/*
 * im-channel.c - SmsIMChannel source
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

#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/interfaces.h>

#include "im-channel.h"
#include "connection.h"
#include "debug.h"

/* properties */
enum
{
  PROP_CONNECTION = 1,
  PROP_OBJECT_PATH,
  PROP_CHANNEL_TYPE,
  PROP_HANDLE_TYPE,
  PROP_HANDLE,

  LAST_PROPERTY
};

typedef struct _SmsIMChannelPrivate
{
    SmsConnection *conn;
    char *object_path;
    guint handle;

//    PurpleConversation *conv;

    gboolean closed;
    gboolean dispose_has_run;
} SmsIMChannelPrivate;

#define SMS_IM_CHANNEL_GET_PRIVATE(o) \
  ((SmsIMChannelPrivate *)o->priv)

static void channel_iface_init (gpointer, gpointer);
static void text_iface_init (gpointer, gpointer);
static void chat_state_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE(SmsIMChannel, sms_im_channel, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, channel_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_TEXT, text_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_IFACE, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_CHAT_STATE,
        chat_state_iface_init);
    )

static void
sms_im_channel_close (TpSvcChannel *iface,
                       DBusGMethodInvocation *context)
{
    SmsIMChannel *self = SMS_IM_CHANNEL (iface);
    SmsIMChannelPrivate *priv = SMS_IM_CHANNEL_GET_PRIVATE (self);

    if (!priv->closed)
    {
        tp_svc_channel_emit_closed (iface);
        priv->closed = TRUE;
    }

    tp_svc_channel_return_from_close(context);
}

static void
sms_im_channel_get_channel_type (TpSvcChannel *iface,
                                  DBusGMethodInvocation *context)
{
    tp_svc_channel_return_from_get_channel_type (context,
        TP_IFACE_CHANNEL_TYPE_TEXT);
}

static void
sms_im_channel_get_handle (TpSvcChannel *iface,
                            DBusGMethodInvocation *context)
{
    SmsIMChannel *self = SMS_IM_CHANNEL (iface);
    SmsIMChannelPrivate *priv = SMS_IM_CHANNEL_GET_PRIVATE (self);

    tp_svc_channel_return_from_get_handle (context, TP_HANDLE_TYPE_CONTACT,
        priv->handle);
}

#if 0
static gboolean
_chat_state_available (SmsIMChannel *chan)
{
    SmsIMChannelPrivate *priv = SMS_IM_CHANNEL_GET_PRIVATE (chan);
    PurplePluginProtocolInfo *prpl_info =
        PURPLE_PLUGIN_PROTOCOL_INFO (priv->conn->account->gc->prpl);

    return (prpl_info->send_typing != NULL);
}
#endif

static void
sms_im_channel_get_interfaces (TpSvcChannel *iface,
                                DBusGMethodInvocation *context)
{
    const char *no_interfaces[] = { NULL };
    const char *chat_state_ifaces[] =
        { TP_IFACE_CHANNEL_INTERFACE_CHAT_STATE, NULL };
    tp_svc_channel_return_from_get_interfaces (context, chat_state_ifaces);
}

static void
channel_iface_init (gpointer g_iface, gpointer iface_data)
{
    TpSvcChannelClass *klass = (TpSvcChannelClass *)g_iface;

#define IMPLEMENT(x) tp_svc_channel_implement_##x (\
    klass, sms_im_channel_##x)
    IMPLEMENT(close);
    IMPLEMENT(get_channel_type);
    IMPLEMENT(get_handle);
    IMPLEMENT(get_interfaces);
#undef IMPLEMENT
}

#if 0
const gchar *typing_state_names[] = {
    "not typing",
    "typing",
    "typed"
};

static gboolean
resend_typing_cb (gpointer data)
{
    PurpleConversation *conv = (PurpleConversation *)data;
    SmsConversationUiData *ui_data = PURPLE_CONV_GET_SMS_UI_DATA (conv);
    PurpleConnection *gc = purple_conversation_get_gc (conv);
    const gchar *who = purple_conversation_get_name (conv);
    PurpleTypingState typing = ui_data->active_state;

    DEBUG ("resending '%s' to %s", typing_state_names[typing], who);
    if (serv_send_typing (gc, who, typing))
    {
        return TRUE; /* Let's keep doing this thang. */
    }
    else
    {
        DEBUG ("clearing resend_typing_cb timeout");
        ui_data->resend_typing_timeout_id = 0;
        return FALSE;
    }
}
#endif
#if 0
static void
sms_im_channel_set_chat_state (TpSvcChannelInterfaceChatState *self,
                                guint state,
                                DBusGMethodInvocation *context)
{
    SmsIMChannel *chan = SMS_IM_CHANNEL (self);
    SmsIMChannelPrivate *priv = SMS_IM_CHANNEL_GET_PRIVATE (chan);

    PurpleConversation *conv = priv->conv;
    SmsConversationUiData *ui_data = PURPLE_CONV_GET_SMS_UI_DATA (conv);
    PurpleConnection *gc = purple_conversation_get_gc (conv);
    const gchar *who = purple_conversation_get_name (conv);

    GError *error = NULL;
    PurpleTypingState typing = PURPLE_NOT_TYPING;
    guint timeout;

//    g_assert (_chat_state_available (chan));
#if 0
    if (ui_data->resend_typing_timeout_id)
    {
        DEBUG ("clearing existing resend_typing_cb timeout");
        g_source_remove (ui_data->resend_typing_timeout_id);
        ui_data->resend_typing_timeout_id = 0;
    }
#endif
    switch (state)
    {
        case TP_CHANNEL_CHAT_STATE_GONE:
            DEBUG ("The Gone state may not be explicitly set");
            g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
                "The Gone state may not be explicitly set");
            break;
        case TP_CHANNEL_CHAT_STATE_INACTIVE:
        case TP_CHANNEL_CHAT_STATE_ACTIVE:
            typing = PURPLE_NOT_TYPING;
            break;
        case TP_CHANNEL_CHAT_STATE_PAUSED:
            typing = PURPLE_TYPED;
            break;
        case TP_CHANNEL_CHAT_STATE_COMPOSING:
            typing = PURPLE_TYPING;
            break;
        default:
            DEBUG ("Invalid chat state: %u", state);
            g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
                "Invalid chat state: %u", state);
    }

    if (error)
    {
          dbus_g_method_return_error (context, error);
          g_error_free (error);
          return;
    }

    DEBUG ("sending '%s' to %s", typing_state_names[typing], who);

    ui_data->active_state = typing;
    timeout = serv_send_typing (gc, who, typing);
    /* Apparently some protocols need you to repeatedly set the typing state,
     * so let's rig up a callback to do that.  serv_send_typing returns the
     * number of seconds till the state times out, or 0 if states don't time
     * out.
     *
     * That said, it would be stupid to repeatedly send not typing, so let's
     * not do that.
     */
    if (timeout && typing != PURPLE_NOT_TYPING)
    {
        ui_data->resend_typing_timeout_id = g_timeout_add (timeout * 1000,
            resend_typing_cb, conv);
    }

    tp_svc_channel_interface_chat_state_return_from_set_chat_state (context);
}
#endif

static void
chat_state_iface_init (gpointer g_iface, gpointer iface_data)
{
    TpSvcChannelInterfaceChatStateClass *klass =
        (TpSvcChannelInterfaceChatStateClass *)g_iface;
#if 0
#define IMPLEMENT(x) tp_svc_channel_interface_chat_state_implement_##x (\
    klass, sms_im_channel_##x)
    IMPLEMENT(set_chat_state);
#endif
#undef IMPLEMENT
}

void
sms_im_channel_send (TpSvcChannelTypeText *channel,
                      guint type,
                      const gchar *text,
                      DBusGMethodInvocation *context)
{
    SmsIMChannel *self = SMS_IM_CHANNEL (channel);
    SmsIMChannelPrivate *priv = SMS_IM_CHANNEL_GET_PRIVATE (self);
    GError *error = NULL;
//    PurpleMessageFlags flags = 0;
    char *message, *escaped;

    if (type >= NUM_TP_CHANNEL_TEXT_MESSAGE_TYPES) {
        DEBUG ("invalid message type %u", type);
        g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
                "invalid message type: %u", type);
        dbus_g_method_return_error (context, error);
        g_error_free (error);

        return;
    }

    if (type == TP_CHANNEL_TEXT_MESSAGE_TYPE_ACTION) {
        /* XXX this is not good enough for prpl-irc, which has a slash-command
         *     for actions and doesn't do special stuff to messages which happen
         *     to start with "/me ".
         */
        message = g_strconcat ("/me ", text, NULL);
    } else {
        message = g_strdup (text);
    }

    escaped = g_markup_escape_text (message, -1);

#if 0
    if (type == TP_CHANNEL_TEXT_MESSAGE_TYPE_AUTO_REPLY) {
        flags |= PURPLE_MESSAGE_AUTO_RESP;
    }

    purple_conv_im_send_with_flags (PURPLE_CONV_IM (priv->conv), escaped,
                                    flags);
#endif
    g_free (escaped);
    g_free (message);

    tp_svc_channel_type_text_return_from_send (context);
}

static void
text_iface_init (gpointer g_iface, gpointer iface_data)
{
    TpSvcChannelTypeTextClass *klass = (TpSvcChannelTypeTextClass *)g_iface;

    tp_text_mixin_iface_init (g_iface, iface_data);
#define IMPLEMENT(x) tp_svc_channel_type_text_implement_##x (\
        klass, sms_im_channel_##x)
    IMPLEMENT(send);
#undef IMPLEMENT
}

static void
sms_im_channel_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
    SmsIMChannel *chan = SMS_IM_CHANNEL (object);
    SmsIMChannelPrivate *priv = SMS_IM_CHANNEL_GET_PRIVATE (chan);

    switch (property_id) {
        case PROP_OBJECT_PATH:
            g_value_set_string (value, priv->object_path);
            break;
        case PROP_CHANNEL_TYPE:
            g_value_set_static_string (value, TP_IFACE_CHANNEL_TYPE_TEXT);
            break;
        case PROP_HANDLE_TYPE:
            g_value_set_uint (value, TP_HANDLE_TYPE_CONTACT);
            break;
        case PROP_HANDLE:
            g_value_set_uint (value, priv->handle);
            break;
        case PROP_CONNECTION:
            g_value_set_object (value, priv->conn);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
sms_im_channel_set_property (GObject     *object,
                              guint        property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
    SmsIMChannel *chan = SMS_IM_CHANNEL (object);
    SmsIMChannelPrivate *priv = SMS_IM_CHANNEL_GET_PRIVATE (chan);

    switch (property_id) {
        case PROP_OBJECT_PATH:
            g_free (priv->object_path);
            priv->object_path = g_value_dup_string (value);
            break;
        case PROP_HANDLE:
            /* we don't ref it here because we don't have access to the
             * contact repo yet - instead we ref it in the constructor.
             */
            priv->handle = g_value_get_uint (value);
            break;
        case PROP_HANDLE_TYPE:
            /* this property is writable in the interface, but not actually
             * meaningfully changable on this channel, so we do nothing.
             */
            break;
        case PROP_CONNECTION:
            priv->conn = g_value_get_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static GObject *
sms_im_channel_constructor (GType type, guint n_props,
                             GObjectConstructParam *props)
{
    GObject *obj;
    TpHandleRepoIface *contact_handles;
    TpBaseConnection *conn;
    SmsIMChannelPrivate *priv;
    const char *recipient;
    DBusGConnection *bus;

    obj = G_OBJECT_CLASS (sms_im_channel_parent_class)->
        constructor (type, n_props, props);
    priv = SMS_IM_CHANNEL_GET_PRIVATE(SMS_IM_CHANNEL(obj));
    conn = TP_BASE_CONNECTION(priv->conn);

    contact_handles = tp_base_connection_get_handles (conn,
        TP_HANDLE_TYPE_CONTACT);
    tp_handle_ref (contact_handles, priv->handle);
    tp_text_mixin_init (obj, G_STRUCT_OFFSET (SmsIMChannel, text),
                        contact_handles);

    bus = tp_get_bus ();
    dbus_g_connection_register_g_object (bus, priv->object_path, obj);

    recipient = tp_handle_inspect(contact_handles, priv->handle);
#if 0
    priv->conv = purple_conversation_new (PURPLE_CONV_TYPE_IM,
                                          priv->conn->account,
                                          recipient);
#endif
    priv->closed = FALSE;
    priv->dispose_has_run = FALSE;

    return obj;
}

static void
sms_im_channel_dispose (GObject *obj)
{
    SmsIMChannel *chan = SMS_IM_CHANNEL (obj);
    SmsIMChannelPrivate *priv = SMS_IM_CHANNEL_GET_PRIVATE (chan);

    if (priv->dispose_has_run)
        return;
    priv->dispose_has_run = TRUE;

    if (!priv->closed)
    {
        tp_svc_channel_emit_closed (obj);
        priv->closed = TRUE;
    }

    g_free (priv->object_path);
}

static void
sms_im_channel_class_init (SmsIMChannelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GParamSpec *param_spec;

    tp_text_mixin_class_init (object_class,
                              G_STRUCT_OFFSET(SmsIMChannelClass, text_class));

    g_type_class_add_private (klass, sizeof (SmsIMChannelPrivate));

    object_class->get_property = sms_im_channel_get_property;
    object_class->set_property = sms_im_channel_set_property;
    object_class->constructor = sms_im_channel_constructor;
    object_class->dispose = sms_im_channel_dispose;

    g_object_class_override_property (object_class, PROP_OBJECT_PATH,
        "object-path");
    g_object_class_override_property (object_class, PROP_CHANNEL_TYPE,
        "channel-type");
    g_object_class_override_property (object_class, PROP_HANDLE_TYPE,
        "handle-type");
    g_object_class_override_property (object_class, PROP_HANDLE,
        "handle");

    param_spec = g_param_spec_object ("connection", "SmsConnection object",
                                      "Sms connection object that owns this "
                                      "IM channel object.",
                                      SMS_TYPE_CONNECTION,
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_READWRITE |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB);
    g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);
}

static void
sms_im_channel_init (SmsIMChannel *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, SMS_TYPE_IM_CHANNEL,
                                              SmsIMChannelPrivate);
}
