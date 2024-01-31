
##

void si_init_blit_functions(struct si_context *sctx)
{
	sctx->b.resource_copy_region = si_resource_copy_region;
	sctx->b.blit = si_blit;
	sctx->b.flush_resource = si_flush_resource;
	sctx->b.generate_mipmap = si_generate_mipmap;
}


## 

void si_resource_copy_region(struct pipe_context *ctx,
			     struct pipe_resource *dst,
			     unsigned dst_level,
			     unsigned dstx, unsigned dsty, unsigned dstz,
			     struct pipe_resource *src,
			     unsigned src_level,
			     const struct pipe_box *src_box)
	if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
		si_copy_buffer(sctx, dst, src, dstx, src_box->x, src_box->width);
		return;
	}


	/* Initialize the surface. */
	dst_view = si_create_surface_custom(ctx, dst, &dst_templ,
					      dst_width0, dst_height0,
					      dst_width, dst_height);

	/* Initialize the sampler view. */
	src_view = si_create_sampler_view_custom(ctx, src, &src_templ,
						 src_width0, src_height0,
						 src_force_level);

	u_box_3d(dstx, dsty, dstz, abs(src_box->width), abs(src_box->height),
		 abs(src_box->depth), &dstbox);

	/* Copy. */
	si_blitter_begin(sctx, SI_COPY);
	util_blitter_blit_generic(sctx->blitter, dst_view, &dstbox,
				  src_view, src_box, src_width0, src_height0,
				  PIPE_MASK_RGBAZS, PIPE_TEX_FILTER_NEAREST, NULL,
				  false);
	si_blitter_end(sctx);



util_blitter_blit_generic(sctx->blitter, dst_view, &dstbox,
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;

    // for blit_color
    unsigned colormask = mask & PIPE_MASK_RGBA;

    pipe->bind_blend_state(pipe, ctx->blend[colormask][alpha_blend]);
    pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
    ctx->bind_fs_state(pipe,
          blitter_get_fs_texfetch_col(ctx, src->format, dst->format, src_target,
                                      src_samples, dst_samples, filter,
                                      use_txf));
   

// same to clear oid bind_fs_write_all_cbufs(struct blitter_context_priv *ctx)
static void *blitter_get_fs_texfetch_col(struct blitter_context_priv *ctx,
                                         enum pipe_format src_format,
                                         enum pipe_format dst_format,
                                         enum pipe_texture_target target,
                                         unsigned src_nr_samples,
                                         unsigned dst_nr_samples,
                                         unsigned filter,
                                         bool use_txf)
    if(msaa) ...
    else 
      void **shader;

      if (use_txf)
         shader = &ctx->fs_texfetch_col[type][target][1];
      else
         shader = &ctx->fs_texfetch_col[type][target][0];

      /* Create the fragment shader on-demand. */
      if (!*shader) {
         assert(!ctx->cached_all_shaders);
         *shader = util_make_fragment_tex_shader(pipe, tgsi_tex,
                                                 TGSI_INTERPOLATE_LINEAR,
                                                 stype, dtype,
                                                 ctx->has_tex_lz, use_txf);
      }

      return *shader;

util_make_fragment_tex_shader(struct pipe_context *pipe,
   return util_make_fragment_tex_shader_writemask( pipe,
        
        ureg_mov,
        ureg_DECL_fs_input
        ...


        return ureg_create_shader_and_destroy( ureg, pipe );

```


* 在si_resource_copy_region内， 首先当源和目的对象均是缓冲对象时， 此时使用si_copy_buffer, si_copy_buffer 采用计算着色进行复制, see si_compute_blit.pdf
* 其次对fmask，dcc，cmask等需要解压缩的资源进行解压缩
* 之后调用si_create_surface_custom初始化目的采样预览


```
FRAG
DCL IN[0], GENERIC[0], LINEAR
DCL OUT[0], COLOR
DCL SAMP[0]
DCL SVIEW[0], 2D, FLOAT
DCL TEMP[0..1]
  0: F2I TEMP[1], IN[0]
  1: TXF_LZ TEMP[0], TEMP[1], SAMP[0], 2D
  2: MOV OUT[0], TEMP[0]
  3: END
$1 = void
```
