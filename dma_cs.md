
***Written by sj.yu***

# SDMA

* sDMA - 系统DMA
* 从CIK（Sea Islands）开始，GPU引入了新的异步DMA引擎。这些引擎用于计算和图形处理。有两个DMA引擎（SDMA0、SDMA1），每个引擎支持一个用于图形处理的环形缓冲区和两个用于计算的队列。
* 编程模型与CP（Command Processor）非常相似（包括环形缓冲区、IB等），但sDMA有自己的数据包格式，与CP使用的PM4格式不同。sDMA支持数据复制、写入嵌入式数据、实心填充以及许多其他操作。它还支持缓冲区的平铺/反铺操作。

dma_cs就是dma ib的命令流它和gfx_cs有着不同的命令形式

# sdma包的定义

针对不同的amdgpu架构，分为si和cik sdma

```c

// sid.h

/* SI异步DMA数据包 */
#define SI_DMA_PACKET(cmd, sub_cmd, n) ((((unsigned)(cmd) & 0xF) << 28) |    \
                                       (((unsigned)(sub_cmd) & 0xFF) << 20) |\
                                       (((unsigned)(n) & 0xFFFFF) << 0))
/* SI异步DMA数据包类型 */
#define SI_DMA_PACKET_WRITE                     0x2
#define SI_DMA_PACKET_COPY                      0x3
#define SI_DMA_COPY_MAX_BYTE_ALIGNED_SIZE       0xfffe0
/* 文档中指出0xffff8是以双字为单位的最大大小，即0x3fffe0字节。 */
#define SI_DMA_COPY_MAX_DWORD_ALIGNED_SIZE      0x3fffe0
#define SI_DMA_COPY_DWORD_ALIGNED               0x00
#define SI_DMA_COPY_BYTE_ALIGNED                0x40
#define SI_DMA_COPY_TILED                       0x8
#define SI_DMA_PACKET_INDIRECT_BUFFER           0x4
#define SI_DMA_PACKET_SEMAPHORE                 0x5
#define SI_DMA_PACKET_FENCE                     0x6
#define SI_DMA_PACKET_TRAP                      0x7
#define SI_DMA_PACKET_SRBM_WRITE                0x9
#define SI_DMA_PACKET_CONSTANT_FILL             0xd
#define SI_DMA_PACKET_NOP                       0xf

/* CIK异步DMA数据包 */
#define CIK_SDMA_PACKET(op, sub_op, n)   ((((unsigned)(n) & 0xFFFF) << 16) |	\
					 (((unsigned)(sub_op) & 0xFF) << 8) |	\
					 (((unsigned)(op) & 0xFF) << 0))
/* CIK异步DMA数据包类型 */
#define    CIK_SDMA_OPCODE_NOP                     0x0
#define    CIK_SDMA_OPCODE_COPY                    0x1
#define        CIK_SDMA_COPY_SUB_OPCODE_LINEAR            0x0
#define        CIK_SDMA_COPY_SUB_OPCODE_TILED             0x1
#define        CIK_SDMA_COPY_SUB_OPCODE_SOA               0x3
#define        CIK_SDMA_COPY_SUB_OPCODE_LINEAR_SUB_WINDOW 0x4
#define        CIK_SDMA_COPY_SUB_OPCODE_TILED_SUB_WINDOW  0x5
#define        CIK_SDMA_COPY_SUB_OPCODE_T2T_SUB_WINDOW    0x6
#define    CIK_SDMA_OPCODE_WRITE                   0x2
#define        SDMA_WRITE_SUB_OPCODE_LINEAR               0x0
#define        SDMA_WRTIE_SUB_OPCODE_TILED                0x1
#define    CIK_SDMA_OPCODE_INDIRECT_BUFFER         0x4
#define    CIK_SDMA_PACKET_FENCE                   0x5
#define    CIK_SDMA_PACKET_TRAP                    0x6
#define    CIK_SDMA_PACKET_SEMAPHORE               0x7
#define    CIK_SDMA_PACKET_CONSTANT_FILL           0xb
#define    CIK_SDMA_OPCODE_TIMESTAMP               0xd
#define        SDMA_TS_SUB_OPCODE_SET_LOCAL_TIMESTAMP     0x0
#define        SDMA_TS_SUB_OPCODE_GET_LOCAL_TIMESTAMP     0x1
#define        SDMA_TS_SUB_OPCODE_GET_GLOBAL_TIMESTAMP    0x2
#define    CIK_SDMA_PACKET_SRBM_WRITE              0xe
/* 有一个未记录的硬件“特性”阻止HW从(1 << 22)的256字节之后进行复制 */
#define    CIK_SDMA_COPY_MAX_SIZE                  0x3fff00

由于调试时使用的amdgpu为vi系列，使用的sdma为cik 

```
# DMA命令流上下文创建 


