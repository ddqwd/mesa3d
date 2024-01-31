

# blitter 功能分析

blitter作为一个工具库存在，用于加速clear, clear_render_target, clear_depth_stencil, resource_copy_region, blit function操作

* clear,glClear
* clear_depth_stencil , clear_render_target glClearTex*
* resouce_copy_region , glteximage2d
* blit function ,glBlitFrame*


在openglcore4.5_texture_and_sampler 一文中已经提到它的使用， 它是在glClearSubImage中使用的，其中使用了clear_render_target, clear_depth_stencil.

在radeonsi驱动中分析使用。


## blitter 上下文创建,删除


```c

static struct pipe_context *si_create_context(struct pipe_screen *screen,
                                              unsigned flags)
	struct si_context *sctx = CALLOC_STRUCT(si_context);

	sctx->blitter = util_blitter_create(&sctx->b);

	sctx->blitter->draw_rectangle = si_draw_rectangle;
	sctx->blitter->skip_viewport_restore = true;

	if (sctx->blitter)
		util_blitter_destroy(sctx->blitter);


struct blitter_context *util_blitter_create(struct pipe_context *pipe);
   struct blitter_context_priv *ctx;
   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_rasterizer_state rs_state;
   struct pipe_sampler_state sampler_state;
   struct pipe_vertex_element velem[2];

   ctx->base.draw_rectangle = util_blitter_draw_rectangle;

   ctx = CALLOC_STRUCT(blitter_context_priv);

   ctx->base.draw_rectangle = util_blitter_draw_rectangle;

   ctx->bind_fs_state = pipe->bind_fs_state = si_bind_fs;

   ctx->has_* = pipe->screen->get_*_param(pipe->screen,...) > 0;

   ctx->blend[i][j] = pipe->create_blend_state(pipe, &blend);

   ctx->dsa_keep_depth_stencil =
      pipe->create_depth_stencil_alpha_state(pipe, &dsa);

   dsa.stencil[0].xxx = ...;

   rs_state.xxx = ... ;
   ctx->rs_state = pipe->create_rasterizer_state(pipe, &rs_state);

   rs_state.scissor = 1;
   ctx->rs_state_scissor = pipe->create_rasterizer_state(pipe, &rs_state);

   velem[i].xxx= ...
   ctx->velem_state = pipe->create_vertex_elements_state(pipe, 2, &velem[0]);

   return &ctx->base;


void util_blitter_destroy(struct blitter_context *blitter);
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = blitter->pipe;

   pipe->delete_blend_state(pipe, ctx->blend[i][j]);
   pipe->delete_blend_state(pipe, ctx->blend_clear[i]);

   pipe->delete_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
   pipe->delete_*_state (pipe, *)

   FREE(ctx);
```




* 在si_context 创建时，通过util_blitter_create 创建blitter上下文，创建完成时将draw_rectangle接口设置成si内部接口
* 在util_blitter_create内创建子类blitter_context_priv, 首先将创建shader的接口如bind_fs_state设置成驱动内,其次通过get_param获取对某些扩展的支持情况，之后创将radeonsi的混合状态，光栅化状态，顶点元素状态,用于之后保存恢复使用



## Clear

这里的clear指的是
```
/**
 * 清除一组当前绑定的缓冲区的指定数值。
 *
 * 除了已经要求保存的状态对象外，这些状态必须在混合器中保存：
 * - 片段着色器
 * - 深度模板透明度状态
 * - 混合状态
 */
void util_blitter_clear(struct blitter_context *blitter,
                        unsigned width, unsigned height, unsigned num_layers,
                        unsigned clear_buffers,
                        const union pipe_color_union *color,
                        double depth, unsigned stencil);

```

当调用glClear时使用 即glClear->si_clear-> util_blitter_clear


