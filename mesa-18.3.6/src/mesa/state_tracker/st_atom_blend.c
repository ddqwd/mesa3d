/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  *   Brian Paul
  */
 

#include "st_context.h"
#include "st_atom.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "cso_cache/cso_context.h"

#include "framebuffer.h"
#include "main/blend.h"
#include "main/macros.h"

/**
 * Convert GLenum blend tokens to pipe tokens.
 * Both blend factors and blend funcs are accepted.
 */
static GLuint
translate_blend(GLenum blend)
{
   switch (blend) {
   /* blend functions */
   case GL_FUNC_ADD:
      return PIPE_BLEND_ADD;
   case GL_FUNC_SUBTRACT:
      return PIPE_BLEND_SUBTRACT;
   case GL_FUNC_REVERSE_SUBTRACT:
      return PIPE_BLEND_REVERSE_SUBTRACT;
   case GL_MIN:
      return PIPE_BLEND_MIN;
   case GL_MAX:
      return PIPE_BLEND_MAX;

   /* blend factors */
   case GL_ONE:
      return PIPE_BLENDFACTOR_ONE;
   case GL_SRC_COLOR:
      return PIPE_BLENDFACTOR_SRC_COLOR;
   case GL_SRC_ALPHA:
      return PIPE_BLENDFACTOR_SRC_ALPHA;
   case GL_DST_ALPHA:
      return PIPE_BLENDFACTOR_DST_ALPHA;
   case GL_DST_COLOR:
      return PIPE_BLENDFACTOR_DST_COLOR;
   case GL_SRC_ALPHA_SATURATE:
      return PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case GL_CONSTANT_COLOR:
      return PIPE_BLENDFACTOR_CONST_COLOR;
   case GL_CONSTANT_ALPHA:
      return PIPE_BLENDFACTOR_CONST_ALPHA;
   case GL_SRC1_COLOR:
      return PIPE_BLENDFACTOR_SRC1_COLOR;
   case GL_SRC1_ALPHA:
      return PIPE_BLENDFACTOR_SRC1_ALPHA;
   case GL_ZERO:
      return PIPE_BLENDFACTOR_ZERO;
   case GL_ONE_MINUS_SRC_COLOR:
      return PIPE_BLENDFACTOR_INV_SRC_COLOR;
   case GL_ONE_MINUS_SRC_ALPHA:
      return PIPE_BLENDFACTOR_INV_SRC_ALPHA;
   case GL_ONE_MINUS_DST_COLOR:
      return PIPE_BLENDFACTOR_INV_DST_COLOR;
   case GL_ONE_MINUS_DST_ALPHA:
      return PIPE_BLENDFACTOR_INV_DST_ALPHA;
   case GL_ONE_MINUS_CONSTANT_COLOR:
      return PIPE_BLENDFACTOR_INV_CONST_COLOR;
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      return PIPE_BLENDFACTOR_INV_CONST_ALPHA;
   case GL_ONE_MINUS_SRC1_COLOR:
      return PIPE_BLENDFACTOR_INV_SRC1_COLOR;
   case GL_ONE_MINUS_SRC1_ALPHA:
      return PIPE_BLENDFACTOR_INV_SRC1_ALPHA;
   default:
      assert("invalid GL token in translate_blend()" == NULL);
      return 0;
   }
}

/**
 * Figure out if colormasks are different per rt.
 */
static GLboolean
colormask_per_rt(const struct gl_context *ctx, unsigned num_cb)
{
   GLbitfield full_mask = _mesa_replicate_colormask(0xf, num_cb);
   GLbitfield repl_mask0 =
      _mesa_replicate_colormask(GET_COLORMASK(ctx->Color.ColorMask, 0),
                                num_cb);

   return (ctx->Color.ColorMask & full_mask) != repl_mask0;
}

/**
 * Figure out if blend enables/state are different per rt.
 */
