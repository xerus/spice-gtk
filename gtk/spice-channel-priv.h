/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2010 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __SPICE_CLIENT_CHANNEL_PRIV_H__
#define __SPICE_CLIENT_CHANNEL_PRIV_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <openssl/ssl.h>
#include <gio/gio.h>

#if HAVE_SASL
#include <sasl/sasl.h>
#endif

#include "coroutine.h"
#include "gio-coroutine.h"

/* common/ */
#include "marshallers.h"
#include "demarshallers.h"

#include "ssl_verify.h"

G_BEGIN_DECLS

struct _SpiceMsgOut {
    int                   refcount;
    SpiceChannel          *channel;
    SpiceMessageMarshallers *marshallers;
    SpiceMarshaller       *marshaller;
    SpiceDataHeader       *header;
    gboolean              ro_check;
};

struct _SpiceMsgIn {
    int                   refcount;
    SpiceChannel          *channel;
    SpiceDataHeader       header;
    uint8_t               *data;
    int                   hpos,dpos;
    uint8_t               *parsed;
    size_t                psize;
    message_destructor_t  pfree;
    SpiceMsgIn            *parent;
};

enum spice_channel_state {
    SPICE_CHANNEL_STATE_UNCONNECTED = 0,
    SPICE_CHANNEL_STATE_CONNECTING,
    SPICE_CHANNEL_STATE_LINK_HDR,
    SPICE_CHANNEL_STATE_LINK_MSG,
    SPICE_CHANNEL_STATE_AUTH,
    SPICE_CHANNEL_STATE_READY,
    SPICE_CHANNEL_STATE_SWITCHING,
    SPICE_CHANNEL_STATE_MIGRATING,
};

struct _SpiceChannelPrivate {
    /* swapped on migration */
    SSL_CTX                     *ctx;
    SSL                         *ssl;
    SpiceOpenSSLVerify          *sslverify;
    GSocket                     *sock;

#if HAVE_SASL
    sasl_conn_t                 *sasl_conn;
    const char                  *sasl_decoded;
    unsigned int                sasl_decoded_length;
    unsigned int                sasl_decoded_offset;
#endif

    /* not swapped */
    SpiceSession                *session;
    struct coroutine            coroutine;
    int                         fd;
    gboolean                    has_error;
    guint                       connect_delayed_id;

    struct wait_queue           wait;
    GQueue                      xmit_queue;
    gboolean                    xmit_queue_blocked;
    GStaticMutex                xmit_queue_lock;
    GThread                     *main_thread;

    char                        name[16];
    enum spice_channel_state    state;
    spice_parse_channel_func_t  parser;
    SpiceMessageMarshallers     *marshallers;
    guint                       channel_watch;
    int                         tls;

    int                         connection_id;
    int                         channel_id;
    int                         channel_type;
    int                         serial;
    SpiceLinkHeader             link_hdr;
    SpiceLinkMess               link_msg;
    SpiceLinkHeader             peer_hdr;
    SpiceLinkReply*             peer_msg;
    int                         peer_pos;

    SpiceMsgIn                  *msg_in;
    int                         message_ack_window;
    int                         message_ack_count;

    GArray                      *caps;
    GArray                      *common_caps;
    GArray                      *remote_caps;
    GArray                      *remote_common_caps;

    gsize                       total_read_bytes;
    uint64_t                    last_message_serial;
};

SpiceMsgIn *spice_msg_in_new(SpiceChannel *channel);
SpiceMsgIn *spice_msg_in_sub_new(SpiceChannel *channel, SpiceMsgIn *parent,
                                   SpiceSubMessage *sub);
void spice_msg_in_ref(SpiceMsgIn *in);
void spice_msg_in_unref(SpiceMsgIn *in);
int spice_msg_in_type(SpiceMsgIn *in);
void *spice_msg_in_parsed(SpiceMsgIn *in);
void *spice_msg_in_raw(SpiceMsgIn *in, int *len);
void spice_msg_in_hexdump(SpiceMsgIn *in);

SpiceMsgOut *spice_msg_out_new(SpiceChannel *channel, int type);
void spice_msg_out_ref(SpiceMsgOut *out);
void spice_msg_out_unref(SpiceMsgOut *out);
void spice_msg_out_send(SpiceMsgOut *out);
void spice_msg_out_send_internal(SpiceMsgOut *out);
void spice_msg_out_hexdump(SpiceMsgOut *out, unsigned char *data, int len);

void spice_channel_up(SpiceChannel *channel);
void spice_channel_wakeup(SpiceChannel *channel);

SpiceSession* spice_channel_get_session(SpiceChannel *channel);

/* coroutine context */
typedef void (*handler_msg_in)(SpiceChannel *channel, SpiceMsgIn *msg, gpointer data);
void spice_channel_recv_msg(SpiceChannel *channel, handler_msg_in handler, gpointer data);

/* channel-base.c */
/* coroutine context */
void spice_channel_handle_set_ack(SpiceChannel *channel, SpiceMsgIn *in);
void spice_channel_handle_ping(SpiceChannel *channel, SpiceMsgIn *in);
void spice_channel_handle_notify(SpiceChannel *channel, SpiceMsgIn *in);
void spice_channel_handle_disconnect(SpiceChannel *channel, SpiceMsgIn *in);
void spice_channel_handle_wait_for_channels(SpiceChannel *channel, SpiceMsgIn *in);
void spice_channel_handle_migrate(SpiceChannel *channel, SpiceMsgIn *in);

gint spice_channel_get_channel_id(SpiceChannel *channel);
gint spice_channel_get_channel_type(SpiceChannel *channel);
void spice_channel_swap(SpiceChannel *channel, SpiceChannel *swap);
void spice_channel_set_common_capability(SpiceChannel *channel, guint32 cap);
gboolean spice_channel_get_read_only(SpiceChannel *channel);

void spice_channel_reset(SpiceChannel *channel, gboolean migrating);

/* coroutine context */
#define emit_main_context(object, event, args...)                       \
    G_STMT_START {                                                      \
        g_signal_emit_main_context(G_OBJECT(object), do_emit_main_context, \
                                   event, &((struct event) { args }), G_STRLOC); \
    } G_STMT_END


G_END_DECLS

#endif /* __SPICE_CLIENT_CHANNEL_PRIV_H__ */