```
_mesa_Clear(GLbitfield mask)
   clear(ctx, mask, false);
        ....
       if (ctx->RasterDiscard)
           return;
       if (ctx->RenderMode == GL_RENDER) {
        
            GLbitfield bufferMask;

            if (mask & GL_COLOR_BUFFER_BIT) 
                for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
                   gl_buffer_index buf = ctx->DrawBuffer->_ColorDrawBufferIndexes[i];
               
                   if (buf != BUFFER_NONE && color_buffer_writes_enabled(ctx, i)) {
                      bufferMask |= 1 << buf;
                   }
                }

            ctx->Driver.Clear(ctx, bufferMask);
                st_Clear
             
        } 

st_Clear(struct gl_context *ctx, GLbitfield mask)
   struct gl_renderbuffer *depthRb
      = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb
      = ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;

   /* This makes sure the pipe has the latest scissor, etc values */
   st_validate_state(st, ST_PIPELINE_CLEAR);

   if (mask & BUFFER_BITS_COLOR) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
         gl_buffer_index b = ctx->DrawBuffer->_ColorDrawBufferIndexes[i];

         clear_buffers |= PIPE_CLEAR_COLOR0 << i;
   }


    st->pipe->clear(st->pipe, clear_buffers,
                      (union pipe_color_union*)&ctx->Color.ClearColor,
                      ctx->Depth.Clear, ctx->Stencil.Clear);
        si_clear



enum {
	SI_CLEAR         = SI_SAVE_FRAGMENT_STATE,
	SI_CLEAR_SURFACE = SI_SAVE_FRAMEBUFFER | SI_SAVE_FRAGMENT_STATE,
};

static void si_clear(struct pipe_context *ctx, unsigned buffers,
		     const union pipe_color_union *color,
		     double depth, unsigned stencil)

	struct si_context *sctx = (struct si_context *)ctx;
	struct pipe_framebuffer_state *fb = &sctx->framebuffer.state;
	struct pipe_surface *zsbuf = fb->zsbuf;
	struct si_texture *zstex =
		zsbuf ? (struct si_texture*)zsbuf->texture : NULL;

	if (buffers & PIPE_CLEAR_COLOR) 
		si_do_fast_color_clear(sctx, &buffers, color);
        
        // 无法快速清理的buffers 进入ublitter clear
		/* These buffers cannot use fast clear, make sure to disable expansion. */
		for (unsigned i = 0; i < fb->nr_cbufs; i++) {
            tex = (struct si_texture *)fb->cbufs[i]->texture;
    else if ...


	si_blitter_begin(sctx, SI_CLEAR);
	util_blitter_clear(sctx->blitter, fb->width, fb->height,
			   util_framebuffer_get_num_layers(fb),
	si_blitter_end(sctx);

               
```

* si_clear首先对于color buffer 采用si_do_fast_color_clear操作，这里会内部调用dcc，cs等操作，对于无法进行这里操作的缓冲使用blitter清除
* 可以看到blitter操作分为三步，保存shader 状态， 光栅化状态，fb状态 blitter_begin ,调用实际绘制矩形 并且完成绘制后恢复状态 blitter_clear， blitter_end设置标志控制寄存器状态下发.


