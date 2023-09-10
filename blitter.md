

blitter分析

在radeonsi驱动中使用了blitter功能。
功能结构体通过si_context.blitter实现的。

在galllium提供了blitter这种功能。源文件位于
gallium/auxiliary/util/目录下


blitter是一个blitter_context结构体如下
```c
/**
 * Blitter上下文结构体，用于执行各种图形绘制操作。
 */
struct blitter_context
{
   /**
    * 绘制矩形。
    *
    * \param get_vs 用于获取绘制调用的顶点着色器的回调函数。
    *                可能会触发着色器编译。驱动程序负责设置顶点着色器，
    *                而回调允许驱动程序查询顶点着色器CSO（常态着色器对象），
    *                如果它想要使用默认的顶点着色器。
    * \param x1     左上角的X坐标。
    * \param y1     左上角的Y坐标。
    * \param x2     右下角的X坐标。
    * \param y2     右下角的Y坐标。
    * \param depth  渲染矩形的深度值。
    * \param num_instances  实例数（通常为1，表示单个实例）。
    * \param type   属性的语义 "attrib"。
    *               如果 type 为 UTIL_BLITTER_ATTRIB_NONE，则忽略它们。
    *               如果 type 为 UTIL_BLITTER_ATTRIB_COLOR，属性
    *               构成一个常量RGBA颜色，并应该传递到片段着色器的GENERIC0变化槽。
    *               如果 type 为 UTIL_BLITTER_ATTRIB_TEXCOORD，则
    *               {a1, a2} 和 {a3, a4} 指定矩形的左上和右下纹理坐标，
    *               并应传递到片段着色器的GENERIC0变化槽。
    *
    * \param attrib 请参阅 type。
    *
    * \note 驱动程序可以选择覆盖此回调以实现绘制矩形的专用硬件路径，例如使用矩形点精灵。
    */
   void (*draw_rectangle)(struct blitter_context *blitter,
                          void *vertex_elements_cso,
                          blitter_get_vs_func get_vs,
                          int x1, int y1, int x2, int y2,
                          float depth, unsigned num_instances,
                          enum blitter_attrib_type type,
                          const union blitter_attrib *attrib);

   /* Blitter是否正在运行。 */
   bool running;

   bool use_index_buffer;

   /* 私有成员，实际上。 */
   struct pipe_context *pipe; /**< 管道上下文 */

   void *saved_blend_state;   /**< 混合状态 */
   void *saved_dsa_state;     /**< 深度模板alpha状态 */
   void *saved_velem_state;   /**< 顶点元素状态 */
   void *saved_rs_state;      /**< 光栅化状态 */
   void *saved_fs, *saved_vs, *saved_gs, *saved_tcs, *saved_tes; /**< 着色器 */

   struct pipe_framebuffer_state saved_fb_state;  /**< 帧缓冲状态 */
   struct pipe_stencil_ref saved_stencil_ref;     /**< 模板引用 */
   struct pipe_viewport_state saved_viewport;
   struct pipe_scissor_state saved_scissor;
   bool skip_viewport_restore;
   bool is_sample_mask_saved;
   unsigned saved_sample_mask;

   unsigned saved_num_sampler_states;
   void *saved_sampler_states[PIPE_MAX_SAMPLERS];

   unsigned saved_num_sampler_views;
   struct pipe_sampler_view *saved_sampler_views[PIPE_MAX_SAMPLERS];

   unsigned cb_slot;
   struct pipe_constant_buffer saved_fs_constant_buffer;

   unsigned vb_slot;
   struct pipe_vertex_buffer saved_vertex_buffer;

   unsigned saved_num_so_targets;
   struct pipe_stream_output_target *saved_so_targets[PIPE_MAX_SO_BUFFERS];

   struct pipe_query *saved_render_cond_query;
   uint saved_render_cond_mode;
   bool saved_render_cond_cond;

   boolean saved_window_rectangles_include;
   unsigned saved_num_window_rectangles;
   struct pipe_scissor_state saved_window_rectangles[PIPE_MAX_WINDOW_RECTANGLES];
};
```


## blitter的创建

blitter的创建是通过util_blitter_create接口在si_create_context内部创建的,并且作了如下一些变量的初始化。

```c
static struct pipe_context *si_create_context(struct pipe_screen *screen,
                                              unsigned flags)
{

	...

	sctx->blitter = util_blitter_create(&sctx->b);
	if (sctx->blitter == NULL)
		goto fail;
	sctx->blitter->draw_rectangle = si_draw_rectangle;
	sctx->blitter->skip_viewport_restore = true;

	...
}

```
从这可以看到radeonsi提供了si_draw_rectangle接口用于实现专用硬件绘制。