```c
static struct pipe_context *si_create_context(struct pipe_screen *screen,
                                              unsigned flags)
    ...

	if (sctx->chip_class >= CIK)
		cik_init_sdma_functions(sctx);
	else
		si_init_dma_functions(sctx);

    // num_sdma_rings 在winsys创建时通过ac_gpu_info获取gpu信息，no_async_dma即为nodma
	if (sscreen->info.num_sdma_rings && !(sscreen->debug_flags & DBG(NO_ASYNC_DMA))) {
		sctx->dma_cs = sctx->ws->cs_create(sctx->ctx, RING_DMA,
						       (void*)si_flush_dma_cs,
						       sctx);
	}
    ...
}
```
* 调试时所用chip为vi ，所以分析cik sdma
* 调用cs_create通过将ring_type 设置成RING_DMA，下发ib时，gpu即可识别出此次ib属于哪种ring类型


通过si_test_dma来分析测试si中的dma


设置环境变量   
R600_DEBUG=testdma


# DMA数据写入 

dma下发提供的接口是一个dma_copy接口

```c

void cik_init_sdma_functions(struct si_context *sctx)
{
	sctx->dma_copy = cik_sdma_copy;
}


static void cik_sdma_copy(struct pipe_context *ctx,
			  struct pipe_resource *dst,
			  unsigned dst_level,
			  unsigned dstx, unsigned dsty, unsigned dstz,
			  struct pipe_resource *src,
			  unsigned src_level,
			  const struct pipe_box *src_box)
{
    // 处理buffer
	if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
		cik_sdma_copy_buffer(sctx, dst, src, dstx, src_box->x, src_box->width);
		return;
	}
    // 处理纹理
	if ((sctx->chip_class == CIK || sctx->chip_class == VI) &&
	    cik_sdma_copy_texture(sctx, dst, dst_level, dstx, dsty, dstz,
				  src, src_level, src_box))
		return;

fallback:
    //使用bliter
	si_resource_copy_region(ctx, dst, dst_level, dstx, dsty, dstz,
				src, src_level, src_box);
}


```

##  缓冲