```
void si_blitter_begin(struct si_context *sctx, enum si_blitter_op op)
    util_blitter_save_vertexr{tess,...}_shader(sctx->blitter, sctx->vs(fs,..)_shader.cso)

	util_blitter_save_rasterizer(sctx->blitter, sctx->queued.named.rasterizer);

    util_blitter_save_depth_stencil_alpha(sctx->blitter, sctx->queued.named.dsa);
    ...


void util_blitter_clear(struct blitter_context *blitter,
                        unsigned width, unsigned height, unsigned num_layers,
                        unsigned clear_buffers,
                        const union pipe_color_union *color,
                        double depth, unsigned stencil)
   util_blitter_clear_custom(blitter, width, height, num_layers,
                             clear_buffers, color, depth, stencil,
                             NULL, NULL);

      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
      struct pipe_stencil_ref sr = { { 0 } };

      util_blitter_common_clear_setup(blitter, width, height, clear_buffers,

      bind_fs_write_all_cbufs(ctx);

      memcpy(attrib.color, color->ui, sizeof(color->ui));

      //typedef void *(*blitter_get_vs_func)(struct blitter_context *blitter);
      blitter_get_vs_func get_vs;
      if (pass_generic)
          get_vs = get_vs_passthrough_pos_generic;
      blitter_set_common_draw_rect_state(ctx, false);
          pipe->bind_rasterizer_state(pipe, scissor ? ctx->rs_state_scissor
          if (ctx->has_geometry_shader or ctx-> has_tessellation)
              pipe->bind_gs{tcs,tes}_state(...)   
          if (ctx->has_stream_out)
              pipe->set_stream_output_targets(pipe, 0, NULL, NULL);
      blitter->draw_rectangle(blitter, ctx->velem_state, get_vs,
                              0, 0, width, height,
                              (float) depth, 1, type, &attrib);
          si_draw_rectangle
        

   util_blitter_restore_*_states(blitter);


void si_blitter_end(struct si_context *sctx)
	sctx->render_cond_force_off = false;

	/* Restore shader pointers because the VS blit shader changed all
	 * non-global VS user SGPRs. */
	sctx->shader_pointers_dirty |= SI_DESCS_SHADER_MASK(VERTEX);
	sctx->vertex_buffer_pointer_dirty = sctx->vb_descriptors_buffer != NULL;
	si_mark_atom_dirty(sctx, &sctx->atoms.s.shader_pointers);
```

* 在util_blitter_clear_custom 内 , bind_fs_write_all_cbufs 作用时为绘制矩形创建一个fs，该fs将输出颜色写入当前fb的所有颜色缓冲，get_vs作用时创将一个vs,根据当前上下文是否使用gs,tes，分别创建对应的伪造shader, 最后调用draw_rectangle进行draw


### blitter fs 创建


```

static void bind_fs_write_all_cbufs(struct blitter_context_priv *ctx)
   if (!ctx->fs_write_all_cbufs) {
      ctx->fs_write_all_cbufs =
         util_make_fragment_passthrough_shader(pipe, TGSI_SEMANTIC_GENERIC,TGSI_INTERPOLATE_CONSTANT, true);
               static const char shader_templ[] =
                     "FRAG\n"
                     "%s"
                     "DCL IN[0], %s[0], %s\n"
                     "DCL OUT[0], COLOR[0]\n"
            
                     "MOV OUT[0], IN[0]\n"
                     "END\n";
              sprintf(text, shader_templ,
                      write_all_cbufs ? "PROPERTY FS_COLOR0_WRITES_ALL_CBUFS 1\n" : "",

              pipe_shader_state_from_tgsi(&state, tokens);

              return pipe->create_fs_state(pipe, &state);
	                sctx->b.create_fs_state = si_create_shader_selector;

   }
   ctx->bind_fs_state(pipe, ctx->fs_write_all_cbufs);
	sctx->b.bind_fs_state = si_bind_ps_shader;

static void si_bind_ps_shader(struct pipe_context *ctx, void *state)
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_shader_selector *old_sel = sctx->ps_shader.cso;
	struct si_shader_selector *sel = state;

	si_update_common_shader_state(sctx);
	si_update_ps_colorbuf0_slot(sctx);
```

* 在bind_fs_write_all_cbufs 内， 对于初次使用blitter无pass fs, 调用util_make_fragment_passthrough_shader 创建fs

* 在util_make_fragment_passthrough_shader 内，直接写了个tgsi程序， 通过tgsi_text_translate将之翻译成tokens存入state中， 插入FS_COLOR0_WRITES_ALL_CBUFS 属性，最后通过create_fs_state = si_create_shader_selector创建ir

* FS_COLOR0_WRITES_ALL_CBUFS 指定对于片段着色器颜色0的写入将被复制到所有已绑定的颜色缓冲区（cbufs）
    

### LLVM IR生成中对mrt 的处理


