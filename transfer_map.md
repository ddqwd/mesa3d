


transfer_map 用于获取bo的映射地址,然后写入数据

#  传输标志

从实现看，在pipe OpenGL 层定义了一套传输标志,大多与MapBuffer映射有关， 驱动根据这些控制自家GPU内存处理以及优化

```c


/* 这些是传递给驱动程序的传输标志。 */
/* 永远不要推断是否安全使用不同步的映射： */
#define TC_TRANSFER_MAP_NO_INFER_UNSYNCHRONIZED (1u << 29)
/* 不要使缓冲区无效： */
#define TC_TRANSFER_MAP_NO_INVALIDATE (1u << 30)
/* transfer_map 是从非驱动程序线程调用的： */
#define TC_TRANSFER_MAP_THREADED_UNSYNC (1u << 31)

/**
 * 传输对象使用标志
 */
enum pipe_transfer_usage
{
   /**
    * 在传输创建时读取资源内容（或直接访问）。 */
   PIPE_TRANSFER_READ = (1 << 0),
   
   /**
    * 在传输取消映射时将资源内容写回（或因直接访问而修改）。
    */
   PIPE_TRANSFER_WRITE = (1 << 1),

   /**
    * 读取/修改/写入
    */
   PIPE_TRANSFER_READ_WRITE = PIPE_TRANSFER_READ | PIPE_TRANSFER_WRITE,

   /** 
    * 传输应直接映射纹理存储。如果这不可能，驱动程序可能返回NULL，状态跟踪器需要处理并使用不带此标志的替代路径。
    *
    * 例如，状态跟踪器可以有一个简化的路径，直接映射纹理并在其上执行读取/修改/写入操作，还可以有一个更复杂的路径，使用最小的读取和写入传输。
    */
   PIPE_TRANSFER_MAP_DIRECTLY = (1 << 2),

   /**
    * 丢弃映射区域内的内存。
    *
    * 不应与PIPE_TRANSFER_READ一起使用。
    *
    * 参考：
    * - OpenGL的ARB_map_buffer_range扩展，MAP_INVALIDATE_RANGE_BIT标志。
    */
   PIPE_TRANSFER_DISCARD_RANGE = (1 << 8),

   /**
    * 如果资源不能立即映射，则失败。
    *
    * 参考：
    * - Direct3D的D3DLOCK_DONOTWAIT标志。
    * - Mesa的MESA_MAP_NOWAIT_BIT标志。
    * - WDDM的D3DDDICB_LOCKFLAGS.DonotWait标志。
    */
   PIPE_TRANSFER_DONTBLOCK = (1 << 9),

   /**
    * 映射时不尝试同步待处理的操作。
    *
    * 不应与PIPE_TRANSFER_READ一起使用。
    *
    * 参考：
    * - OpenGL的ARB_map_buffer_range扩展，MAP_UNSYNCHRONIZED_BIT标志。
    * - Direct3D的D3DLOCK_NOOVERWRITE标志。
    * - WDDM的D3DDDICB_LOCKFLAGS.IgnoreSync标志。
    */
   PIPE_TRANSFER_UNSYNCHRONIZED = (1 << 10),

   /**
    * 写入范围将稍后通过pipe_context::transfer_flush_region通知。
    *
    * 不应与PIPE_TRANSFER_READ一起使用。
    *
    * 参考：
    * - pipe_context::transfer_flush_region
    * - OpenGL的ARB_map_buffer_range扩展，MAP_FLUSH_EXPLICIT_BIT标志。
    */
   PIPE_TRANSFER_FLUSH_EXPLICIT = (1 << 11),

   /**
    * 丢弃支持资源的所有内存。
    *
    * 不应与PIPE_TRANSFER_READ一起使用。
    *
    * 这相当于：
    * - OpenGL的ARB_map_buffer_range扩展，MAP_INVALIDATE_BUFFER_BIT
    * - GL缓冲区上的BufferData(NULL)
    * - Direct3D的D3DLOCK_DISCARD标志。
    * - WDDM的D3DDDICB_LOCKFLAGS.Discard标志。
    * - D3D10 DDI的D3D10_DDI_MAP_WRITE_DISCARD标志
    * - D3D10的D3D10_MAP_WRITE_DISCARD标志。
    */
   PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE = (1 << 12),

   /**
    * 允许在映射时用于渲染资源。
    *
    * 创建资源时必须设置PIPE_RESOURCE_FLAG_MAP_PERSISTENT。
    *
    * 如果未设置COHERENT，必须调用memory_barrier(PIPE_BARRIER_MAPPED_BUFFER)以确保设备能够看到CPU写入的内容。
    */
   PIPE_TRANSFER_PERSISTENT = (1 << 13),

   /**
    * 如果设置PERSISTENT，则确保设备执行的任何写入立即对CPU可见，反之亦然。
    *
    * 创建资源时必须设置PIPE_RESOURCE_FLAG_MAP_COHERENT。
    */
   PIPE_TRANSFER_COHERENT = (1 << 14)
};

```

# resource / transfer class


传输的来源是数据，数据结构在每个阶段的处理不同达到封装目的.

通过逐步继承，纹理-> 缓冲(r600_resource)->多线程资源->pipe层

每次传输用一个si_transfer表示

