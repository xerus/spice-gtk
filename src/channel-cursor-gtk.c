/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2017 Red Hat, Inc.

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
#include "config.h"

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include "spice-client.h"
#include "spice-util.h"
#include "channel-cursor-gtk.h"

/* since GLib 2.38 */
#ifndef g_assert_true
#define g_assert_true g_assert
#endif

struct _SpiceCursorChannelGtk {
    GObject parent;

    SpiceCursorChannel *cursor;

    GdkPoint hotspot;
    GdkPixbuf *cursor_pixbuf;
};

struct _SpiceCursorChannelGtkClass {
    GObjectClass parent_class;
};

/* properties */
enum {
    PROP_0,
    PROP_CHANNEL,
    PROP_CURSOR_PIXBUF,
};

/* ------------------------------------------------------------------ */
/* Prototypes for private functions */
static void cursor_set(SpiceCursorChannel *channel,
                       gint width, gint height, gint hot_x, gint hot_y,
                       gpointer rgba, gpointer data);

/* ------------------------------------------------------------------ */
/* gobject glue                                                       */

G_DEFINE_TYPE (SpiceCursorChannelGtk, spice_cursor_channel_gtk, G_TYPE_OBJECT);

static void spice_cursor_channel_gtk_init(G_GNUC_UNUSED SpiceCursorChannelGtk *self)
{
}

static GObject *
spice_cursor_channel_gtk_constructor(GType                  gtype,
                                     guint                  n_properties,
                                     GObjectConstructParam *properties)
{
    GObject *obj;
    SpiceCursorChannelGtk *self;

    {
        /* Always chain up to the parent constructor */
        GObjectClass *parent_class;
        parent_class = G_OBJECT_CLASS(spice_cursor_channel_gtk_parent_class);
        obj = parent_class->constructor(gtype, n_properties, properties);
    }

    self = SPICE_CURSOR_CHANNEL_GTK(obj);
    if (self->cursor == NULL) {
        g_error("SpiceCursorChannelGtk constructed without an cursor channel");
    }

    g_signal_connect(self->cursor, "cursor-set", G_CALLBACK(cursor_set), self);

    return obj;
}

static void spice_cursor_channel_gtk_finalize(GObject *gobject)
{
    SpiceCursorChannelGtk *self = SPICE_CURSOR_CHANNEL_GTK(gobject);

    g_clear_object(&self->cursor_pixbuf);

    /* Chain up to the parent class */
    if (G_OBJECT_CLASS(spice_cursor_channel_gtk_parent_class)->finalize)
        G_OBJECT_CLASS(spice_cursor_channel_gtk_parent_class)->finalize(gobject);
}

static void spice_cursor_channel_gtk_dispose(GObject *gobject)
{
    SpiceCursorChannelGtk *self = SPICE_CURSOR_CHANNEL_GTK(gobject);

    if (self->cursor != NULL) {
        g_signal_handlers_disconnect_by_func(self->cursor,
                                             G_CALLBACK(cursor_set),
                                             self);
        self->cursor = NULL;
    }

    /* Chain up to the parent class */
    if (G_OBJECT_CLASS(spice_cursor_channel_gtk_parent_class)->dispose)
        G_OBJECT_CLASS(spice_cursor_channel_gtk_parent_class)->dispose(gobject);
}

static void spice_cursor_channel_gtk_get_property(GObject    *gobject,
                                                  guint       prop_id,
                                                  GValue     *value,
                                                  GParamSpec *pspec)
{
    SpiceCursorChannelGtk *self = SPICE_CURSOR_CHANNEL_GTK(gobject);

    switch (prop_id) {
    case PROP_CHANNEL:
        g_value_set_object(value, self->cursor);
        break;
    case PROP_CURSOR_PIXBUF:
        g_value_set_object(value, self->cursor_pixbuf);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, pspec);
        break;
    }
}

static void spice_cursor_channel_gtk_set_property(GObject      *gobject,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec)
{
    SpiceCursorChannelGtk *self = SPICE_CURSOR_CHANNEL_GTK(gobject);

    switch (prop_id) {
    case PROP_CHANNEL:
        self->cursor = g_value_get_object(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, pspec);
        break;
    }
}