```
static inline void si_shader_selector_key(struct pipe_context *ctx,
		if (sel->info.properties[TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS] &&
		    sel->info.colors_written == 0x1)
			key->part.ps.epilog.last_cbuf = MAX2(sctx->framebuffer.state.nr_cbufs, 1) - 1;


static void si_build_ps_epilog_function(struct si_shader_context *ctx,
					union si_shader_part_key *key)
	int last_color_export = -1;

	/* Find the last color export. */
	if (!key->ps_epilog.writes_z &&
	    !key->ps_epilog.writes_stencil &&
	    !key->ps_epilog.writes_samplemask) {
		unsigned spi_format = key->ps_epilog.states.spi_shader_col_format;

		/* If last_cbuf > 0, FS_COLOR0_WRITES_ALL_CBUFS is true. */
		if (colors_written == 0x1 && key->ps_epilog.states.last_cbuf > 0) {
			/* Just set this if any of the colorbuffers are enabled. */
			if (spi_format &
			    ((1ull << (4 * (key->ps_epilog.states.last_cbuf + 1))) - 1))
				last_color_export = 0;
		} else {
			for (i = 0; i < 8; i++)
				if (colors_written & (1 << i) &&
				    (spi_format >> (i * 4)) & 0xf)
					last_color_export = i;
		}
	}
	while (colors_written) {
		LLVMValueRef color[4];
		int mrt = u_bit_scan(&colors_written);

		for (i = 0; i < 4; i++)
			color[i] = LLVMGetParam(ctx->main_fn, vgpr++);

		si_export_mrt_color(bld_base, color, mrt,
				    fninfo.num_params - 1,
				    mrt == last_color_export, &exp);
	}

	if (exp.num)
		si_emit_ps_exports(ctx, &exp);


static void si_export_mrt_color(struct lp_build_tgsi_context *bld_base,
    ...

	/* If last_cbuf > 0, FS_COLOR0_WRITES_ALL_CBUFS is true. */
	if (ctx->shader->key.part.ps.epilog.last_cbuf > 0) {
		struct ac_export_args args[8];
		int c, last = -1;

		/* Get the export arguments, also find out what the last one is. */
		for (c = 0; c <= ctx->shader->key.part.ps.epilog.last_cbuf; c++) {
			si_llvm_init_export_args(ctx, color,
						 V_008DFC_SQ_EXP_MRT + c, &args[c]);
			if (args[c].enabled_channels)
				last = c;
		}

		/* Emit all exports. */
		for (c = 0; c <= ctx->shader->key.part.ps.epilog.last_cbuf; c++) {
			if (is_last && last == c) {
				args[c].valid_mask = 1; /* whether the EXEC mask is valid */
				args[c].done = 1; /* DONE bit */
			} else if (!args[c].enabled_channels)
				continue; /* unnecessary NULL export */

			memcpy(&exp->args[exp->num++], &args[c], sizeof(args[c]));
		}
	} else {
	
```

* 在si_shader_selector_key 中当前fbo绑定的颜色buf数设置last_buf
* 在si_build_ps_epilog_function中，从刚才的tgsi可以看到只是一个颜色写入，所以无论在哪种情况last_color_export都是0
* 对颜色输出调用si_export_mrt_color处理mrt
* 在si_export_mrt_color内，如果多个颜色buf存在 ，初始化导出参数，以及exp->num.
* 在si_build_ps_epilog_function 最后， 对每个exp->num 调用amdgpu内建函数@llvm.amdgcn.exp.compr输出该颜色到每个buf，最后清除完毕

### blitter vs

