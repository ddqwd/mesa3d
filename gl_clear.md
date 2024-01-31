
_mesa_Clear
    clear
        st_Clear
            st_validate_state
                st_validate_framebuffer
                    st_framebuffer_validate
                        dri_st_framebuffer_validate
                    st_context_validate
                    st_update_window_rectangles 
                    st_update_framebuffer_rectangles
                    st_update_scissor
            si_clear
                si_do_fast_color_clear
                    vi_dcc_enabled
                    si_clear_buffer
                        si_compute_do_clear_or_copy
                            si_get_shader_buffers
                                si_get_buffer_from_descriptors
                                si_set_shader_buffers
                                si_create_dma_compute_shader
                                    si_create_compute_state
                                        si_schedule_initial_compile
                                si_bind_compute_state
                                    si_set_active_descriptors
                                si_lauch_grid
                                    si_decompress_texture
                                    si_context_add_resource_size
                                    si_need_gfx_cs_space
                                    si_initialize_compute
                                    si_switch_compute_shader
                                    si_upload_compute_shader_descriptors
                                        si_upload_shader_descritptors
                                    si_emit_compute_shader_pointer

                                    si_emit_dispatch_packets
                    si_cp_dma_clear_buffer
R_00B858_COMPUTE_STATIC_THREAD_MGMT_SE0,



DCC（Delta Color Compression）是一种图形处理技术，用于压缩图形颜色数据以节省显存带宽和存储空间。它主要用于图形渲染中，特别是在图形卡中，以提高渲染性能和效率。

DCC的工作原理是在颜色数据中查找重复的颜色值，并将它们替换为一种更紧凑的表示方式。这可以通过以下方式实现：

1. 颜色压缩：DCC会检测相邻像素中相似的颜色值，并使用更少的位数来表示它们。例如，如果相邻像素的颜色非常接近，DCC可以使用较少的位数来存储颜色分量，从而减少存储需求。

2. 压缩块：DCC通常将颜色数据划分为块，并对每个块进行压缩。这可以减少冗余数据的存储，并在需要时只解压缩特定块。

3. 压缩格式：DCC还可以使用不同的颜色表示格式，如YUV（亮度和色度分量）来存储颜色数据，而不是RGB（红绿蓝分量）。这可以进一步减少存储空间。

DCC通常用于图形卡的纹理和帧缓冲区，以减少颜色数据的存储和传输成本。它可以显着提高图形渲染性能，特别是在需要处理大量纹理和渲染目标的情况下。

需要注意的是，DCC是硬件和驱动程序实现的特定技术，因此它的支持和效果可能因不同的图形卡和驱动程序而异。

CMASK（Color Mask）是一种图形处理技术，用于提高图形渲染性能和节省显存带宽。CMASK的主要作用是减少图形渲染过程中的颜色数据传输和存储成本。

CMASK的工作原理如下：

1. **颜色掩码存储**：在图形渲染过程中，通常会使用多个像素的颜色值进行计算。CMASK会存储一个颜色掩码，该掩码表示哪些像素的颜色值是有效的，哪些是无效的。只有有效的像素颜色值才会被传输到帧缓冲区或存储在显存中。

2. **颜色值的复制**：对于无效的像素颜色值，CMASK会将其设置为与有效像素相邻的颜色值的副本。这意味着无效像素的颜色值与相邻像素的颜色值相同，从而减少了需要传输的颜色数据的数量。

3. **颜色数据压缩**：CMASK还可以采用颜色数据的压缩格式，以减少存储颜色数据所需的位数。这可以进一步降低存储需求。

CMASK通常用于图形卡的帧缓冲区和纹理，以减少颜色数据的传输和存储成本。它可以显着提高图形渲染性能，特别是在需要处理大量像素的情况下，例如高分辨率屏幕、多重采样抗锯齿（MSAA）和其他复杂的渲染技术中。

需要注意的是，CMASK是硬件和驱动程序实现的特定技术，因此它的支持和效果可能因不同的图形卡和驱动程序而异。

    


static void si_do_fast_color_clear(struct si_context *sctx,
				   unsigned *buffers,
				   const union pipe_color_union *color)
	struct pipe_framebuffer_state *fb = &sctx->framebuffer.state;
    
    // 记录
	if (sctx->render_cond)
		return;

    
	for (i = 0; i < fb->nr_cbufs; i++) 
		struct si_texture *tex;
		unsigned clear_bit = PIPE_CLEAR_COLOR0 << i; // from _mesa_Clear
        if(fb->cbufs[i] && !(*buffers &clear_bit) && ..)
		    tex = (struct si_texture *)fb->cbufs[i]->texture;
            
            // not dccclear R600_DEBUG=nodccclear
            if( ... && si_alloc_separate_cmask(... )) 

    ...
 
