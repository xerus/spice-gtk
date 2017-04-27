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
#ifndef __SPICE_CLIENT_GTK_CHANNEL_CURSOR_H__
#define __SPICE_CLIENT_GTK_CHANNEL_CURSOR_H__

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define SPICE_TYPE_CURSOR_CHANNEL_GTK            (spice_cursor_channel_gtk_get_type())
#define SPICE_CURSOR_CHANNEL_GTK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                                  SPICE_TYPE_CURSOR_CHANNEL_GTK, \
                                                  SpiceCursorChannelGtk))
#define SPICE_CURSOR_CHANNEL_GTK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), \
                                                  SPICE_TYPE_CURSOR_CHANNEL_GTK, \
                                                  SpiceCursorChannelGtkClass))
#define SPICE_IS_CURSOR_CHANNEL_GTK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                                  SPICE_TYPE_CURSOR_CHANNEL_GTK))
#define SPICE_IS_CURSOR_CHANNEL_GTK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
                                                  SPICE_TYPE_CURSOR_CHANNEL_GTK))
#define SPICE_CURSOR_CHANNEL_GTK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), \
                                                  SPICE_TYPE_CURSOR_CHANNEL_GTK, \
                                                  SpiceCursorChannelGtkClass))

typedef struct _SpiceCursorChannelGtk SpiceCursorChannelGtk;
typedef struct _SpiceCursorChannelGtkClass SpiceCursorChannelGtkClass;

GType spice_cursor_channel_gtk_get_type(void);

SpiceCursorChannelGtk *spice_cursor_channel_gtk_get(SpiceCursorChannel *channel);
void spice_cursor_channel_gtk_get_hotspot(SpiceCursorChannelGtk *self, GdkPoint *hotspot);
gboolean spice_channel_gtk_has_cursor(SpiceCursorChannelGtk *self);

G_END_DECLS

#endif /* __SPICE_CLIENT_GTK_CHANNEL_CURSOR_H__ */