static void spice_cursor_channel_gtk_class_init(SpiceCursorChannelGtkClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->constructor  = spice_cursor_channel_gtk_constructor;
    gobject_class->dispose      = spice_cursor_channel_gtk_dispose;
    gobject_class->finalize     = spice_cursor_channel_gtk_finalize;
    gobject_class->get_property = spice_cursor_channel_gtk_get_property;
    gobject_class->set_property = spice_cursor_channel_gtk_set_property;

    /**
     * SpiceCursorChannelGtk:cursor-pixbuf:
     *
     * Represents the current #GdkPixbuf to be used for creating a cursor
     * using gdk_cursor_new_from_pixbuf
     *
     **/
    g_object_class_install_property
        (gobject_class, PROP_CURSOR_PIXBUF,
         g_param_spec_object("cursor-pixbuf",
                             "Pixbuf of the cursor",
                             "GdkPixbuf to be used for creating a cursor",
                             GDK_TYPE_PIXBUF,
                             G_PARAM_READABLE |
                             G_PARAM_STATIC_STRINGS));

    /**
     * SpiceCursorChannelGtk:channel:
     *
     * #SpiceCursorChannel this #SpiceCursorChannelGtk is associated with
     *
     **/
    g_object_class_install_property
        (gobject_class, PROP_CHANNEL,
         g_param_spec_object("channel",
                             "Pixbuf of the cursor",
                             "GdkPixbuf to be used for creating a cursor",
                             SPICE_TYPE_CHANNEL,
                             G_PARAM_READWRITE |
                             G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_STRINGS));
}


static void cursor_set(SpiceCursorChannel *channel,
                       gint width, gint height, gint x_hot, gint y_hot,
                       gpointer rgba, gpointer user_data)
{
    gchar *hotspot_str;
    SpiceCursorChannelGtk *self = user_data;

    self->hotspot.x = x_hot;
    self->hotspot.y = y_hot;

    g_return_if_fail(rgba != NULL);

    g_clear_object(&self->cursor_pixbuf);
    self->cursor_pixbuf = gdk_pixbuf_new_from_data(g_memdup(rgba, width * height * 4),
                                                   GDK_COLORSPACE_RGB,
                                                   TRUE,
                                                   8,
                                                   width,
                                                   height,
                                                   width * 4,
                                                   (GdkPixbufDestroyNotify) g_free,
                                                   NULL);
    /*
        It is possible to set cursor cordinates using pixbuf options and use them
        when creating the cursor.
        See gdk_cursor_new_from_pixbuf documentation for more information.
    */
    hotspot_str = g_strdup_printf("%d", x_hot);
    g_assert_true(gdk_pixbuf_set_option(self->cursor_pixbuf, "x_hot", hotspot_str));
    g_free(hotspot_str);

    hotspot_str = g_strdup_printf("%d", y_hot);
    g_assert_true(gdk_pixbuf_set_option(self->cursor_pixbuf, "y_hot", hotspot_str));
    g_free(hotspot_str);

    g_object_notify(G_OBJECT(self), "cursor-pixbuf");
}

SpiceCursorChannelGtk *spice_cursor_channel_gtk_get(SpiceCursorChannel *channel)
{
    SpiceCursorChannelGtk *self;
    static GMutex mutex;

    g_return_val_if_fail(SPICE_IS_CURSOR_CHANNEL(channel), NULL);

    g_mutex_lock(&mutex);
    self = g_object_get_data(G_OBJECT(channel), "spice-channel-cursor-gtk");
    if (self == NULL) {
        self = g_object_new(SPICE_TYPE_CURSOR_CHANNEL_GTK, "channel", channel, NULL);
        g_object_set_data_full(G_OBJECT(channel), "spice-channel-cursor-gtk", self, g_object_unref);
    }
    g_mutex_unlock(&mutex);

    return self;
}

void spice_cursor_channel_gtk_get_hotspot(SpiceCursorChannelGtk *self, GdkPoint *hotspot)
{
    g_return_if_fail(SPICE_IS_CURSOR_CHANNEL_GTK(self));
    g_return_if_fail(hotspot != NULL);

    *hotspot = self->hotspot;
}