static GLboolean
blend_per_rt(const struct gl_context *ctx, unsigned num_cb)
{
   GLbitfield cb_mask = u_bit_consecutive(0, num_cb);
   GLbitfield blend_enabled = ctx->Color.BlendEnabled & cb_mask;

   if (blend_enabled && blend_enabled != cb_mask) {
      /* This can only happen if GL_EXT_draw_buffers2 is enabled */
      return GL_TRUE;
   }
   if (ctx->Color._BlendFuncPerBuffer || ctx->Color._BlendEquationPerBuffer) {
      /* this can only happen if GL_ARB_draw_buffers_blend is enabled */
      return GL_TRUE;
   }
   if (ctx->DrawBuffer->_IntegerBuffers &&
       (ctx->DrawBuffer->_IntegerBuffers != cb_mask)) {
      /* If there is a mix of integer/non-integer buffers then blending
       * must be handled on a per buffer basis. */
      return GL_TRUE;
   }
   return GL_FALSE;
}

void
st_update_blend( struct st_context *st )
{
   struct pipe_blend_state *blend = &st->state.blend;
   const struct gl_context *ctx = st->ctx;
   unsigned num_cb = st->state.fb_num_cb;
   unsigned num_state = 1;
   unsigned i, j;

   memset(blend, 0, sizeof(*blend));

   if (num_cb > 1 &&
       (blend_per_rt(ctx, num_cb) || colormask_per_rt(ctx, num_cb))) {
      num_state = num_cb;
      blend->independent_blend_enable = 1;
   }

   for (i = 0; i < num_state; i++)
      blend->rt[i].colormask = GET_COLORMASK(ctx->Color.ColorMask, i);

   if (ctx->Color.ColorLogicOpEnabled) {
      /* logicop enabled */
      blend->logicop_enable = 1;
      blend->logicop_func = ctx->Color._LogicOp;
   }
   else if (ctx->Color.BlendEnabled && !ctx->Color._AdvancedBlendMode) {
      /* blending enabled */
      for (i = 0, j = 0; i < num_state; i++) {
         if (!(ctx->Color.BlendEnabled & (1 << i)) ||
             (ctx->DrawBuffer->_IntegerBuffers & (1 << i)) ||
             !blend->rt[i].colormask)
            continue;

	 if (ctx->Extensions.ARB_draw_buffers_blend)
            j = i;

         blend->rt[i].blend_enable = 1;
         blend->rt[i].rgb_func =
            translate_blend(ctx->Color.Blend[j].EquationRGB);

         if (ctx->Color.Blend[i].EquationRGB == GL_MIN ||
             ctx->Color.Blend[i].EquationRGB == GL_MAX) {
            /* Min/max are special */
            blend->rt[i].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
            blend->rt[i].rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
         }
         else {
            blend->rt[i].rgb_src_factor =
               translate_blend(ctx->Color.Blend[j].SrcRGB);
            blend->rt[i].rgb_dst_factor =
               translate_blend(ctx->Color.Blend[j].DstRGB);
         }

         blend->rt[i].alpha_func =
            translate_blend(ctx->Color.Blend[j].EquationA);

         if (ctx->Color.Blend[i].EquationA == GL_MIN ||
             ctx->Color.Blend[i].EquationA == GL_MAX) {
            /* Min/max are special */
            blend->rt[i].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
            blend->rt[i].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
         }
         else {
            blend->rt[i].alpha_src_factor =
               translate_blend(ctx->Color.Blend[j].SrcA);
            blend->rt[i].alpha_dst_factor =
               translate_blend(ctx->Color.Blend[j].DstA);
         }
      }
   }
   else {
      /* no blending / logicop */
   }

   blend->dither = ctx->Color.DitherFlag;

   if (_mesa_is_multisample_enabled(ctx) &&
       !(ctx->DrawBuffer->_IntegerBuffers & 0x1)) {
      /* Unlike in gallium/d3d10 these operations are only performed
       * if both msaa is enabled and we have a multisample buffer.
       */
      blend->alpha_to_coverage = ctx->Multisample.SampleAlphaToCoverage;
      blend->alpha_to_one = ctx->Multisample.SampleAlphaToOne;
   }

   cso_set_blend(st->cso_context, blend);
}

void
st_update_blend_color(struct st_context *st)
{
   struct pipe_blend_color bc;

   COPY_4FV(bc.color, st->ctx->Color.BlendColorUnclamped);
   cso_set_blend_color(st->cso_context, &bc);
}