/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

#include <config.h>

#include <meta/meta-background-group.h>
#include "meta-cullable.h"

static void cullable_iface_init (MetaCullableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MetaBackgroundGroup, meta_background_group, CLUTTER_TYPE_ACTOR,
                         G_IMPLEMENT_INTERFACE (META_TYPE_CULLABLE, cullable_iface_init));

static void
meta_background_group_class_init (MetaBackgroundGroupClass *klass)
{
}

static void
meta_background_group_cull_out (MetaCullable   *cullable,
                                cairo_region_t *unobscured_region,
                                cairo_region_t *clip_region)
{
  meta_cullable_cull_out_children (cullable, unobscured_region, clip_region);
}

static void
meta_background_group_reset_culling (MetaCullable *cullable)
{
  meta_cullable_reset_culling_children (cullable);
}

static void
cullable_iface_init (MetaCullableInterface *iface)
{
  iface->cull_out = meta_background_group_cull_out;
  iface->reset_culling = meta_background_group_reset_culling;
}

static void
meta_background_group_init (MetaBackgroundGroup *self)
{
}

ClutterActor *
meta_background_group_new (void)
{
  MetaBackgroundGroup *background_group;

  background_group = g_object_new (META_TYPE_BACKGROUND_GROUP, NULL);

  return CLUTTER_ACTOR (background_group);
}
