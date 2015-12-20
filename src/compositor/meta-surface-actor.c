/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/**
 * SECTION:meta-surface-actor
 * @title: MetaSurfaceActor
 * @short_description: An actor representing a surface in the scene graph
 *
 * A surface can be either a shaped texture, or a group of shaped texture,
 * used to draw the content of a window.
 */

#include <config.h>
#include <clutter/clutter.h>
#include <cogl/cogl-texture-pixmap-x11.h>
#include <meta/meta-shaped-texture.h>
#include "meta-surface-actor.h"
#include "meta-cullable.h"

struct _MetaSurfaceActorPrivate
{
  MetaShapedTexture *texture;
  Pixmap pixmap;

  /* The region that is visible, used to optimize out redraws */
  cairo_region_t   *unobscured_region;

  cairo_region_t *input_region;
};

static void cullable_iface_init (MetaCullableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MetaSurfaceActor, meta_surface_actor, CLUTTER_TYPE_ACTOR,
                         G_IMPLEMENT_INTERFACE (META_TYPE_CULLABLE, cullable_iface_init));

static gboolean
meta_surface_actor_get_paint_volume (ClutterActor       *actor,
                                     ClutterPaintVolume *volume)
{
  MetaSurfaceActor *self = META_SURFACE_ACTOR (actor);
  MetaSurfaceActorPrivate *priv = self->priv;

  if (!CLUTTER_ACTOR_CLASS (meta_surface_actor_parent_class)->get_paint_volume (actor, volume))
    return FALSE;

  if (priv->unobscured_region)
    {
      ClutterVertex origin;
      cairo_rectangle_int_t bounds, unobscured_bounds;

      /* I hate ClutterPaintVolume so much... */
      clutter_paint_volume_get_origin (volume, &origin);
      bounds.x = origin.x;
      bounds.y = origin.y;
      bounds.width = clutter_paint_volume_get_width (volume);
      bounds.height = clutter_paint_volume_get_height (volume);

      cairo_region_get_extents (priv->unobscured_region, &unobscured_bounds);
      gdk_rectangle_intersect (&bounds, &unobscured_bounds, &bounds);

      origin.x = bounds.x;
      origin.y = bounds.y;
      clutter_paint_volume_set_origin (volume, &origin);
      clutter_paint_volume_set_width (volume, bounds.width);
      clutter_paint_volume_set_height (volume, bounds.height);
    }

  return TRUE;
}

gboolean
meta_surface_actor_get_unobscured_bounds (MetaSurfaceActor      *self,
                                          cairo_rectangle_int_t *unobscured_bounds)
{
  MetaSurfaceActorPrivate *priv = self->priv;
  return meta_shaped_texture_get_unobscured_bounds (priv->texture, unobscured_bounds);
}

static void
meta_surface_actor_pick (ClutterActor       *actor,
                         const ClutterColor *color)
{
  MetaSurfaceActor *self = META_SURFACE_ACTOR (actor);
  MetaSurfaceActorPrivate *priv = self->priv;

  if (!clutter_actor_should_pick_paint (actor))
    return;

  /* If there is no region then use the regular pick */
  if (priv->input_region == NULL)
    CLUTTER_ACTOR_CLASS (meta_surface_actor_parent_class)->pick (actor, color);
  else
    {
      int n_rects;
      float *rectangles;
      int i;
      CoglPipeline *pipeline;
      CoglContext *ctx;
      CoglFramebuffer *fb;
      CoglColor cogl_color;

      n_rects = cairo_region_num_rectangles (priv->input_region);
      rectangles = g_alloca (sizeof (float) * 4 * n_rects);

      for (i = 0; i < n_rects; i++)
        {
          cairo_rectangle_int_t rect;
          int pos = i * 4;

          cairo_region_get_rectangle (priv->input_region, i, &rect);

          rectangles[pos + 0] = rect.x;
          rectangles[pos + 1] = rect.y;
          rectangles[pos + 2] = rect.x + rect.width;
          rectangles[pos + 3] = rect.y + rect.height;
        }

      ctx = clutter_backend_get_cogl_context (clutter_get_default_backend ());
      fb = cogl_get_draw_framebuffer ();

      cogl_color_init_from_4ub (&cogl_color, color->red, color->green, color->blue, color->alpha);

      pipeline = cogl_pipeline_new (ctx);
      cogl_pipeline_set_color (pipeline, &cogl_color);
      cogl_framebuffer_draw_rectangles (fb, pipeline, rectangles, n_rects);
      cogl_object_unref (pipeline);
    }
}

static void
meta_surface_actor_dispose (GObject *object)
{
  MetaSurfaceActor *self = META_SURFACE_ACTOR (object);
  MetaSurfaceActorPrivate *priv = self->priv;

  g_clear_pointer (&priv->input_region, cairo_region_destroy);

  G_OBJECT_CLASS (meta_surface_actor_parent_class)->dispose (object);
}