这个skip_viewport_restore看源码是在util_blitter_restore_fragment_states 用到用于恢复窗口状态。


### util_blitter_create

```c
/**
 * Blitter上下文的私有部分，用于执行图形数据传输和处理操作。
 */
struct blitter_context_priv
{
   struct blitter_context base; /**< 基本的Blitter上下文 */

   float vertices[4][2][4];   /**< {pos, color} 或 {pos, texcoord} */

   /* 各种状态对象的模板。 */

   /* 常量状态对象。 */
   /* 顶点着色器。 */
   void *vs; /**< 传递 {pos, generic} 到输出的顶点着色器。*/
   void *vs_nogeneric;
   void *vs_pos_only[4]; /**< 仅传递 pos 到输出的顶点着色器，用于 clear_buffer/copy_buffer。*/
   void *vs_layered; /**< 设置 LAYER = INSTANCEID 的顶点着色器。 */

   /* 片段着色器。 */
   void *fs_empty;
   void *fs_write_one_cbuf;
   void *fs_write_all_cbufs;

   /* 从纹理输出颜色的FS，其中
    * 第1个索引指示纹理类型/目标类型，
    * 第2个索引是要采样的 PIPE_TEXTURE_*，
    * 第3个索引是 0 = 使用 TEX，1 = 使用 TXF。
    */
   void *fs_texfetch_col[5][PIPE_MAX_TEXTURE_TYPES][2];

   /* 从纹理输出深度的FS，其中
    * 第1个索引是要采样的 PIPE_TEXTURE_*，
    * 第2个索引是 0 = 使用 TEX，1 = 使用 TXF。
    */
   void *fs_texfetch_depth[PIPE_MAX_TEXTURE_TYPES][2];
   void *fs_texfetch_depthstencil[PIPE_MAX_TEXTURE_TYPES][2];
   void *fs_texfetch_stencil[PIPE_MAX_TEXTURE_TYPES][2];

   /* 从多采样纹理输出一个样本的FS。 */
   void *fs_texfetch_col_msaa[5][PIPE_MAX_TEXTURE_TYPES];
   void *fs_texfetch_depth_msaa[PIPE_MAX_TEXTURE_TYPES];
   void *fs_texfetch_depthstencil_msaa[PIPE_MAX_TEXTURE_TYPES];
   void *fs_texfetch_stencil_msaa[PIPE_MAX_TEXTURE_TYPES];

   /* 输出所有样本的平均值的FS。 */
   void *fs_resolve[PIPE_MAX_TEXTURE_TYPES][NUM_RESOLVE_FRAG_SHADERS][2];

   /* 混合状态。 */
   void *blend[PIPE_MASK_RGBA+1][2]; /**< 带写入掩码的混合状态 */
   void *blend_clear[GET_CLEAR_BLEND_STATE_IDX(PIPE_CLEAR_COLOR)+1];

   /* 深度模板alpha状态。 */
   void *dsa_write_depth_stencil;
   void *dsa_write_depth_keep_stencil;
   void *dsa_keep_depth_stencil;
   void *dsa_keep_depth_write_stencil;

   /* 顶点元素状态。 */
   void *velem_state;
   void *velem_state_readbuf[4]; /**< X, XY, XYZ, XYZW */

   /* 采样器状态。 */
   void *sampler_state;
   void *sampler_state_linear;
   void *sampler_state_rect;
   void *sampler_state_rect_linear;

   /* 光栅化状态。 */
   void *rs_state, *rs_state_scissor, *rs_discard_state;

   /* 目标表面的尺寸。 */
   unsigned dst_width;
   unsigned dst_height;

   void *custom_vs;

   bool has_geometry_shader;
   bool has_tessellation;
   bool has_layered;
   bool has_stream_out;
   bool has_stencil_export;
   bool has_texture_multisample;
   bool has_tex_lz;
   bool has_txf;
   bool cube_as_2darray;
   bool cached_all_shaders;

   /* Draw模块重写这些函数。
    * 始终在Draw之前创建Blitter。 */
   void   (*bind_fs_state)(struct pipe_context *, void *);
   void   (*delete_fs_state)(struct pipe_context *, void *);
};
```


util_blitter_create 实际上是内部创建了一个blitter_context_priv。

这个函数是用来创建一个Blitter上下文的，该上下文用于执行图形数据传输和处理操作。以下是这个函数的主要作用和步骤的解释：

1. **分配内存**：函数开始时，它使用`CALLOC_STRUCT`分配了一个`struct blitter_context_priv`结构体的内存，这将用作Blitter上下文。如果内存分配失败，函数返回`NULL`。

2. **初始化上下文基本信息**：函数设置Blitter上下文的一些基本信息，包括与上下文关联的管道（`pipe`）和Blitter的绘制矩形函数（`draw_rectangle`）。