threaded_resource 开启tc时使用，除了vtbl寻表，其他成员过渡

```
si_texture.buffer -> r600_resource.b -> threaded_resource.b -> pipe_resource


struct si_transfer {
	struct threaded_transfer	b;
	struct r600_resource		*staging;
	unsigned			offset;
};

// pipe_transfer.resource 指向资源
si_transfer.b -> threaded_transfer.b ->  pipe_transfer.resource -> pipe_resource*

```

# 使用接口


mesa内部为transfer_map定义了u_resource_vtbl这套虚表接口， 如此可对不同资源类型进行传输实现控制

according to convention ,纹理对象创建(si_texture_create_object )或缓冲 创建(si_alloc_buffer_struct) 将之初始化内部处理接口

完成`pipe->stack_tracker->driver `

```
/* Useful helper to allow >1 implementation of resource functionality
 * to exist in a single driver.  This is intended to be transitionary!
 */
struct u_resource_vtbl {

   boolean (*resource_get_handle)(struct pipe_screen *,
                                  struct pipe_resource *tex,
                                  struct winsys_handle *handle);

   void (*resource_destroy)(struct pipe_screen *,
                            struct pipe_resource *pt);

   void *(*transfer_map)(struct pipe_context *,
                         struct pipe_resource *resource,
                         unsigned level,
                         unsigned usage,
                         const struct pipe_box *,
                         struct pipe_transfer **);


   void (*transfer_flush_region)( struct pipe_context *,
                                  struct pipe_transfer *transfer,
                                  const struct pipe_box *);

   void (*transfer_unmap)( struct pipe_context *,
                           struct pipe_transfer *transfer );
};



static const struct u_resource_vtbl si_buffer_vtbl =
{
	NULL,				/* get_handle */
	si_buffer_destroy,		/* resource_destroy */
	si_buffer_transfer_map,	/* transfer_map */
	si_buffer_flush_region,	/* transfer_flush_region */
	si_buffer_transfer_unmap,	/* transfer_unmap */
};



static const struct u_resource_vtbl si_texture_vtbl =
{
	NULL,				/* get_handle */
	si_texture_destroy,		/* resource_destroy */
	si_texture_transfer_map,	/* transfer_map */
	u_default_transfer_flush_region, /* transfer_flush_region */
	si_texture_transfer_unmap,	/* transfer_unmap */
};


```


# pipe_box


作为传参表示数据具体区域

box意指天空盒？还是同tuple??

```c

/**
 * Subregion of 1D/2D/3D image resource.
 */
struct pipe_box
{
   /* Fields only used by textures use int16_t instead of int.
    * x and width are used by buffers, so they need the full 32-bit range.
    */
   int x;   // 缓冲offset
   int16_t y;
   int16_t z;
   int width; // buffer size
   int16_t height;
   int16_t depth;
};


```


si中资源有纹理和缓冲， 纹理最终使用缓冲映射

# 纹理

## 映射