static void
meta_surface_actor_class_init (MetaSurfaceActorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  object_class->dispose = meta_surface_actor_dispose;
  actor_class->pick = meta_surface_actor_pick;

  g_type_class_add_private (klass, sizeof (MetaSurfaceActorPrivate));
}

static void
set_unobscured_region (MetaSurfaceActor *self,
                       cairo_region_t   *unobscured_region)
{
  MetaSurfaceActorPrivate *priv = self->priv;

  if (priv->unobscured_region)
    cairo_region_destroy (priv->unobscured_region);

  if (unobscured_region)
    priv->unobscured_region = cairo_region_copy (unobscured_region);
  else
    priv->unobscured_region = NULL;
}

static void
meta_surface_actor_cull_out (MetaCullable   *cullable,
                             cairo_region_t *unobscured_region,
                             cairo_region_t *clip_region)
{
  MetaSurfaceActor *self = META_SURFACE_ACTOR (cullable);

  set_unobscured_region (self, unobscured_region);
  meta_cullable_cull_out_children (cullable, unobscured_region, clip_region);
}

static void
meta_surface_actor_reset_culling (MetaCullable *cullable)
{
  MetaSurfaceActor *self = META_SURFACE_ACTOR (cullable);

  set_unobscured_region (self, NULL);
  meta_cullable_reset_culling_children (cullable);
}

static void
cullable_iface_init (MetaCullableInterface *iface)
{
  iface->cull_out = meta_surface_actor_cull_out;
  iface->reset_culling = meta_surface_actor_reset_culling;
}

static void
meta_surface_actor_init (MetaSurfaceActor *self)
{
  MetaSurfaceActorPrivate *priv;

  priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                                   META_TYPE_SURFACE_ACTOR,
                                                   MetaSurfaceActorPrivate);

  priv->texture = META_SHAPED_TEXTURE (meta_shaped_texture_new ());
  clutter_actor_add_child (CLUTTER_ACTOR (self), CLUTTER_ACTOR (priv->texture));
}

cairo_surface_t *
meta_surface_actor_get_image (MetaSurfaceActor      *self,
                              cairo_rectangle_int_t *clip)
{
  return meta_shaped_texture_get_image (self->priv->texture, clip);
}

MetaShapedTexture *
meta_surface_actor_get_texture (MetaSurfaceActor *self)
{
  return self->priv->texture;
}

static void
update_area (MetaSurfaceActor *self,
             int x, int y, int width, int height)
{
  MetaSurfaceActorPrivate *priv = self->priv;

  CoglTexturePixmapX11 *texture = COGL_TEXTURE_PIXMAP_X11 (meta_shaped_texture_get_texture (priv->texture));
  cogl_texture_pixmap_x11_update_area (texture, x, y, width, height);
}

static cairo_region_t *
effective_unobscured_region (MetaSurfaceActor *self)
{
  MetaSurfaceActorPrivate *priv = self->priv;

  return clutter_actor_has_mapped_clones (CLUTTER_ACTOR (self)) ? NULL : priv->unobscured_region;
}

gboolean
meta_surface_actor_damage_all (MetaSurfaceActor *self)
{
  MetaSurfaceActorPrivate *priv = self->priv;
  CoglTexture *texture = meta_shaped_texture_get_texture (priv->texture);

  update_area (self, 0, 0, cogl_texture_get_width (texture), cogl_texture_get_height (texture));
  return meta_shaped_texture_update_area (priv->texture, 0, 0, cogl_texture_get_width (texture), cogl_texture_get_height (texture));
}

gboolean
meta_surface_actor_damage_area (MetaSurfaceActor *self,
                                int               x,
                                int               y,
                                int               width,
                                int               height)
{
  MetaSurfaceActorPrivate *priv = self->priv;

  update_area (self, x, y, width, height);
  return meta_shaped_texture_update_area (priv->texture, x, y, width, height);
}

gboolean
meta_surface_actor_is_obscured (MetaSurfaceActor *self)
{
  MetaSurfaceActorPrivate *priv = self->priv;
  return meta_shaped_texture_is_obscured (priv->texture);
}

void
meta_surface_actor_set_texture (MetaSurfaceActor *self,
                                CoglTexture      *texture)
{
  MetaSurfaceActorPrivate *priv = self->priv;
  meta_shaped_texture_set_texture (priv->texture, texture);
}

void
meta_surface_actor_set_input_region (MetaSurfaceActor *self,
                                     cairo_region_t   *region)
{
  MetaSurfaceActorPrivate *priv = self->priv;
  
  if (priv->input_region)
    cairo_region_destroy (priv->input_region);

  if (region)
    priv->input_region = cairo_region_reference (region);
  else
    priv->input_region = NULL;
}

void
meta_surface_actor_set_opaque_region (MetaSurfaceActor *self,
                                      cairo_region_t   *region)
{
  MetaSurfaceActorPrivate *priv = self->priv;
  meta_shaped_texture_set_opaque_region (priv->texture, region);
}

MetaSurfaceActor *
meta_surface_actor_new (void)
{
  return g_object_new (META_TYPE_SURFACE_ACTOR, NULL);
}
