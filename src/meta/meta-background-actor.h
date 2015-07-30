/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * meta-background-actor.h: Actor for painting the root window background
 *
 * Copyright 2010 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Suite 500, Boston, MA
 * 02110-1335, USA.
 */

#ifndef META_BACKGROUND_ACTOR_H
#define META_BACKGROUND_ACTOR_H

#include <clutter/clutter.h>

#include <meta/screen.h>

/**
 * MetaBackgroundActor:
 *
 * This class handles tracking and painting the root window background.
 * By integrating with #MetaWindowGroup we can avoid painting parts of
 * the background that are obscured by other windows.
 */

#define META_TYPE_BACKGROUND_ACTOR            (meta_background_actor_get_type ())
#define META_BACKGROUND_ACTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), META_TYPE_BACKGROUND_ACTOR, MetaBackgroundActor))
#define META_BACKGROUND_ACTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), META_TYPE_BACKGROUND_ACTOR, MetaBackgroundActorClass))
#define META_IS_BACKGROUND_ACTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), META_TYPE_BACKGROUND_ACTOR))
#define META_IS_BACKGROUND_ACTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), META_TYPE_BACKGROUND_ACTOR))
#define META_BACKGROUND_ACTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), META_TYPE_BACKGROUND_ACTOR, MetaBackgroundActorClass))

typedef struct _MetaBackgroundActor        MetaBackgroundActor;
typedef struct _MetaBackgroundActorClass   MetaBackgroundActorClass;
typedef struct _MetaBackgroundActorPrivate MetaBackgroundActorPrivate;

struct _MetaBackgroundActorClass
{
  ClutterActorClass parent_class;
};

struct _MetaBackgroundActor
{
  ClutterActor parent;

  MetaBackgroundActorPrivate *priv;
};

GType meta_background_actor_get_type (void);

ClutterActor *meta_background_actor_new_for_screen (MetaScreen *screen);

/**
 * MetaSnippetHook:
 * Temporary hack to work around Cogl not exporting CoglSnippetHook in
 * the 1.0 API. Don't use.
 */
typedef enum {
  /* Per pipeline vertex hooks */
  META_SNIPPET_HOOK_VERTEX = 0,
  META_SNIPPET_HOOK_VERTEX_TRANSFORM,

  /* Per pipeline fragment hooks */
  META_SNIPPET_HOOK_FRAGMENT = 2048,

  /* Per layer vertex hooks */
  META_SNIPPET_HOOK_TEXTURE_COORD_TRANSFORM = 4096,

  /* Per layer fragment hooks */
  META_SNIPPET_HOOK_LAYER_FRAGMENT = 6144,
  META_SNIPPET_HOOK_TEXTURE_LOOKUP
} MetaSnippetHook;


void meta_background_actor_add_glsl_snippet (MetaBackgroundActor *actor,
                                             MetaSnippetHook      hook,
                                             const char          *declarations,
                                             const char          *code,
                                             gboolean             is_replace);

void meta_background_actor_set_uniform_float (MetaBackgroundActor *actor,
                                              const char          *uniform_name,
                                              int                  n_components,
                                              int                  count,
                                              const float         *uniform,
                                              int                  uniform_length);

#endif /* META_BACKGROUND_ACTOR_H */