```c
static void cik_sdma_copy_buffer(struct si_context *ctx,
                                 struct pipe_resource *dst,
                                 struct pipe_resource *src,
                                 uint64_t dst_offset,
                                 uint64_t src_offset,
                                 uint64_t size)
{
    // 获取DMA命令流
    struct radeon_cmdbuf *cs = ctx->dma_cs;
    unsigned i, ncopy, csize;
    struct r600_resource *rdst = r600_resource(dst);
    struct r600_resource *rsrc = r600_resource(src);

    // 标记目标缓冲区范围为有效（已初始化），
    // 这样在映射该范围时，transfer_map就知道它应该等待GPU。
    util_range_add(&rdst->valid_buffer_range, dst_offset,
                   dst_offset + size);

    // 更新偏移以获得GPU地址
    dst_offset += rdst->gpu_address;
    src_offset += rsrc->gpu_address;

    // 计算拷贝次数
    ncopy = DIV_ROUND_UP(size, CIK_SDMA_COPY_MAX_SIZE);
    // 确保有足够的DMA空间
    si_need_dma_space(ctx, ncopy * 7, rdst, rsrc);

    // 遍历拷贝次数
    for (i = 0; i < ncopy; i++) {
        // 计算当前拷贝的大小
        csize = MIN2(size, CIK_SDMA_COPY_MAX_SIZE);
        // 发送SDMA拷贝命令
        radeon_emit(cs, CIK_SDMA_PACKET(CIK_SDMA_OPCODE_COPY,
                                        CIK_SDMA_COPY_SUB_OPCODE_LINEAR,
                                        0));
        radeon_emit(cs, ctx->chip_class >= GFX9 ? csize - 1 : csize);
        radeon_emit(cs, 0); /* src/dst endian swap */
        radeon_emit(cs, src_offset);
        radeon_emit(cs, src_offset >> 32);
        radeon_emit(cs, dst_offset);
        radeon_emit(cs, dst_offset >> 32);
        // 更新偏移和大小
        dst_offset += csize;
        src_offset += csize;
        size -= csize;
    }
}


void si_need_dma_space(struct si_context *ctx, unsigned num_dw,
                       struct r600_resource *dst, struct r600_resource *src)
{
    // 计算当前DMA命令流的VRAM和GTT使用量
    uint64_t vram = ctx->dma_cs->used_vram;
    uint64_t gtt = ctx->dma_cs->used_gart;

    // 如果目标缓冲存在，添加其VRAM和GTT使用量
    if (dst) {
        vram += dst->vram_usage;
        gtt += dst->gart_usage;
    }
    // 如果源缓冲存在，添加其VRAM和GTT使用量
    if (src) {
        vram += src->vram_usage;
        gtt += src->gart_usage;
    }

    // 如果DMA命令流中包含了GFX IB，并且该IB被DMA所依赖， // 则刷新GFX IB
    if (radeon_emitted(ctx->gfx_cs, ctx->initial_gfx_cs_size) &&
        ((dst &&
          ctx->ws->cs_is_buffer_referenced(ctx->gfx_cs, dst->buf,
                                           RADEON_USAGE_READWRITE)) ||
         (src &&
          ctx->ws->cs_is_buffer_referenced(ctx->gfx_cs, src->buf,
                                           RADEON_USAGE_WRITE))))
        si_flush_gfx_cs(ctx, RADEON_FLUSH_ASYNC_START_NEXT_GFX_IB_NOW, NULL);

    // 如果DMA空间不足，或者内存使用量超过限制，则刷新DMA命令流
    num_dw++; /* 用于下面的emit_wait_idle */
    if (!ctx->ws->cs_check_space(ctx->dma_cs, num_dw) ||
        ctx->dma_cs->used_vram + ctx->dma_cs->used_gart > 64 * 1024 * 1024 ||
        !radeon_cs_memory_below_limit(ctx->screen, ctx->dma_cs, vram, gtt)) {
        si_flush_dma_cs(ctx, PIPE_FLUSH_ASYNC, NULL);
        assert((num_dw + ctx->dma_cs->current.cdw) <= ctx->dma_cs->current.max_dw);
    }

    // 如果目标或源缓冲曾经在IB中被使用，则等待空闲
    if ((dst &&
         ctx->ws->cs_is_buffer_referenced(ctx->dma_cs, dst->buf,
                                          RADEON_USAGE_READWRITE)) ||
        (src &&
         ctx->ws->cs_is_buffer_referenced(ctx->dma_cs, src->buf,
                                          RADEON_USAGE_WRITE)))
        si_dma_emit_wait_idle(ctx);

    // 将目标和源缓冲添加到DMA命令流的buffer list中
    if (dst) {
        radeon_add_to_buffer_list(ctx, ctx->dma_cs, dst,
                                  RADEON_USAGE_WRITE, 0);
    }
    if (src) {
        radeon_add_to_buffer_list(ctx, ctx->dma_cs, src,
                                  RADEON_USAGE_READ, 0);
    }

    // 增加DMA调用计数
    ctx->num_dma_calls++;
}

static void si_dma_emit_wait_idle(struct si_context *sctx)
{
	struct radeon_cmdbuf *cs = sctx->dma_cs;

	/* NOP waits for idle. */
	if (sctx->chip_class >= CIK)
		radeon_emit(cs, 0x00000000); /* NOP */
	else
		radeon_emit(cs, 0xf0000000); /* NOP */
}
```


## 纹理