```
void si_draw_rectangle(struct blitter_context *blitter,
		       void *vertex_elements_cso,
		       blitter_get_vs_func get_vs,
		       int x1, int y1, int x2, int y2,
		       float depth, unsigned num_instances,
		       enum blitter_attrib_type type,
		       const union blitter_attrib *attrib)


	/* Pack position coordinates as signed int16. */
	sctx->vs_blit_sh_data[0] = (uint32_t)(x1 & 0xffff) |
				   ((uint32_t)(y1 & 0xffff) << 16);
	sctx->vs_blit_sh_data[1] = (uint32_t)(x2 & 0xffff) |
				   ((uint32_t)(y2 & 0xffff) << 16);
	sctx->vs_blit_sh_data[2] = fui(depth);

	pipe->bind_vs_state(pipe, si_get_blitter_vs(sctx, type, num_instances));

	struct pipe_draw_info info = {};
	info.mode = SI_PRIM_RECTANGLE_LIST;
	info.count = 3;
	info.instance_count = num_instances;

	/* Don't set per-stage shader pointers for VS. */
	sctx->shader_pointers_dirty &= ~SI_DESCS_SHADER_MASK(VERTEX);
	sctx->vertex_buffer_pointer_dirty = false;

	si_draw_vbo(pipe, &info);

void *si_get_blitter_vs(struct si_context *sctx, enum blitter_attrib_type type,
			unsigned num_layers)
	unsigned vs_blit_property;
	void **vs;

	switch (type) {
        ...
	case UTIL_BLITTER_ATTRIB_COLOR:
		vs = num_layers > 1 ? &sctx->vs_blit_color_layered :
				      &sctx->vs_blit_color;
		vs_blit_property = SI_VS_BLIT_SGPRS_POS_COLOR;
		break;
    ...

	if (*vs)
		return *vs;

	struct ureg_program *ureg = ureg_create(PIPE_SHADER_VERTEX);

	/* Tell the shader to load VS inputs from SGPRs: */
	ureg_property(ureg, TGSI_PROPERTY_VS_BLIT_SGPRS, vs_blit_property);
	ureg_property(ureg, TGSI_PROPERTY_VS_WINDOW_SPACE_POSITION, true);

    // 加入tgsi指令
    ureg_MOV*, ureg_system* 
	/* This is just a pass-through shader with 1-3 MOV instructions. */
	ureg_MOV(ureg,
		 ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0),

	*vs = ureg_create_shader_and_destroy(ureg, &sctx->b);
         void *result = ureg_create_shader( p, pipe, so );
                struct pipe_shader_state state;

                pipe_shader_state_from_tgsi(&state, ureg_finalize(ureg));
                switch (ureg->processor) {
                case PIPE_SHADER_VERTEX:
                   return pipe->create_vs_state(pipe, &state); -> void*
                        [return] si_create_shader_selector -> si_shader_selector*
                ...

    return *vs

```

* 在si_draw_rectangle 首先根据参数x,y设置vs_sh_data坐标,这里时把x1, x2, 通过组合放到一个32位数vs_sh_data[0]中,vice vesa
* 在si_get_blitter_vs中首先根据此次blitter清除的属性设置不同tgsi属性声明，这直接影响之后llvm ir main函数的生成。对于此次ClearColor，属性为*_POS_COLOR.
* 在si_get_blitter_vs 中首先根据属性值添加了两个属性  TGSI_PROPERTY_VS_BLIT_SGPRS = TGSI_PROPERTY_FS_COORD_ORIGIN, 
* TGSI_PROPERTY_VS_BLIT_SGPRS设置此属性的目的时设置之后的num_vs_blit_sgprs ，对于不同的清除目的， 采用的sgpr不同
```
	sctx->num_vs_blit_sgprs = sel ? sel->info.properties[TGSI_PROPERTY_VS_BLIT_SGPRS] : 0;
```
* TGSI_PROPERTY_FS_COORD_ORIGIN 会根据清除的类型设置不同，对于clearcolor这种，直接就是color, 对于 glClearTex会将之表示纹理坐标

* TGSI_PROPERTY_VS_WINDOW_SPACE_POSITION, VS_WINDOW_SPACE_POSITION表示直接将坐标翻译成实际窗口坐标

"如果在顶点着色器中设置了这个属性，那么假定TGSI_SEMANTIC_POSITION输出包含窗口空间坐标。不会进行X、Y、Z除以W和视口变换，而是直接从着色器输出的第四个分量中获取1/W。自然地，窗口坐标上也不执行裁剪操作。如果使用几何着色器或细分着色器，则此属性的效果是未定义的。"

