
## 

```
// 初始化新的图形命令流
void si_begin_new_gfx_cs(struct si_context *ctx)
{
	// 如果处于调试模式，启动图形命令流的调试
	if (ctx->is_debug)
		si_begin_gfx_cs_debug(ctx);

	/* 总是在IB（Indirect Buffer）开头使缓存失效，因为外部
	 * 用户（如BO（Buffer Object）驱逐和SDMA/UVD/VCE IBs）可能修改我们的
	 * 缓冲区。
	 *
	 * 请注意，内核在GFX IB的末尾执行的缓存刷新在这里无法使用，因为
	 * 该刷新可能在后续IB开始绘制之后才完成。
	 *
	 * TODO：我们是否还需要使CB（颜色缓存）和DB（深度缓存）缓存失效？
	 */
	ctx->flags |= SI_CONTEXT_INV_ICACHE |
		      SI_CONTEXT_INV_SMEM_L1 | SI_CONTEXT_INV_VMEM_L1 | SI_CONTEXT_INV_GLOBAL_L2 | SI_CONTEXT_START_PIPELINE_STATS;

	/* 将所有有效组标记为脏，以确保它们在下一个绘图命令中重新生成 */
	si_pm4_reset_emitted(ctx);

	/* CS初始化应该在所有其他操作之前发射 */
	si_pm4_emit(ctx, ctx->init_config);
	if (ctx->init_config_gs_rings)
		si_pm4_emit(ctx, ctx->init_config_gs_rings);

	// 根据已排队的着色器类型设置L2缓存预取掩码
	if (ctx->queued.named.ls)
		ctx->prefetch_L2_mask |= SI_PREFETCH_LS;
	if (ctx->queued.named.hs)
		ctx->prefetch_L2_mask |= SI_PREFETCH_HS;
	if (ctx->queued.named.es)
		ctx->prefetch_L2_mask |= SI_PREFETCH_ES;
	if (ctx->queued.named.gs)
		ctx->prefetch_L2_mask |= SI_PREFETCH_GS;
	if (ctx->queued.named.vs)
		ctx->prefetch_L2_mask |= SI_PREFETCH_VS;
	if (ctx->queued.named.ps)
		ctx->prefetch_L2_mask |= SI_PREFETCH_PS;
	if (ctx->vb_descriptors_buffer && ctx->vertex_elements)
		ctx->prefetch_L2_mask |= SI_PREFETCH_VBO_DESCRIPTORS;

	/* CLEAR_STATE禁用所有颜色缓存，因此只启用绑定的颜色缓存 */
	bool has_clear_state = ctx->screen->has_clear_state;
	if (has_clear_state) {
		ctx->framebuffer.dirty_cbufs =
			u_bit_consecutive(0, ctx->framebuffer.state.nr_cbufs);
		/* CLEAR_STATE禁用深度缓存，因此只有在绑定时才启用 */
		ctx->framebuffer.dirty_zsbuf = ctx->framebuffer.state.zsbuf != NULL;
	} else {
		ctx->framebuffer.dirty_cbufs = u_bit_consecutive(0, 8);
		ctx->framebuffer.dirty_zsbuf = true;
	}
	/* 这应该始终标记为脏，以至少设置帧缓冲裁剪 */
	si_mark_atom_dirty(ctx, &ctx->atoms.s.framebuffer);

	si_mark_atom_dirty(ctx, &ctx->atoms.s.clip_regs);
	/* CLEAR_STATE设置为零 */
	if (!has_clear_state || ctx->clip_state.any_nonzeros)
		si_mark_atom_dirty(ctx, &ctx->atoms.s.clip_state);
	ctx->sample_locs_num_samples = 0;
	si_mark_atom_dirty(ctx, &ctx->atoms.s.msaa_sample_locs);
	si_mark_atom_dirty(ctx, &ctx->atoms.s.msaa_config);
	/* CLEAR_STATE设置为0xffff */
	if (!has_clear_state || ctx->sample_mask != 0xffff)
		si_mark_atom_dirty(ctx, &ctx->atoms.s.sample_mask);
	si_mark_atom_dirty(ctx, &ctx->atoms.s.cb_render_state);
	/* CLEAR_STATE设置为零 */
	if (!has_clear_state || ctx->blend_color.any_nonzeros)
		si_mark_atom_dirty(ctx, &ctx->atoms.s.blend_color);
	/* CLEAR_STATE禁用所有窗口矩形 */
	if (!has_clear_state || ctx->num_window_rectangles > 0)
		si_mark_atom_dirty(ctx, &ctx->atoms.s.window_rectangles);
	si_all_descriptors_begin_new_cs(ctx);
	si_all_resident_buffers_begin_new_cs(ctx);
	si_mark_atom_dirty(ctx, &ctx->atoms.s.scratch_state);
	if (ctx->scratch_buffer) {
		si_context_add_resource_size(ctx, &ctx->scratch_buffer->b.b);
	}

	if (ctx->streamout.suspended) {
		ctx->streamout.append_bitmask = ctx->streamout.enabled_mask;
		si_streamout_buffers_dirty(ctx);
	}

	if (!LIST_IS_EMPTY(&ctx->active_queries))
		si_resume_queries(ctx);

	// 断言，确保图形命令流的上一个双字为空
	assert(!ctx->gfx_cs->prev_dw);
	ctx->initial_gfx_cs_size = ctx->gfx_cs->current.cdw;

	/* 使各种绘图状态无效，以确保它们在第一个绘图调用之前发射 */
	si_invalidate_draw_sh_constants(ctx);
	ctx->last_index_size = -1;
	ctx->last_primitive_restart_en = -1;
	ctx->last_restart_index = SI_RESTART_INDEX_UNKNOWN;

	// 重置着色器状态初始化标志
	ctx->cs_shader_state.initialized = false;

	// 如果有清除状态，设置某些受跟踪的寄存器的值
	if (has_clear_state) {
		ctx->tracked_regs.reg_value[SI_TRACKED_DB_RENDER_CONTROL] = 0x00000000;
		ctx->tracked_regs.reg_value[SI_TRACKED_VGT_VERTEX_REUSE_BLOCK_CNTL] = 0x0000001e; /* 来自VI（Volcanic Islands） */

		// 将所有受跟踪的寄存器状态设置为已保存
		ctx->tracked_regs.reg_saved = 0xffffffffffffffff;
	} else {
		// 将所有受跟踪的寄存器状态设置为未知
		ctx->tracked_regs.reg_saved = 0;
	}

	/* 0xffffffff是SPI_PS_INPUT_CNTL_n寄存器的不可能值 */
	memset(ctx->tracked_regs.spi_ps_input_cntl, 0xff, sizeof(uint32_t) * 32);
}

```


## 调试

```

static void si_begin_gfx_cs_debug(struct si_context *ctx)
	ctx->current_saved_cs = calloc(1, sizeof(*ctx->current_saved_cs));
	ctx->current_saved_cs->trace_buf = r600_resource(
		pipe_buffer_create(ctx->b.screen, 0, PIPE_USAGE_STAGING, 8));

```