3. **初始化状态对象为无效值**：函数将Blitter上下文中的各种状态对象的初始值设置为`INVALID_PTR`，表示它们尚未被有效初始化。这些状态对象包括混合状态、深度模板alpha状态、光栅化状态、采样器状态等等。

4. **检测GPU功能**：函数根据GPU的能力来设置一些标志，以确定GPU是否支持几个重要的功能，如几何着色器、细分着色器、流输出、模板导出、纹理多重采样等等。

5. **初始化混合状态对象**：函数创建并初始化混合状态对象，包括不同颜色写入掩码的混合状态和带有混合的混合状态。

6. **初始化深度模板alpha状态对象**：函数创建和初始化深度模板alpha状态对象，包括保持深度模板、保持深度、写入深度、写入深度并保持模板等状态。

7. **初始化采样器状态对象**：函数创建和初始化采样器状态对象，包括一般采样器状态和线性采样器状态。

8. **初始化光栅化状态对象**：函数创建和初始化光栅化状态对象，包括剔除面、半像素居中、底边规则、扁平着色、深度剪切等状态。

9. **初始化顶点元素状态对象**：函数创建和初始化顶点元素状态对象，用于描述顶点数据的格式。

10. **根据流输出初始化额外的顶点元素状态对象**：如果支持流输出，函数创建和初始化额外的顶点元素状态对象，以用于流输出操作。

11. **检测是否支持分层视图和视口**：函数检测GPU是否支持分层视图和视口，并设置相应的标志。

12. **设置不变的顶点坐标**：函数将不变的顶点坐标值设置为1（v.w），这可能是用于一些固定的顶点着色器操作。

13. **返回Blitter上下文**：最后，函数返回Blitter上下文，该上下文现在已准备好执行图形数据传输和处理操作。

总之，这个函数的主要作用是初始化和配置Blitter上下文，为后续的图形数据传输和处理操作提供必要的状态和功能。这些操作通常用于底层的图形编程，例如GPU驱动程序或图形库的实现。






















## si_blit.c中的接口

这个文本列出了一系列与图形处理和Blit操作相关的函数。根据你的要求，以下是其中提炼出的函数列表：

1. `si_blit` - 执行Blit操作的主要函数。
2. `si_blit_dbcb_copy` - 用于在两个si_texture之间复制数据。
3. `si_blit_decompress_color` - 解压缩颜色纹理。
4. `si_blit_decompress_depth` - 解压缩深度纹理。
5. `si_blit_decompress_zs_in_place` - 在原地解压缩深度和模板纹理。
6. `si_blit_decompress_zs_planes_in_place` - 在原地解压缩深度和模板纹理的特定平面。
7. `si_blitter_begin` - 开始Blitter操作。
8. `si_blitter_end` - 结束Blitter操作。
9. `si_check_render_feedback` - 检查渲染反馈。
10. `si_check_render_feedback_images` - 检查渲染反馈中的图像。
11. `si_check_render_feedback_resident_images` - 检查渲染反馈中的居住图像。
12. `si_check_render_feedback_resident_textures` - 检查渲染反馈中的居住纹理。
13. `si_check_render_feedback_texture` - 检查渲染反馈中的纹理。
14. `si_check_render_feedback_textures` - 检查渲染反馈中的纹理。
15. `si_decompress_color_texture` - 解压缩颜色纹理。
16. `si_decompress_dcc` - 解压缩DCC压缩的颜色纹理。
17. `si_decompress_depth` - 解压缩深度纹理。
18. `si_decompress_image_color_textures` - 解压缩图像中的颜色纹理。
19. `si_decompress_resident_images` - 解压缩居住图像。
20. `si_decompress_resident_textures` - 解压缩居住纹理。
21. `si_decompress_sampler_color_textures` - 解压缩采样器中的颜色纹理。
22. `si_decompress_sampler_depth_textures` - 解压缩采样器中的深度纹理。
23. `si_decompress_subresource` - 解压缩特定子资源。
24. `si_decompress_textures` - 解压缩纹理。
25. `si_do_CB_resolve` - 执行颜色缓冲解析操作。
26. `si_flush_resource` - 刷新资源。
27. `si_generate_mipmap` - 生成纹理的mipmap。
28. `si_init_blit_functions` - 初始化Blit函数。
29. `si_resource_copy_region` - 在两个资源之间复制数据。
30. `u_max_sample` - 获取资源的最大采样数。

这些函数用于不同的图形处理任务，包括颜色和深度缓冲区的操作、纹理解压缩、渲染反馈检查等。具体的函数功能和用途会根据函数的实现和上下文而有所不同。