这段文本描述了在顶点着色器中使用"VS_WINDOW_SPACE_POSITION"属性的效果。它表明，通过使用此属性，可以控制顶点着色器输出的坐标数据被解释为窗口空间坐标，而不执行坐标变换和裁剪操作。然而，需要注意的是，在使用几何着色器或细分着色器的情况下，此属性的行为是未定义的。 

* 之后使用ureg_MOV在vs main中加入了一些mov pass 指令， 
* 和fs创建不同这里是先创建ureg_program在翻译成tgsi,ureg_program一般用来表示glsl ir翻译后的中间程序
* 最后在ureg_create_shader_and_destroy 中创建tgsi_tokens. 编译shader到llvm ir
* 在ureg_create_shader 内， 使用ureg_finalize 将数据转化tgsi_tokens , pipe_shader_state_from_tgsi 获取tgsi_tokens,保存到state中，最后调用create_vs_state->si_shader_selector 开始编译shader llvm ir.

* 在si_draw_rectangle的最后，设置draw info，绘制一个矩形列表 SI_PRIM_RECTANGLE_LIST, 实际值为PIPE_PRIM_MAX, 目的时告诉gpu这是个清理作用

### llvm IR blitter vs 

在生成llvm ir时， blitter vs和普通vs, ls, es有所不同,具体体现在ir的创建时

```

static void create_function(struct si_shader_context *ctx)

    ...
	switch (type) {
	case PIPE_SHADER_VERTEX:
		declare_global_desc_pointers(ctx, &fninfo);
    	if (vs_blit_property) {
    			ctx->param_vs_blit_inputs = fninfo.num_params;
    			add_arg(&fninfo, ARG_SGPR, ctx->i32); /* i16 x1, y1 */
    			add_arg(&fninfo, ARG_SGPR, ctx->i32); /* i16 x2, y2 */
    			add_arg(&fninfo, ARG_SGPR, ctx->f32); /* depth */
    
    			if (vs_blit_property == SI_VS_BLIT_SGPRS_POS_COLOR) {
    				add_arg(&fninfo, ARG_SGPR, ctx->f32); /* color0 */
    				add_arg(&fninfo, ARG_SGPR, ctx->f32); /* color1 */
    				add_arg(&fninfo, ARG_SGPR, ctx->f32); /* color2 */
    				add_arg(&fninfo, ARG_SGPR, ctx->f32); /* color3 */
    			} else if (vs_blit_property == SI_VS_BLIT_SGPRS_POS_TEXCOORD) {
    				add_arg(&fninfo, ARG_SGPR, ctx->f32); /* texcoord.x1 */
    				add_arg(&fninfo, ARG_SGPR, ctx->f32); /* texcoord.y1 */
    				add_arg(&fninfo, ARG_SGPR, ctx->f32); /* texcoord.x2 */
    				add_arg(&fninfo, ARG_SGPR, ctx->f32); /* texcoord.y2 */
    				add_arg(&fninfo, ARG_SGPR, ctx->f32); /* texcoord.z */
    				add_arg(&fninfo, ARG_SGPR, ctx->f32); /* texcoord.w */
    			}

			/* VGPRs */
			declare_vs_input_vgprs(ctx, &fninfo, &num_prolog_vgprs);
			break;
	    }	



```

可以总结出如下表合格

SI_VS_BLIT_SGPRS_POS_COLOR

|参数索引|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0  |v4i32*| param_rw_buffers                  | SGPR| SPI_SHADER_USER_DATA_VS_0 |
| 1  |v8i32*| param_bindless_samplers_and_images| SGPR| SPI_SHADER_USER_DATA_VS_1 | 
| 2  |i32   |i16 x1,y1                          | SGPR| SPI_SHADER_USER_DATA_VS_2 |
| 3  |i32   | i16 x2, y2                        | SGPR| SPI_SHADER_USER_DATA_VS_3 |
| 4  |i32   | depth                             | SGPR| SPI_SHADER_USER_DATA_VS_4 | 
| 5  |i32   | color0                            | SGPR| SPI_SHADER_USER_DATA_VS_5 |
| 6  |i32   | color1                            | SGPR| SPI_SHADER_USER_DATA_VS_6 |
| 7  |i32   | color2                            | SGPR| SPI_SHADER_USER_DATA_VS_7 |
| 8  |i32   | color3                            | SGPR| SPI_SHADER_USER_DATA_VS_8 |
| 9  |i32   | vertex_id                         | VGPR| |
| 10 |i32   | instance_id                       | VGPR||
| 11 |i32   | param_vs_prim_id                  | VGPR||
| 12 |i32   | unused                            | VGPR ||