```c

// 检查是否需要为DMA传输准备空间
static bool cik_sdma_copy_texture(struct si_context *sctx,
                                  struct pipe_resource *dst,
                                  unsigned dst_level,
                                  unsigned dstx, unsigned dsty, unsigned dstz,
                                  struct pipe_resource *src,
                                  unsigned src_level,
                                  const struct pipe_box *src_box)
{
    struct radeon_info *info = &sctx->screen->info;
    struct si_texture *ssrc = (struct si_texture *)src;
    struct si_texture *sdst = (struct si_texture *)dst;
    unsigned bpp = sdst->surface.bpe;
    uint64_t dst_address = sdst->buffer.gpu_address +
                           sdst->surface.u.legacy.level[dst_level].offset;
    uint64_t src_address = ssrc->buffer.gpu_address +
                           ssrc->surface.u.legacy.level[src_level].offset;
    // ... (一系列变量和计算)

    // 线性 -> 线性子窗口拷贝
    if (dst_mode == RADEON_SURF_MODE_LINEAR_ALIGNED &&
        src_mode == RADEON_SURF_MODE_LINEAR_ALIGNED &&
        // 检查是否符合位字段限制
        // ... (一系列检查条件)
    {
        struct radeon_cmdbuf *cs = sctx->dma_cs;

        si_need_dma_space(sctx, 13, &sdst->buffer, &ssrc->buffer);

        radeon_emit(cs, CIK_SDMA_PACKET(CIK_SDMA_OPCODE_COPY,
                                        CIK_SDMA_COPY_SUB_OPCODE_LINEAR_SUB_WINDOW, 0) |
                      (util_logbase2(bpp) << 29));
        // ... (一系列 DMA 数据包配置)
        return true;
    }

    // 瓦片 <-> 线性子窗口拷贝
    if ((src_mode >= RADEON_SURF_MODE_1D) != (dst_mode >= RADEON_SURF_MODE_1D)) {
        struct si_texture *tiled = src_mode >= RADEON_SURF_MODE_1D ? ssrc : sdst;
        struct si_texture *linear = tiled == ssrc ? sdst : ssrc;
        // ... (一系列变量和计算)
        unsigned tiled_micro_mode = tiled == ssrc ? src_micro_mode : dst_micro_mode;

        // 检查是否符合位字段限制
        if (tiled_address % 256 == 0 &&
            linear_address % 4 == 0 &&
            linear_pitch % xalign == 0 &&
            linear_x % xalign == 0 &&
            tiled_x % xalign == 0 &&
            // ... (一系列检查条件)
        {
            struct radeon_cmdbuf *cs = sctx->dma_cs;
            uint32_t direction = linear == sdst ? 1u << 31 : 0;

            si_need_dma_space(sctx, 14, &sdst->buffer, &ssrc->buffer);

            radeon_emit(cs, CIK_SDMA_PACKET(CIK_SDMA_OPCODE_COPY,
                                            CIK_SDMA_COPY_SUB_OPCODE_TILED_SUB_WINDOW, 0) |
                          direction);
            // ... (一系列 DMA 数据包配置)
            return true;
        }
    }

    // 瓦片 -> 瓦片子窗口拷贝
    if (dst_mode >= RADEON_SURF_MODE_1D &&
        src_mode >= RADEON_SURF_MODE_1D &&
        // 检查是否符合位字段限制
        // ... (一系列检查条件)
    {
        struct radeon_cmdbuf *cs = sctx->dma_cs;

        si_need_dma_space(sctx, 15, &sdst->buffer, &ssrc->buffer);

        radeon_emit(cs, CIK_SDMA_PACKET(CIK_SDMA_OPCODE_COPY,
                                        CIK_SDMA_COPY_SUB_OPCODE_T2T_SUB_WINDOW, 0));
        // ... (一系列 DMA 数据包配置)
        return true;
    }

    return false;
}

```
* 这段代码是一个 GPU DMA 引擎的调用，用于在不同纹理间执行拷贝操作。拷贝的类型和参数会根据纹理的特性进行不同的处理。这包括线性到线性、瓦片到线性、以及瓦片到瓦片等不同情况。这里使用的是 AMD 的 CIK 架构的 DMA 数据包来配置拷贝操作。整个函数的目的是在满足一系列条件的情况下配置 DMA 数据包，并将其传递给 GPU。


## DMA IB 数据下发