```c
/* 
 * 在SI图形驱动中映射纹理资源的函数。
 */
static void *si_texture_transfer_map(struct pipe_context *ctx,
                                     struct pipe_resource *texture,
                                     unsigned level,
                                     unsigned usage,
                                     const struct pipe_box *box,
                                     struct pipe_transfer **ptransfer)
{
    struct si_context *sctx = (struct si_context*)ctx;
    struct si_texture *tex = (struct si_texture*)texture;
    struct si_transfer *trans;
    struct r600_resource *buf;
    unsigned offset = 0;
    char *map;
    bool use_staging_texture = false;

    /* 确保纹理资源的标志中没有使用转移标志。 */
    assert(!(texture->flags & SI_RESOURCE_FLAG_TRANSFER));
    /* 确保传入的盒子参数的尺寸是有效的。 */
    assert(box->width && box->height && box->depth);

    /* 深度纹理使用临时缓冲区。 */
    if (!tex->is_depth) {
        /* 在APU上，如果传输太频繁，降低瓦片模式。在dGPU上，临时缓冲区始终更快。 */
        if (!sctx->screen->info.has_dedicated_vram &&
            level == 0 &&
            box->width >= 4 && box->height >= 4 &&
            p_atomic_inc_return(&tex->num_level0_transfers) == 10) {
            bool can_invalidate =
                si_can_invalidate_texture(sctx->screen, tex, usage, box);

            si_reallocate_texture_inplace(sctx, tex,
                                          PIPE_BIND_LINEAR,
                                          can_invalidate);
        }

        /* 对于CPU访问，需要将瓦片纹理转换为线性纹理。临时缓冲区始终是线性的，并放置在GART中。 */
        if (!tex->surface.is_linear)
            use_staging_texture = true;
        /* 从VRAM或GTT WC读取速度较慢，因此在这种情况下始终使用临时缓冲区。 */
        else if (usage & PIPE_TRANSFER_READ)
            use_staging_texture =
                tex->buffer.domains & RADEON_DOMAIN_VRAM ||
                tex->buffer.flags & RADEON_FLAG_GTT_WC;
        /* 仅写入和线性的情况下： */
        else if (si_rings_is_buffer_referenced(sctx, tex->buffer.buf,
                                               RADEON_USAGE_READWRITE) ||
                 !sctx->ws->buffer_wait(tex->buffer.buf, 0,
                                        RADEON_USAGE_READWRITE)) {
            /* 缓冲区繁忙。 */
            if (si_can_invalidate_texture(sctx->screen, tex,
                                          usage, box))
                si_texture_invalidate_storage(sctx, tex);
            else
                use_staging_texture = true;
        }
    }

    /* 分配传输结构并设置基本信息。 */
    trans = CALLOC_STRUCT(si_transfer);
    if (!trans)
        return NULL;
    pipe_resource_reference(&trans->b.b.resource, texture);
    trans->b.b.level = level;
    trans->b.b.usage = usage;
    trans->b.b.box = *box;

    /* 对于深度纹理，进行特殊处理。 */
    if (tex->is_depth) {
        struct si_texture *staging_depth;

        if (tex->buffer.b.b.nr_samples > 1) {
            /* MSAA深度缓冲区需要转换为单样本缓冲区。
             *
             * 当使用多采样的GLX可视化（visual）调用ReadPixels时，可能需要映射MSAA深度缓冲区。
             *
             * 首先将深度缓冲区降采样到一个临时纹理，
             * 然后将临时纹理解压缩到staging纹理。
             *
             * 只有正在映射的区域被传输。
             */

            struct pipe_resource resource;

            si_init_temp_resource_from_box(&resource, texture, box, level, 0);

            // 内部创建一个临时纹理，将resource的数据复制到staging_depth
            if (!si_init_flushed_depth_texture(ctx, &resource, &staging_depth)) {
                PRINT_ERR("failed to create temporary texture to hold untiled copy\n");
                goto fail_trans;
            }

            if (usage & PIPE_TRANSFER_READ) {
                struct pipe_resource *temp = ctx->screen->resource_create(ctx->screen, &resource);
                if (!temp) {
                    PRINT_ERR("failed to create a temporary depth texture\n");
                    goto fail_trans;
                }

                si_copy_region_with_blit(ctx, temp, 0, 0, 0, 0, texture, level, box);
                si_blit_decompress_depth(ctx, (struct si_texture*)temp, staging_depth,
                                         0, 0, 0, box->depth, 0, 0);
                pipe_resource_reference(&temp, NULL);
            }

            /* 获取步长。 */
            si_texture_get_offset(sctx->screen, staging_depth, level, NULL,
                                  &trans->b.b.stride,
                                  &trans->b.b.layer_stride);
        } else {
            /* 深度纹理，对矩形进行读回？ */
            if (!si_init_flushed_depth_texture(ctx, texture, &staging_depth)) {
                PRINT_ERR("failed to create temporary texture to hold untiled copy\n");
                goto fail_trans;
            }

            si_blit_decompress_depth(ctx, tex, staging_depth,
                                     level, level,
                                     box->z, box->z + box->depth - 1,
                                     0, 0);

            offset = si_texture_get_offset(sctx->screen, staging_depth,
                                           level, box,
                                           &trans->b.b.stride,
                                           &trans->b.b.layer_stride);
        }

        trans->staging = &staging_depth->buffer;
        buf = trans->staging;
    } else if (use_staging_texture) {
        /* 对于需要使用临时缓冲区的情况。 */
        struct pipe_resource resource;
        struct si_texture *staging;

        si_init_temp_resource_from_box(&resource, texture, box, level,
                                       SI_RESOURCE_FLAG_TRANSFER);
        resource.usage = (usage & PIPE_TRANSFER_READ) ?
            PIPE_USAGE_STAGING : PIPE_USAGE_STREAM;

        /* 创建临时纹理。*/
        staging = (struct si_texture*)ctx->screen->resource_create(ctx->screen, &resource);
        if (!staging) {
            PRINT_ERR("failed to create temporary texture to hold untiled copy\n");
            goto fail_trans;
        }
        trans->staging = &staging->buffer;

        /* 获取步长。 */
        si_texture_get_offset(sctx->screen, staging, 0, NULL,
                              &trans->b.b.stride,
                              &trans->b.b.layer_stride);

        /* 如果是读取操作，则将数据复制到临时纹理中。 */
        if (usage & PIPE_TRANSFER_READ)
            si_copy_to_staging_texture(ctx, trans);
        else
            usage |= PIPE_TRANSFER_UNSYNCHRONIZED;

        buf = trans->staging;
    } else {
        /* 直接映射底层纹理。 */
        offset = si_texture_get_offset(sctx->screen, tex, level, box,
                                       &trans->b.b.stride,
                                       &trans->b.b.layer_stride);
        buf = &tex->buffer;
    }

    /* 使用环形缓冲区同步映射缓冲区。 */
    if (!(map = si_buffer_map_sync_with_rings(sctx, buf, usage)))
        goto fail_trans;

    /* 返回映射后的指针。 */
    *ptransfer = &trans->b.b;
    return map + offset;

fail_trans:
    /* 失败处理。 */
    r600_resource_reference(&trans->staging, NULL);
    pipe_resource_reference(&trans->b.b.resource, NULL);
    FREE(trans);
    return NULL;
}


```


* "Tile mode"（瓦片模式）是一种纹理或帧缓冲存储的方式。在图形渲染中，通常有两种主要的存储模式：线性模式和瓦片模式。