### blitter draw / shade参数下发

blitter 的参数数据在draw 时下发

```

void si_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *info)
    ...

    si_emit_draw_packets(sctx, info, indexbuf, index_size, index_offset);
        
	    unsigned sh_base_reg = sctx->shader_pointers.sh_base[PIPE_SHADER_VERTEX];
        ...

		if (sctx->num_vs_blit_sgprs) {
			/* Re-emit draw constants after we leave u_blitter. */
			si_invalidate_draw_sh_constants(sctx);
	            sctx->last_base_vertex = SI_BASE_VERTEX_UNKNOWN;

			/* Blit VS doesn't use BASE_VERTEX, START_INSTANCE, and DRAWID. */
			radeon_set_sh_reg_seq(cs, sh_base_reg + SI_SGPR_VS_BLIT_DATA * 4,
					      sctx->num_vs_blit_sgprs);
			radeon_emit_array(cs, sctx->vs_blit_sh_data,
					  sctx->num_vs_blit_sgprs);
		} 
        ...

    ...

```

* 从上可以看到blitter draw不会下发base_vertex, start_instance, drawid之类包，这也是ir 没有这类vgpr原因，si把参数数据从VS_2处依次下发下去，数量num_vs_blit_sgprs为除参数0和1之后的sgpr数量之和


## 调试

调试代码，其中fbo为了测试mrt

```
   // 创建帧缓冲对象（FBO）
    GLuint FBO;
    GLuint colorBuffer1, colorBuffer2;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // 创建两个颜色附件并附加到FBO
    glGenTextures(1, &colorBuffer1);
    glBindTexture(GL_TEXTURE_2D, colorBuffer1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer1, 0);

    glGenTextures(1, &colorBuffer2);
    glBindTexture(GL_TEXTURE_2D, colorBuffer2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, colorBuffer2, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("帧缓冲不完整！\n");
    }

    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

    glDrawBuffers(2, attachments);

    glClear(GL_COLOR_BUFFER_BIT );


```

执行以上测试用列 打印tgsi

```

FRAG
PROPERTY FS_COLOR0_WRITES_ALL_CBUFS 1
DCL IN[0], GENERIC[0], CONSTANT
DCL OUT[0], COLOR
  0: MOV OUT[0], IN[0]
  1: END


VERT
PROPERTY FS_COORD_ORIGIN 7
PROPERTY VS_WINDOW_SPACE_POSITION 1
PROPERTY NEXT_SHADER FRAG
DCL IN[0]
DCL IN[1]
DCL OUT[0], POSITION
DCL OUT[1], GENERIC[0]
  0: MOV OUT[0], IN[0]
  1: MOV OUT[1], IN[1]
  2: END


```

FS_COORD_ORIGIN 为7

从gallium_ddebug日志可以看到使用的blit sgpr数量为7

```

c0077600 SET_SH_REG:
0000004e
00000000         SPI_SHADER_USER_DATA_VS_2 <- 0
02580320         SPI_SHADER_USER_DATA_VS_3 <- 0x02580320
3f800000         SPI_SHADER_USER_DATA_VS_4 <- 1.0f (0x3f800000)
00000000         SPI_SHADER_USER_DATA_VS_5 <- 0
00000000         SPI_SHADER_USER_DATA_VS_6 <- 0
00000000         SPI_SHADER_USER_DATA_VS_7 <- 0
00000000         SPI_SHADER_USER_DATA_VS_8 <- 0

```
拆分0x02580320，
0x258对应十进制为600, 0x320对应十进制为800,与测试一致。