```c
void si_flush_dma_cs(struct si_context *ctx, unsigned flags,
		     struct pipe_fence_handle **fence)
{
	struct radeon_cmdbuf *cs = ctx->dma_cs;
	struct radeon_saved_cs saved;
	bool check_vm = (ctx->screen->debug_flags & DBG(CHECK_VM)) != 0;

	if (!radeon_emitted(cs, 0)) {
		if (fence)
			ctx->ws->fence_reference(fence, ctx->last_sdma_fence);
		return;
	}

	if (check_vm)
		si_save_cs(ctx->ws, cs, &saved, true);

	ctx->ws->cs_flush(cs, flags, &ctx->last_sdma_fence);
	if (fence)
		ctx->ws->fence_reference(fence, ctx->last_sdma_fence);

	if (check_vm) {
		/* Use conservative timeout 800ms, after which we won't wait any
		 * longer and assume the GPU is hung.
		 */
		ctx->ws->fence_wait(ctx->ws, ctx->last_sdma_fence, 800*1000*1000);

		si_check_vm_faults(ctx, &saved, RING_DMA);
		si_clear_saved_cs(&saved);
	}
}
```

* 通过cs_flush 下发dma ib 与 gfx ib一致

# 调试

调试直接打开R600_DEBUG=testdma

通过si提供的si_test_dma来测试

```
  0: dst = ( 1431 x  1916 x 1, 2D_TILED_THIN1),  src = (   98 x    85 x 1, 1D_TILED_THIN1), bpp =  4, 
BLITs: GFX = 30, DMA =  0, fail [0/1]
   1: dst = (  602 x  1607 x 1, 2D_TILED_THIN1),  src = (  602 x  1607 x 1, 2D_TILED_THIN1), bpp =  2,
BLITs: GFX =  1, DMA =  0, fail [0/2]
   2: dst = (  292 x   644 x 3, 2D_TILED_THIN1),  src = (  292 x   644 x 3, 2D_TILED_THIN1), bpp = 16,  
BLITs: GFX =  1, DMA =  0, fail [0/3]
   3: dst = (  634 x   459 x 1, 2D_TILED_THIN1),  src = (  634 x   459 x 1, LINEAR_ALIGNED), bpp =  8,  
BLITs: GFX =  1, DMA =  0, fail [0/4]
   4: dst = (   64 x  2048 x 1, 1D_TILED_THIN1),  src = (   64 x  2048 x 1, 1D_TILED_THIN1), bpp =  1,  
BLITs: GFX =  0, DMA =  1, fail [0/5]
   5: dst = (   40 x    28 x 1, 1D_TILED_THIN1),  src = ( 1761 x   273 x 1, 2D_TILED_THIN1), bpp =  8,  
BLITs: GFX = 30, DMA =  0, fail [0/6]
   6: dst = (  228 x  2007 x 1, 2D_TILED_THIN1),  src = ( 1289 x   283 x 1, 2D_TILED_THIN1), bpp =  4,  
BLITs: GFX = 30, DMA =  0, fail [0/7]
   7: dst = (  842 x  1918 x 1, LINEAR_ALIGNED),  src = ( 1131 x  1607 x 1, 2D_TILED_THIN1), bpp =  2,  
BLITs: GFX = 30, DMA =  0, pass [1/8]
   8: dst = (  128 x    64 x 1, 2D_TILED_THIN1),  src = (  128 x    64 x 1, 2D_TILED_THIN1), bpp =  8,  
BLITs: GFX =  1, DMA =  0, fail [1/9]
   9: dst = (   63 x  7463 x 1, 1D_TILED_THIN1),  src = ( 1481 x   860 x 1, 2D_TILED_THIN1), bpp =  2,
```
可以看到在mesa18全都失败

***在mesa3d的提交历史可以看出这个sdma功能存在问题, 在 mesa-21 https://github.com/Mesa3D/mesa/commit/1f31a216640f294ce310898773d9b42bda5d1d47 中中移除了所有sdma功能， 而在https://github.com/Mesa3D/mesa/commit/46c95047bd77f6e824e4edccad590da8a1823fb4 又重新加入了sdma功能，但是只保留了图像复制操作, 所以对于之后的调试都将R600_DEBUG=nodma 打开***