> 1. **线性模式：** 数据被按照线性地址排列，也就是说，所有的像素按照一定的顺序依次存储。这种模式的优点是对顺序读取和写入操作来说比较高效，但对于随机读取或写入来说效率较低。
> 
> 2. **瓦片模式：** 数据被分割成小块（瓦片），每个瓦片按照自己的地址空间存储。这种模式的优点是可以更好地利用缓存，尤其是对于图形渲染中常见的大量相邻像素操作，如纹理采样和渲染目标的读写。由于瓦片之间的距离相对较小，它更适合并行操作。
> 

* [ADM APU](https://en.wikipedia.org/wiki/AMD_APU) 
> AMD 加速处理单元( APU )，以前称为Fusion ，是Advanced Micro Devices (AMD)的一系列 64 位微处理器，结合了通用AMD64中央处理单元 ( CPU ) 和 3D集成图形处理单元(IGPU)在单个芯片上。

* 暂存纹理指使用一个临时缓冲作为cpu，gpu传输媒介， 这个缓冲作为一个颜色附件存在
* *深度纹理使用暂存纹理,具体原因不祥*
* *深度纹理多采样缓冲会将转换成单采样缓冲*
* 深度纹理申请bo之后， 会将之前的纹理数据通过blit(blitter或者dma) 拷贝的方式拷贝到staging buffer中
* 对于使用staging buffer的普通纹理写入的bo地址为staging buf = si_transfer.staging
* 用法存疑较多记录日后继续分析



```c
void si_blit_decompress_depth(struct pipe_context *ctx,
			      struct si_texture *texture,
			      struct si_texture *staging,
			      unsigned first_level, unsigned last_level,
			      unsigned first_layer, unsigned last_layer,
			      unsigned first_sample, unsigned last_sample)
{
	si_blit_dbcb_copy( ...)
}


static unsigned
si_blit_dbcb_copy(struct si_context *sctx,
		  struct si_texture *src,
		  struct si_texture *dst,
		  unsigned planes, unsigned level_mask,
		  unsigned first_layer, unsigned last_layer,
		  unsigned first_sample, unsigned last_sample)
{
	struct pipe_surface surf_tmpl = {{0}};
	unsigned layer, sample, checked_last_layer, max_layer;
	unsigned fully_copied_levels = 0;

	// 如果需要深度复制，启用深度复制标志
	if (planes & PIPE_MASK_Z)
		sctx->dbcb_depth_copy_enabled = true;
	// 如果需要模板复制，启用模板复制标志
	if (planes & PIPE_MASK_S)
		sctx->dbcb_stencil_copy_enabled = true;
	// 标记深度/模板复制状态为脏，以便后续渲染阶段重新配置
	si_mark_atom_dirty(sctx, &sctx->atoms.s.db_render_state);

	// 确保深度/模板复制已启用
	assert(sctx->dbcb_depth_copy_enabled || sctx->dbcb_stencil_copy_enabled);

	// 启用解压缩
	sctx->decompression_enabled = true;

	// 对每个需要复制的层级进行循环
	while (level_mask) {
		unsigned level = u_bit_scan(&level_mask);

		// 对于3D纹理，较小的mipmap级别有较少的层级
		max_layer = util_max_layer(&src->buffer.b.b, level);
		checked_last_layer = MIN2(last_layer, max_layer);

		surf_tmpl.u.tex.level = level;

		// 对每个层级进行循环
		for (layer = first_layer; layer <= checked_last_layer; layer++) {
			struct pipe_surface *zsurf, *cbsurf;

			// 创建源纹理深度表面
			surf_tmpl.format = src->buffer.b.b.format;
			surf_tmpl.u.tex.first_layer = layer;
			surf_tmpl.u.tex.last_layer = layer;
			zsurf = sctx->b.create_surface(&sctx->b, &src->buffer.b.b, &surf_tmpl);

			// 创建目标纹理表面
			surf_tmpl.format = dst->buffer.b.b.format;
			cbsurf = sctx->b.create_surface(&sctx->b, &dst->buffer.b.b, &surf_tmpl);

			// 对每个采样进行循环
			for (sample = first_sample; sample <= last_sample; sample++) {
				// 如果当前采样与之前的不同，更新渲染状态
				if (sample != sctx->dbcb_copy_sample) {
					sctx->dbcb_copy_sample = sample;
					si_mark_atom_dirty(sctx, &sctx->atoms.s.db_render_state);
				}

				// 使用blitter开始深度/模板复制
				si_blitter_begin(sctx, SI_DECOMPRESS);
				// 使用blitter执行深度/模板复制操作
				util_blitter_custom_depth_stencil(sctx->blitter, zsurf, cbsurf, 1 << sample,
								  sctx->custom_dsa_flush, 1.0f);
				// 使用blitter结束深度/模板复制
				si_blitter_end(sctx);
			}

			// 释放深度和模板表面
			pipe_surface_reference(&zsurf, NULL);
			pipe_surface_reference(&cbsurf, NULL);
		}

		// 如果复制涵盖了整个层级，更新标志
		if (first_layer == 0 && last_layer >= max_layer &&
		    first_sample == 0 && last_sample >= u_max_sample(&src->buffer.b.b))
			fully_copied_levels |= 1u << level;
	}

	// 禁用解压缩和深度/模板复制标志
	sctx->decompression_enabled = false;
	sctx->dbcb_depth_copy_enabled = false;
	sctx->dbcb_stencil_copy_enabled = false;
	// 标记深度/模板渲染状态为脏
	si_mark_atom_dirty(sctx, &sctx->atoms.s.db_render_state);

	// 返回已完全复制的层级标志
	return fully_copied_levels;
}

```
* util_blitter_custom_depth_stencil和util_blitter_clear 原理相同通过draw一个矩形表面，不同的是构造的fs内部只写入一个颜色缓冲,用于保存深度值
* si_blit_dbcb_copy 开启深度缓冲的渲染拷贝控制，通过控制这个可将深度纹理数据复制到颜色缓冲中

```c
static void si_emit_db_render_state(struct si_context *sctx)
{
	struct si_state_rasterizer *rs = sctx->queued.named.rasterizer;
	unsigned db_shader_control, db_render_control, db_count_control;
	unsigned initial_cdw = sctx->gfx_cs->current.cdw;

	/* DB_RENDER_CONTROL */
	if (sctx->dbcb_depth_copy_enabled ||
	    sctx->dbcb_stencil_copy_enabled) {
		db_render_control =
			S_028000_DEPTH_COPY(sctx->dbcb_depth_copy_enabled) |
			S_028000_STENCIL_COPY(sctx->dbcb_stencil_copy_enabled) |
			S_028000_COPY_CENTROID(1) |
			S_028000_COPY_SAMPLE(sctx->dbcb_copy_sample);
	} else {...}

	radeon_opt_set_context_reg2(sctx, R_028000_DB_RENDER_CONTROL,
				    SI_TRACKED_DB_RENDER_CONTROL, db_render_control,
				    db_count_control);

    ...

```


* 通过DB_RENDER_CONTROL 该寄存器即可启用深度扩展

DB:DB_RENDER_CONTROL 寄存器是一个可读写的 32 位寄存器，用于控制深度和模板测试等渲染操作。该寄存器的地址是 0x28000。


| 字段名称              | 位范围 | 默认值 | 描述                                                                                           |
|----------------------|--------|--------|--------------------------------------------------------------------------------------------------|
| DEPTH_CLEAR_ENABLE    | 0      | none   | 启用深度缓冲区的清除操作，将深度值清除为指定的 Clear Value。                                       |
| STENCIL_CLEAR_ENABLE  | 1      | none   | 启用模板缓冲区的清除操作，将模板值清除为指定的 Clear Value。                                      |
| DEPTH_COPY            | 2      | none   | 启用深度值的扩展，将其复制到颜色渲染目标 0。必须在颜色缓冲区（CB）中设置所需的目标格式。             |
| STENCIL_COPY          | 3      | none   | 启用模板值的扩展，将其复制到颜色渲染目标 0。必须在颜色缓冲区（CB）中设置所需的目标格式。          |
| RESUMMARIZE_ENABLE    | 4      | none   | 如果设置，所有受影响的瓦片将更新 HTILE 表面信息。                                                 |
| STENCIL_COMPRESS_DISABLE | 5   | none   | 强制在任何渲染的瓦片上禁用层次剔除的模板值解压缩。                                          |
| DEPTH_COMPRESS_DISABLE   | 6   | none   | 强制在任何渲染的瓦片上禁用层次剔除的深度值解压缩。                                          |
| COPY_CENTROID          | 7      | none   | 如果设置，从像素内的第一个亮点样本开始复制，从 COPY_SAMPLE 开始（循环回到较低的样本）。如果 COPY_CENTROID==0 并且启用了深度或模板写入（在生产驱动程序中不会发>生），则必须设置 DB_RENDER_OVERRIDE.FORCE_QC_SMASK_CONFLICT。此外，在执行深度或模板复制并且启用 ps_iter 时，必须将 COPY_CENTROID 设置为 1。 |
| COPY_SAMPLE            | 11:8   | none   | 如果 COPY_CENTROID 为真，则从此样本编号开始复制第一个亮点样本。否则，不管是否亮点样本都复制此样本。       |

## 解映射

```c
static void si_texture_transfer_unmap(struct pipe_context *ctx,
                                      struct pipe_transfer* transfer)
{
    struct si_context *sctx = (struct si_context*)ctx;
    struct si_transfer *stransfer = (struct si_transfer*)transfer; struct pipe_resource *texture = transfer->resource;
    struct si_texture *tex = (struct si_texture*)texture;

    // 如果是写入操作，并且有临时缓冲区，则执行拷贝
    if ((transfer->usage & PIPE_TRANSFER_WRITE) && stransfer->staging) {
        if (tex->is_depth && tex->buffer.b.b.nr_samples <= 1) {
            // 如果是深度纹理并且采样数不大于1，则执行区域拷贝
            ctx->resource_copy_region(ctx, texture, transfer->level,
                                      transfer->box.x, transfer->box.y, transfer->box.z,
                                      &stransfer->staging->b.b, transfer->level,
                                      &transfer->box);
        } else {
            // 否则，使用自定义的函数从临时缓冲区拷贝
            si_copy_from_staging_texture(ctx, stransfer);
        }
    }

    // 释放临时缓冲区
    if (stransfer->staging) {
        sctx->num_alloc_tex_transfer_bytes += stransfer->staging->buf->size;
        r600_resource_reference(&stransfer->staging, NULL);
    }

    /* 启发式策略用于 {上传，绘制，上传，绘制，..} 操作：
     *
     * 如果分配的纹理存储过多，刷新图形IB。
     *
     * 这个想法是我们不希望构建使用太多内存的IB，
     * 否则会对内核内存管理器造成压力，同时我们还想让
     * 临时和失效的缓冲区尽早进入空闲状态，以减少总内存
     * 使用量或者使它们可重用。由于winsys中有缓冲区
     * 缓存，因此实际内存使用会稍微高一些。
     *
     * 结果是内核内存管理器永远不会成为瓶颈。
     */
    if (sctx->num_alloc_tex_transfer_bytes > sctx->screen->info.gart_size / 4) {
        si_flush_gfx_cs(sctx, RADEON_FLUSH_ASYNC_START_NEXT_GFX_IB_NOW, NULL);
        sctx->num_alloc_tex_transfer_bytes = 0;
    }

    // 释放传输资源
    pipe_resource_reference(&transfer->resource, NULL);
    FREE(transfer);
}


static void si_copy_from_staging_texture(struct pipe_context *ctx, struct si_transfer *stransfer)
	u_box_3d(0, 0, 0, transfer->box.width, transfer->box.height, transfer->box.depth, &sbox);

	if (dst->nr_samples > 1) {
		si_copy_region_with_blit(ctx, dst, transfer->level,
					   transfer->box.x, transfer->box.y, transfer->box.z,
					   src, 0, &sbox);
-----------------------------------------------------------
		    pipe->blit(pipe, &blit);
                si_blit
		            sctx->dma_copy(ctx, info->dst.resource, info->dst.level,
	                util_blitter_blit(sctx->blitter, info);
            
		return;
	}

	sctx->dma_copy(ctx, dst, transfer->level,
		       transfer->box.x, transfer->box.y, transfer->box.z,
		       src, 0, &sbox);

```


* 不断通过map /unmap blitter或者dma_copy这类高效复制数据的方式达到高效传输的目的？？


# 缓冲

## 映射

```c
static void *si_buffer_transfer_map(struct pipe_context *ctx,
				    struct pipe_resource *resource,
				    unsigned level,
				    unsigned usage,
				    const struct pipe_box *box,
				    struct pipe_transfer **ptransfer)
{
	struct si_context *sctx = (struct si_context*)ctx;
	struct r600_resource *rbuffer = r600_resource(resource);
	uint8_t *data;

	assert(box->x + box->width <= resource->width0);

	/* 根据 GL_AMD_pinned_memory 问题的解决方案：
	 *
	 *     4) 在共享缓冲区上调用 glMapBuffer 是否保证返回在创建时指定的
	 *        相同系统地址？
	 *
	 *        答案：不保证。GL 实现可能返回内存的不同虚拟映射，尽管会使用
	 *        相同的物理页。
	 *
	 * 因此，永远不要使用临时缓冲区。*/
	if (rbuffer->b.is_user_ptr)
		usage |= PIPE_TRANSFER_PERSISTENT;


    /* 查看要映射的缓冲区范围是否已经初始化，
       在这种情况下，可以无需同步地进行映射。 */
    
    /* 如果使用标志中不包含 PIPE_TRANSFER_UNSYNCHRONIZED 或
       TC_TRANSFER_MAP_NO_INFER_UNSYNCHRONIZED，并且
       使用标志包含 PIPE_TRANSFER_WRITE，并且
       渲染缓冲区不是共享的，并且
       渲染缓冲区的有效范围与指定的 box 不相交，则执行下面的操作。 */
    if (!(usage & (PIPE_TRANSFER_UNSYNCHRONIZED |
                   TC_TRANSFER_MAP_NO_INFER_UNSYNCHRONIZED)) &&
        usage & PIPE_TRANSFER_WRITE &&
        !rbuffer->b.is_shared &&
        !util_ranges_intersect(&rbuffer->valid_buffer_range, box->x, box->x + box->width)) {
        /* 设置使用标志，表明可以无需同步地进行映射。 */
        usage |= PIPE_TRANSFER_UNSYNCHRONIZED;
    }


	/* 如果丢弃整个范围，检查范围如果是整个缓冲的大小，则丢弃整个资源。*/
	if (usage & PIPE_TRANSFER_DISCARD_RANGE &&
	    box->x == 0 && box->width == resource->width0) {
		usage |= PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE;
	}

	/* 如果 VRAM 中的缓冲区太大并且范围被丢弃，则不直接映射。
	 * 这确保了缓冲区保持在 VRAM 中。 */
	bool force_discard_range = false;
	if (usage & (PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE |
		     PIPE_TRANSFER_DISCARD_RANGE) &&
	    !(usage & PIPE_TRANSFER_PERSISTENT) &&
	    rbuffer->max_forced_staging_uploads > 0 &&
	    p_atomic_dec_return(&rbuffer->max_forced_staging_uploads) >= 0) {
		usage &= ~(PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE |
			   PIPE_TRANSFER_UNSYNCHRONIZED);
		usage |= PIPE_TRANSFER_DISCARD_RANGE;
		force_discard_range = true;
	}

	/* 如果丢弃整个资源并且不禁用无效化，确保缓冲区已经被无效化。 */
	if (usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE &&
	    !(usage & (PIPE_TRANSFER_UNSYNCHRONIZED |
		       TC_TRANSFER_MAP_NO_INVALIDATE))) {
		assert(usage & PIPE_TRANSFER_WRITE);

		if (si_invalidate_buffer(sctx, rbuffer)) {
			/* 在这一点上，缓冲区总是空闲的。*/
			usage |= PIPE_TRANSFER_UNSYNCHRONIZED;
		} else {
			/* 退回到临时缓冲区。*/
			usage |= PIPE_TRANSFER_DISCARD_RANGE;
		}
	}

	/* 如果需要丢弃范围，并且不禁用无效化，或者缓冲区是稀疏的，则执行下面的操作。 
    */ if ((usage & PIPE_TRANSFER_DISCARD_RANGE) && 
        ((!(usage & (PIPE_TRANSFER_UNSYNCHRONIZED | PIPE_TRANSFER_PERSISTENT)))
        || (rbuffer->flags & RADEON_FLAG_SPARSE))) {
		assert(usage & PIPE_TRANSFER_WRITE);

		/* 检查映射此缓冲区是否会导致等待 GPU。 */
		if (rbuffer->flags & RADEON_FLAG_SPARSE ||
		    force_discard_range ||
		    si_rings_is_buffer_referenced(sctx, rbuffer->buf, RADEON_USAGE_READWRITE) ||
		    !sctx->ws->buffer_wait(rbuffer->buf, 0, RADEON_USAGE_READWRITE)) {
			/* 使用临时缓冲区进行无等待写入传输。 */
			unsigned offset;
			struct r600_resource *staging = NULL;

			u_upload_alloc(ctx->stream_uploader, 0,
                                       box->width + (box->x % SI_MAP_BUFFER_ALIGNMENT),
				       sctx->screen->info.tcc_cache_line_size,
				       &offset, (struct pipe_resource**)&staging,
                                       (void**)&data);

			if (staging) {
				data += box->x % SI_MAP_BUFFER_ALIGNMENT;
				return si_buffer_get_transfer(ctx, resource, usage, box,
								ptransfer, data, staging, offset);
			} else if (rbuffer->flags & RADEON_FLAG_SPARSE) {
				return NULL;
			}
		} else {
			/* 在这一点上，缓冲区总是空闲的（我们在上面检查过了）。 */
			usage |= PIPE_TRANSFER_UNSYNCHRONIZED;
		}
	}
	/* 对于读取操作，使用在缓存的 GTT 中的临时缓冲区。 */
	else if (((usage & PIPE_TRANSFER_READ) &&
		  !(usage & PIPE_TRANSFER_PERSISTENT) &&
		  (rbuffer->domains & RADEON_DOMAIN_VRAM ||
		   rbuffer->flags & RADEON_FLAG_GTT_WC)) ||
		 (rbuffer->flags & RADEON_FLAG_SPARSE)) {
		struct r600_resource *staging;

		assert(!(usage & TC_TRANSFER_MAP_THREADED_UNSYNC));
		staging = r600_resource(pipe_buffer_create(
				ctx->screen, 0, PIPE_USAGE_STAGING,
				box->width + (box->x % SI_MAP_BUFFER_ALIGNMENT)));
		if (staging) {
			/* 将 VRAM 缓冲区复制到临时缓冲区。 */
			sctx->dma_copy(ctx, &staging->b.b, 0,
				       box->x % SI_MAP_BUFFER_ALIGNMENT,
				       0, 0, resource, 0, box);

			data = si_buffer_map_sync_with_rings(sctx, staging,
							     usage & ~PIPE_TRANSFER_UNSYNCHRONIZED);
			if (!data) {
				r600_resource_reference(&staging, NULL);
				return NULL;
			}
			data += box->x % SI_MAP_BUFFER_ALIGNMENT;

			return si_buffer_get_transfer(ctx, resource, usage, box,
							ptransfer, data, staging, 0);
		} else if (rbuffer->flags & RADEON_FLAG_SPARSE) {
			return NULL;
		}
	}

	/* 使用环形缓冲区中的同步映射缓冲区。 */
	data = si_buffer_map_sync_with_rings(sctx, rbuffer, usage);
	if (!data) {
		return NULL;
	}
	data += box->x;

	return si_buffer_get_transfer(ctx, resource, usage, box,
					ptransfer, data, NULL, 0);
}


void *si_buffer_map_sync_with_rings(struct si_context *sctx,
				    struct r600_resource *resource,
				    unsigned usage)
{

	bool busy = false;
    // 直接映射，不与其他事件同步
	if (usage & PIPE_TRANSFER_UNSYNCHRONIZED) {
		return sctx->ws->buffer_map(resource->buf, NULL, usage);
	}
    
    //  如果当前gfx_cs已经有ib写入，且此次传输的bo是ib所使用的bo
	if (radeon_emitted(sctx->gfx_cs, sctx->initial_gfx_cs_size) &&
	    sctx->ws->cs_is_buffer_referenced(sctx->gfx_cs,
						resource->buf, rusage)) {
        // 如果使用传输无阻塞标志立刻返回空
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			si_flush_gfx_cs(sctx, RADEON_FLUSH_ASYNC_START_NEXT_GFX_IB_NOW, NULL);
			return NULL;
		} else {
            // 否则将busy设置为真表示需要等待
			si_flush_gfx_cs(sctx, RADEON_FLUSH_ASYNC_START_NEXT_GFX_IB_NOW, NULL);
			busy = true;
		}
	}

    // 检查dma_cs 是否已经有ib写入， 并且用到此次bo，刷新dma_cs 等待释放
	if (radeon_emitted(sctx->dma_cs, 0) &&
	    sctx->ws->cs_is_buffer_referenced(sctx->dma_cs,
						resource->buf, rusage)) {
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			si_flush_dma_cs(sctx, PIPE_FLUSH_ASYNC, NULL);
			return NULL;
		} else {
			si_flush_dma_cs(sctx, 0, NULL);
			busy = true;
		}
	}

    // 如果需要等待， 无限等待
	if (busy || !sctx->ws->buffer_wait(resource->buf, 0, rusage)) {
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			return NULL;
		} else {
			/* We will be wait for the GPU. Wait for any offloaded
			 * CS flush to complete to avoid busy-waiting in the winsys. */
             // 通过cs->flush_completed fence等待之前的ib提交任务完成
			sctx->ws->cs_sync_flush(sctx->gfx_cs); if (sctx->dma_cs)
				sctx->ws->cs_sync_flush(sctx->dma_cs);
		}
	}

	/* Setting the CS to NULL will prevent doing checks we have done already. */
    // 已经作的就是flush_cs, sync_flush
	return sctx->ws->buffer_map(resource->buf, NULL, usage);


}
```

* si_buffer_transfer_map 首先检查了bo是否已经在使用，如果使用对gfx, dma 刷新流，之后根据丢弃范围传输表标志， 是否使用稀疏缓冲，是否是读取操作作了不同的优化传输策略处理
*  PIPE_TRANSFER_DISCARD_RANGE , PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE  are analogous to MapBufferRange's GL_MAP_INVALIDATE_RANGE_BIT，indicate映射之前的区域可以被丢弃， also see usage in st_MapTextureImage, st_MapRenderbuffer 


## 解映射

```c

static void si_buffer_transfer_unmap(struct pipe_context *ctx,
				     struct pipe_transfer *transfer)
	
	if (transfer->usage & PIPE_TRANSFER_WRITE &&
	    !(transfer->usage & PIPE_TRANSFER_FLUSH_EXPLICIT))
		si_buffer_do_flush_region(ctx, transfer, &transfer->box);
-----------------------------------------------------------------------
          	if (stransfer->staging) {
          		unsigned src_offset = stransfer->offset +
          				      transfer->box.x % SI_MAP_BUFFER_ALIGNMENT +
          				      (box->x - transfer->box.x);
          
          		/* Copy the staging buffer into the original one. */
          		si_copy_buffer((struct si_context*)ctx, transfer->resource,
          			       &stransfer->staging->b.b, box->x, src_offset,
          			       box->width);
          	}
------------------------------------------------------------------
	slab_free(&sctx->pool_transfers, transfer);

void si_copy_buffer(struct si_context *sctx,
                    struct pipe_resource *dst, struct pipe_resource *src,
                    uint64_t dst_offset, uint64_t src_offset, unsigned size)
{
    // 如果复制大小为0，直接返回
    if (!size)
        return;

    // 获取一致性和缓存策略
    enum si_coherency coher = SI_COHERENCY_SHADER;
    enum si_cache_policy cache_policy = get_cache_policy(sctx, coher, size);

    // 仅在dGPU上对VRAM进行大块复制时使用计算引擎
    if (sctx->screen->info.has_dedicated_vram &&
        r600_resource(dst)->domains & RADEON_DOMAIN_VRAM &&
        r600_resource(src)->domains & RADEON_DOMAIN_VRAM &&
        size > 32 * 1024 &&
        dst_offset % 4 == 0 && src_offset % 4 == 0 && size % 4 == 0) {
        // 使用计算引擎执行清除或复制
        si_compute_do_clear_or_copy(sctx, dst, dst_offset, src, src_offset,
                                    size, NULL, 0, coher);
    } else {
        // 否则使用CP/DMA引擎进行缓冲复制
        si_cp_dma_copy_buffer(sctx, dst, src, dst_offset, src_offset, size,
                              0, coher, cache_policy);
    }
}

```

* 对于临时缓冲解映射作了特殊处理(cs, dma) ,之后将该次传输使用的transfer加入内存池留待下次使用

# 调试

open gallium_ddebug 的transfer

get transfer log information by gallium_ddebug 

```

transfer_map:
  transfer: {resource = 0x555f68377ad0, level = 0, usage = PIPE_TRANSFER_WRITE|PIPE_TRANSFER_PERSISTENT, box = {x = 0, y = 0, z = 0, width = 512, height = 512, depth = 1, }, stride = 2048, layer_stride = 1048576, }
  transfer_ptr: 0x555f67edce20
  ptr: 0x7effb80f6000

```


# TODO

***需结合场景进一步分析***
