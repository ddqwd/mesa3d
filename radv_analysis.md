


# radv_winsys的创建初始化

在开始枚举物理设备时,入口点为radv_EnumeratePhysicalDevices，内部首先调用radv_enumerate_devices进行设备探测

在radv_enumerate_devices 内部 调用libdrm::drmGetDevices2获取可用物理设备数量后，调用radv_physical_device_init 初始化每个设备对应的radv_physical_device
radv_physical_device_init:
    1. open drm node，获取文件描述符后，通过libdrm接口获取版本号设备名，
    2. 调用radv_amdgpu_winsys_create初始化device->ws,类型为radv_amdgpu_winsys
    3. 调用 radv_physical_device_init_mem_types 进行设备内存类型属性的初始化
    4. 调用radv_init_wsi初始化radv_physical_device->wsi_device[]

radv_amdgpu_winsys_create:
    1. do_winsys_init
    2. bo,cs, surface的虚表接口初始化 


# radv_CreateDevice


struct radv_device {
	VK_LOADER_DATA                              _loader_data;

	VkAllocationCallbacks                       alloc;

	struct radv_instance *                       instance;
	struct radeon_winsys *ws;

	struct radv_meta_state                       meta_state;

	struct radv_queue *queues[RADV_MAX_QUEUE_FAMILIES];
	int queue_count[RADV_MAX_QUEUE_FAMILIES];
	struct radeon_cmdbuf *empty_cs[RADV_MAX_QUEUE_FAMILIES];

	bool always_use_syncobj;
	bool has_distributed_tess;
	bool pbb_allowed;
	bool dfsm_allowed;
	uint32_t tess_offchip_block_dw_size;
	uint32_t scratch_waves;
	uint32_t dispatch_initiator;

	uint32_t gs_table_depth;

	/* MSAA sample locations.
	 * The first index is the sample index.
	 * The second index is the coordinate: X, Y. */
	float sample_locations_1x[1][2];
	float sample_locations_2x[2][2];
	float sample_locations_4x[4][2];
	float sample_locations_8x[8][2];
	float sample_locations_16x[16][2];

	/* CIK and later */
	uint32_t gfx_init_size_dw;
	struct radeon_winsys_bo                      *gfx_init;

	struct radeon_winsys_bo                      *trace_bo;
	uint32_t                                     *trace_id_ptr;

	/* Whether to keep shader debug info, for tracing or VK_AMD_shader_info */
	bool                                         keep_shader_info;

	struct radv_physical_device                  *physical_device;

	/* Backup in-memory cache to be used if the app doesn't provide one */
	struct radv_pipeline_cache *                mem_cache;

	/*
	 * use different counters so MSAA MRTs get consecutive surface indices,
	 * even if MASK is allocated in between.
	 */
	uint32_t image_mrt_offset_counter;
	uint32_t fmask_mrt_offset_counter;
	struct list_head shader_slabs;
	mtx_t shader_slab_mutex;

	/* For detecting VM faults reported by dmesg. */
	uint64_t dmesg_timestamp;

	struct radv_device_extension_table enabled_extensions;

	/* Whether the driver uses a global BO list. */
	bool use_global_bo_list;

	struct radv_bo_list bo_list;

	/* Whether anisotropy is forced with RADV_TEX_ANISO (-1 is disabled). */
	int force_aniso;
};



    1. 初始化重要成员来自radv_physical_device 
        radv_device-> physical_device = radv_physical_device
        device->ws = radv_amdgpu_winsys
    2. 创建逻辑队列并且通过radv_queue_init初始化, 在radv_queue_init内部会进行cs上下文的初始化
       这一步通过调用radv_amdgpu_ctx_create完成, 而radv_amdgpu_ctx_create内部首先调用libdrm接口amdgpu_cs_ctx_create2创建上下文，然后通过winsys接口buffer_create创建了fence_bo，并且通过buffer_map获取映射地址

    3. 初始化每个queuefamily的radeon_cmdbuf empty_cs
      对于RADV_QUEUE_GENERAL和 RADV_QUEUE_COMPUTE分别为PKT3_CONTEXT_CONTROL和PKT3_NOP
      之后调用radv_amdgpu_cs_finalize填充固定数值

    4. 创建了管线缓存device->mem_cache




## radv_winsys bo的创建

buffer_create := radv_amdgpu_winsys_bo_create
该接口和amdgpu_winsys的接口基本功能一致:
1. amdgpu_bo_alloc
2. amdgpu_bo_va_op_raw

## radv_winsys bo的map映射
buffer_map := radv_amdgpu_winsys_bo_map
内部直接调用amdgpu_bo_cpu_map获取映射地址





# radv_CreateCommandPool

内部直接创建了一个 radv_cmd_pool
该结构就是一个以cmd_buffers为头的链表



# radv_CreateSemaphore

radv_semaphore
TODO:

# 创建surface

## radv_GetPhysicalDeviceXcbPresentationSupportKHR

```
   return wsi_get_physical_device_xcb_presentation_support(
      &device->wsi_device,
      queueFamilyIndex,
      XGetXCBConnection(dpy), visualID);

```

# radv_GetPhysicalDeviceSurfaceCapabilitiesKHR


# radv_CreateRenderPass

radv_render_pass *

struct radv_render_pass {
	uint32_t                                     attachment_count;
	uint32_t                                     subpass_count;
	struct radv_subpass_attachment *             subpass_attachments;
	struct radv_render_pass_attachment *         attachments;
	struct radv_subpass_barrier                  end_barrier;
	struct radv_subpass                          subpasses[0];
};


# radv_CreateSwapchainKHR


wsi_swapchain

```
struct wsi_swapchain {
   const struct wsi_device *wsi;

   VkDevice device;
   VkAllocationCallbacks alloc;
   VkFence fences[3];
   VkPresentModeKHR present_mode;
   uint32_t image_count;

   bool use_prime_blit;

   /* Command pools, one per queue family */
   VkCommandPool *cmd_pools;

   VkResult (*destroy)(struct wsi_swapchain *swapchain,
                       const VkAllocationCallbacks *pAllocator);
   struct wsi_image *(*get_wsi_image)(struct wsi_swapchain *swapchain,
                                      uint32_t image_index);
   VkResult (*acquire_next_image)(struct wsi_swapchain *swap_chain,
                                  const VkAcquireNextImageInfoKHR *info,
                                  uint32_t *image_index);
   VkResult (*queue_present)(struct wsi_swapchain *swap_chain,
                             uint32_t image_index,
                             const VkPresentRegionKHR *damage);
};

```


-> wsi_common_create_swapchain(wsi_device*wsi, device, pCreateInfo, pAllocator, pSwapChain)
   wsi->base.create_swapchain = x11_surface_create_swapchain;

x11_surface_create_swapchain

1.  wsi_swapchain_init 调用radv_CreateCommandPool构建cmd_pools
2. 对于请求的minImageCount, 调用x11_image_init,初始化每一张图像, x11_image_init内部会调用wsi_create_native_image
在wsi_create_native_image内部首先会调用radv_CreateImage创建图像资源，然后调用radv_AllocateMemory申请bo内存，再调用radv_BindBufferMemory2将图像与该内存bo绑定起来
3. 初始化显示和获取队列,及队列管理线程用于保存将要显示的图像和可以显示的图像




wsi_image
    VkImage image;

# radv_CreateImage

radv_image:
     struct radeon_winsys_bo * bo;
     struct radeon_surf surface;

bo = 


# radv_AllocateMemory

radv_device_memory:
    struct radeon_winsys_bo *bo;
    struct radv_imgae *image;
    struct radv_buffer *buffer;
    void *map;
    void *use_ptr;

bo = device->ws->buffer_create(device->ws, alloc_size, device->physical_device->rad_info.max_alignment,
		                                    domain, flags);


# radv_BindImageMemory

radv_image.bo = radv_device_memory.bo 


# radv_GetImageMemoryRequirements

直接返回image 表面大小

# radv_CreateImageView

radv_image_view:
    struct radv_image *image; // from image 
	struct radeon_winsys_bo *bo; // from image->bo
	uint32_t descriptor[16];

descriptor:
    radv_image_view_make_descriptor
        si_make_texture_descriptor
        si_set_mutable_tex_desc_fields


# radv_CreateFramebuffer

radv_framebuffer:
	uint32_t                                     attachment_count;
	struct radv_attachment_info                  attachments[0];
    
radv_attachment_info:
	union {
		struct radv_color_buffer_info cb;
		struct radv_ds_buffer_info ds;
	};
	struct radv_image_view *attachment;

radv_color_buffer_info:
	uint64_t cb_color_base;
	uint64_t cb_color_cmask;


radv_ds_buffer_info:
	uint64_t db_z_read_base;
	uint64_t db_stencil_read_base;


attachment:
    radv_initialise_color_surface(device, &framebuffer->attachments[i].cb, iview);
    radv_initialise_ds_surface(device, &framebuffer->attachments[i].ds, iview);


# radv_CreateFence

radv_fence:
	struct radeon_winsys_fence *fence; // create by ws->create_fence
	struct wsi_fence *fence_wsi;



# radv_AllocateCommandBuffers

-> radv_create_cmd_buffer(device, pool, level, VkCommandBuffer * pCommandBuffer);

radv_cmd_buffer:
	struct radv_device *                          device;
	struct radv_cmd_pool *                        pool;
	struct list_head                             pool_link;
	struct radeon_cmdbuf *cs;
	struct radv_vertex_binding                   vertex_bindings[MAX_VBS];

cs = ws->cs_create;
    radv_amdgpu_cs_create



# radv_winsys_create

radeon_winsys:
    cs_submit
    buffer_create
    buffer_map
    surface_init


radv_amdgpu_winsys:
	struct radeon_winsys base;
	amdgpu_device_handle dev;

	struct radeon_info info;
	struct amdgpu_gpu_info amdinfo;
	ADDR_HANDLE addrlib;

# radv_amdgpu_cs_create


struct radeon_cmdbuf {
	unsigned cdw;  /* Number of used dwords. */
	unsigned max_dw; /* Maximum number of dwords. */
	uint32_t *buf; /* The base pointer of the chunk. */
};

radv_amdgpu_cs:
    struct radeon_cmdbuf base;
	struct radv_amdgpu_winsys *ws;

	struct amdgpu_cs_ib_info    ib;

	struct radeon_winsys_bo     *ib_buffer;

	uint8_t                 *ib_mapped;

	amdgpu_bo_handle            *handles;

	unsigned                    *ib_size_ptr;



if (cs->ws->use_ib_bos) {
ib_buffer=ws->buffer_create
cs->ib_mapped = ws->buffer_map(cs->ib_buffer);
cs->ib.ib_mc_address = radv_amdgpu_winsys_bo(cs->ib_buffer)->base.va;
cs->base.buf = (uint32_t *)cs->ib_mapped;
cs->base.max_dw = ib_size / 4 - 4;
cs->ib_size_ptr = &cs->ib.size;
cs->ib.size = 0;
ws->cs_add_buffer(&cs->base, cs->ib_buffer);
}




# radv_CreateDescriptorSetLayout


radv_descriptor_set_layout:
   /* The create flags for this descriptor set layout */
   VkDescriptorSetLayoutCreateFlags flags;

   /* Number of bindings in this descriptor set */
   uint32_t binding_count;

   uint32_t size; // total size

   /* Shader stages affected by this descriptor set */
   uint16_t shader_stages;
   uint16_t dynamic_shader_stages;

   /* Number of buffers in this descriptor set */
   uint32_t buffer_count;

   /* Number of dynamic offsets used by this descriptor set */
   uint16_t dynamic_offset_count;

   bool has_immutable_samplers;
   bool has_variable_descriptors;

   /* Bindings in this descriptor set */
   struct radv_descriptor_set_binding_layout binding[0];


可变数组

foreach pCreateInfo->bindingCount
    set_layout->size += binding->descriptorCount * set_layout->binding[b].size;
    buffer_count += binding->descriptorCount * binding_buffer_count;

set_layout->buffer_count = buffer_count;
set_layout->dynamic_offset_count = dynamic_offset_count;


# radv_CreatePipelineLayout

radv_pipeline_layout:
   struct {
      struct radv_descriptor_set_layout *layout;
      uint32_t size;
      uint32_t dynamic_offset_start;
   } set[MAX_SETS];

   uint32_t num_sets;
   uint32_t push_constant_size;
   uint32_t dynamic_offset_count;



foreach pCreateInfo->setLayoutCount: set
		layout->set[set].layout = set_layout;



# radv_CreateShaderModule


struct radv_shader_module {
	struct nir_shader *nir;
	unsigned char sha1[20];
	uint32_t size;
	char data[0];
};


module->size = pCreateInfo->codeSize;
memcpy(module->data, pCreateInfo->pCode, module->size);

**No compile , at time, nir is nullptr;**


# radv_CreateGraphicsPipelines

foreach count -> radv_graphics_pipeline_create

## radv_graphics_pipeline_create

radv_pipeline :
	struct radv_device *                          device;
	struct radv_dynamic_state                     dynamic_state;

	struct radv_pipeline_layout *                 layout;

	bool					     need_indirect_descriptor_sets;
	struct radv_shader_variant *                 shaders[MESA_SHADER_STAGES];
	struct radv_shader_variant *gs_copy_shader;
	VkShaderStageFlags                           active_stages;

	struct radeon_cmdbuf                      cs;

	struct radv_vertex_elements_info             vertex_elements;

	uint32_t                                     binding_stride[MAX_VBS];

	uint32_t user_data_0[MESA_SHADER_STAGES];
	union {
		struct {
			struct radv_multisample_state ms;
			uint32_t spi_baryc_cntl;
			bool prim_restart_enable;
			unsigned esgs_ring_size;
			unsigned gsvs_ring_size;
			uint32_t vtx_base_sgpr;
			struct radv_ia_multi_vgt_param_helpers ia_multi_vgt_param;
			uint8_t vtx_emit_num;
			struct radv_prim_vertex_count prim_vertex_count;
 			bool can_use_guardband;
			uint32_t needed_dynamic_state;
			bool disable_out_of_order_rast_for_occlusion;

			/* Used for rbplus */
			uint32_t col_format;
			uint32_t cb_target_mask;
		} graphics;
	};

	unsigned max_waves;
	unsigned scratch_bytes_per_wave;

	/* Not NULL if graphics pipeline uses streamout. */
	struct radv_shader_variant *streamout_shader;



1. 调用radv_pipeline_init初始化radv_pipeline, 内部调用radv_create_shaders 编译链接shader为binary



# shader的编译过程

调用radv_shader_compile_to_nir产生nir 

1. 构建vtn_builder， 并解析spir-v物理布局的前5个dwords信息
vtn_builder:

   nir_builder nb;

   const uint32_t *spirv;

   nir_shader *shader;

   const char *entry_point_name;
   struct vtn_value *entry_point;

   struct vtn_value *values;


其中values保存spir-v解析的指令信息,所有的nir_类型信息保存在相关成员里面

```
struct vtn_value {
   enum vtn_value_type value_type;
   const char *name;
   struct vtn_decoration *decoration;
   struct vtn_type *type;
   union {
      void *ptr;
      char *str;
      nir_constant *constant;
      struct vtn_pointer *pointer;
      struct vtn_image_pointer *image;
      struct vtn_sampled_image *sampled_image;
      struct vtn_function *func;
      struct vtn_block *block;
      struct vtn_ssa_value *ssa;
      vtn_instruction_handler ext_handler;
   };
};

```


2. 调用vtn_foreach_instruction处理序言指令

```
   /* Handle all the preamble instructions */
   words = vtn_foreach_instruction(b, words, word_end,
                                   vtn_handle_preamble_instruction);
```

3.  创建nir_shader保存nir信息
 

4. 解析执行模型 
```
   vtn_foreach_execution_mode(b, b->entry_point,
                              vtn_handle_execution_mode, NULL);

```
5. 对解析完的vtn_values设置类型

   `vtn_foreach_instruction(b, words, word_end, vtn_set_instruction_result_type);`

6.  解析控制流

   vtn_build_cfg(b, words, word_end);

7. 解析所有函数的函数体指令

    vtn_function_emit(b, func, vtn_handle_body_instruction);

8. 返回解析后的入口点nir_function 函数

   nir_function *entry_point = b->entry_point->func->impl->function;


9. 通过NIR_PASS_V等解析nir_shader信息,产生shader_info信息


10. 链接nir_shader ,这一步主要进行优化，消除未使用变量,未链接变量
radv_link_shaders(struct radv_pipeline *pipeline, nir_shader **shaders)


11. 创建shader_variant ,这一步主要产生LLVM IR

shader_variant_create
    radv_compile_nir_shader
        ac_translate_nir_to_llvm
            create_function 
和OpenGL一样在create_function形成shader的LLVM IR 函数参数
在创建llvm ir的main函数之后， create_function同时为参数的所用的寄存器指定了位置
其中参数的表示通过 radv_ud_index枚举来表示

enum radv_ud_index {
	AC_UD_SCRATCH_RING_OFFSETS = 0,
	AC_UD_PUSH_CONSTANTS = 1,
	AC_UD_INDIRECT_DESCRIPTOR_SETS = 2,
	AC_UD_VIEW_INDEX = 3,
	AC_UD_STREAMOUT_BUFFERS = 4,
	AC_UD_SHADER_START = 5,
	AC_UD_VS_VERTEX_BUFFERS = AC_UD_SHADER_START,
	AC_UD_VS_BASE_VERTEX_START_INSTANCE,
	AC_UD_VS_MAX_UD,
	AC_UD_PS_MAX_UD,
	AC_UD_CS_GRID_SIZE = AC_UD_SHADER_START,
	AC_UD_CS_MAX_UD,
	AC_UD_GS_MAX_UD,
	AC_UD_TCS_MAX_UD,
	AC_UD_TES_MAX_UD,
	AC_UD_MAX_UD = AC_UD_TCS_MAX_UD,
}

流程如下:
```
    user_sgpr_idx  = 0;

	set_global_input_locs(ctx, stage, has_previous_stage, previous_stage,
			      &user_sgpr_info, desc_sets, &user_sgpr_idx);



	switch (stage) {
	case MESA_SHADER_COMPUTE:
		if (ctx->shader_info->info.cs.uses_grid_size) {
			set_loc_shader(ctx, AC_UD_CS_GRID_SIZE,
				       &user_sgpr_idx, 3);
		}
		break;
	case MESA_SHADER_VERTEX:
		set_vs_specific_input_locs(ctx, stage, has_previous_stage,
					   previous_stage, &user_sgpr_idx);
		if (ctx->abi.view_index)
			set_loc_shader(ctx, AC_UD_VIEW_INDEX, &user_sgpr_idx, 1);
		break;
	case MESA_SHADER_TESS_CTRL:
		set_vs_specific_input_locs(ctx, stage, has_previous_stage,
					   previous_stage, &user_sgpr_idx);
		if (ctx->abi.view_index)
			set_loc_shader(ctx, AC_UD_VIEW_INDEX, &user_sgpr_idx, 1);
		break;
	case MESA_SHADER_TESS_EVAL:
		if (ctx->abi.view_index)
			set_loc_shader(ctx, AC_UD_VIEW_INDEX, &user_sgpr_idx, 1);
		break;
	case MESA_SHADER_GEOMETRY:
		if (has_previous_stage) {
			if (previous_stage == MESA_SHADER_VERTEX)
				set_vs_specific_input_locs(ctx, stage,
							   has_previous_stage,
							   previous_stage,
							   &user_sgpr_idx);
		}
		if (ctx->abi.view_index)
			set_loc_shader(ctx, AC_UD_VIEW_INDEX, &user_sgpr_idx, 1);
		break;
	case MESA_SHADER_FRAGMENT:
		break;
	default:
		unreachable("Shader stage not implemented");
	}


```
实际构建llvm main函数参数如下表示

*  vs

```
void main(
        /* global input SGPR */
        i8* desc_sets,  
        [i8 *push_constants],
        [v4i32 *stream_buffers] ,

        /* vs specific input sgprs */
        [v4i32* vetex_buffers], 
        i32 base_vetex,
        i32 start_instance, 
        [i32 draw_id],

        [i32 view_index],
        [i32 es2gs_offset],
        [i32 streamout_config],
        [i32 streamout_write_index],
        [i32 streamout_offset0],
        [i32 streamout_offset1],
        [i32 streamout_offset2],
        [i32 streamout_offset3],

        /* vs input vgprs */
        i32 vetex_id,
        [i32 rel_auto_id],
        i32 instance_id,
        [i32 vs_prim_id ],
        i32 NULL
)

```

其中i8 = int8_t ,i32 = int32_t
v4i32 = vecotr(int32, int32,int32,int32);

## tcs 

```
void main(
    /* 6 SYSTEM SGPR */
    i32  oc_lds,  
    i32 merged_wave_info,
    i32 tess_factor_offset,
    i32 scratchOffset,
    i32 unused1,
    i32 unused2,

    /* global input SGPR */
    i8* desc_sets,  
    [i8 *push_constants],
    [v4i32 *stream_buffers] ,

    /* vs specific input sgprs */
    [v4i32* vetex_buffers], 
    i32 base_vetex,
    i32 start_instance, 
    [i32 draw_id],

    [i32 view_index],
    i32 tcs_patch_id,
    i32 tcs_rel_idsa,

    /* vs input vgprs */
    i32 vetex_id,
    [i32 rel_auto_id],
    i32 instance_id,
    [i32 vs_prim_id ],
    i32 NULL
)

```

## tes

```
void main(
    /* global input SGPR */
    i8* desc_sets,  
    [i8 *push_constants],
    [v4i32 *stream_buffers] ,

    [i32 view_index],

    /* tes as es */
    [i32 oc_lds],
    [i32 unused1],
    [i32 es2gs_offset],

    /* tes as not es */
    i32 unused1,

    /* streamout sgprs */
    i32 streamout_config,
    i32 streamout_write_index,
    i32 streamout_offset1,
    i32 streamout_offset2,
    i32 streamout_offset3,
    i32 streamout_offset4,

    /* tes input vgprs */
    f32 tes_u,
    f32 tes_v,
    i32 tes_rel_patch_id,
    i32 tes_patch_id,
)

```


## gs 


```
void main(
    /* 6 system sgprs */
    i32 gs2vs_offset, 
    i32 merged_wave_info,
    i32 oc_lds,
    i32 scratchOffset,
    i32 unused1,
    i32 unused2,

    /* global input SGPR */
    i8* desc_sets,  
    [i8 *push_constants],
    [v4i32 *stream_buffers] ,

    /* vs specific input sgprs */
    [v4i32* vetex_buffers], 
    i32 base_vetex,
    i32 start_instance, 
    [i32 draw_id],

    /* view index sgpr */
    [i32 view_index],

    i32 gs_vtx_offset[0],
    i32 gs_vtx_offset[1],
    i32 gs_vtx_offset[2],
    i32 gs_prim_id,

    i32 gs_vtx_offset[3],
    i32 gs_vtx_offset[4],
    i32 gs_vtx_offset[5],
    i32 gs_invocation_id ,

    /* vs input vgprs */
    i32 vetex_id,
    [i32 rel_auto_id],
    i32 instance_id,
    [i32 vs_prim_id ],
    i32 NULL

    /* tes input vgprs */
    f32 tes_u,
    f32 tes_v,
    i32 tes_rel_patch_id,
    i32 tes_patch_id,

)


```
## fs

```
void main(
    /* global input SGPR */
    i8* desc_sets,  
    [i8 *push_constants],
    [v4i32 *stream_buffers] ,

    i32 prim_mask,
    v2i32 persp_sample,
    v2i32 persp_center,
    v2i32 persp_centroid,
    v3i32 persp_pull_model,

    v2i32 linear_sample, 
    v2i32 linear_center,
    v2i32 linear_centroid,
    f32 line_stipple_tex,
    f32 frag_pos[0],
    f32 frag_pos[1],
    f32 frag_pos[2],
    f32 frag_pos[3],
    i32 front_face,
    i32 ancillary,
    i32 sample_converage,
    i32 fixed_pt
)

```
## cs

```
void main(
    /* global input SGPR */
    i8* desc_sets,  
    [i8 *push_constants],
    [v4i32 *stream_buffers] ,

    v3i32 num_work_groups,
    i32 workgroup_ids[0], 
    i32 workgroup_ids[1], 
    i32 workgroup_ids[2], 

    /* use_local invocation_idx */
    i32 tg_size,

    v3i32 local_invocation_ids
)

```

12. 调用radv_alloc_shader_memory 申请bo保存shader binary

13. 如果存在gs ，则调用radv_create_gs_copy_shader创建gs_copy_shader.

14. 调用radv_pipeline_cache_insert_shaders将shader插入管线缓存radv_pipeline_cache


15 . 调用radv_pipeline_init_multisample_state初始化多重采样的管线状态

16 . 调用si_translate_prim解析图元种类 

17. 如果存在gs步骤， 调用calculate_gs_ring_sizes计算gs_ring大小, 这一点和radeonsi中preload_ring基本相同

18. 如果存在tes， 调用calculate_tess_state计算tes参数中所需要的lds大小等参数

19. 设定每个shader步骤的user_data0的寄存器基地址

20. 调用radv_pipeline_generate_pm4 下发各种shader及管线状态的pm4包，保存在pipeline->cs中



# radv_CreateBuffer

radv_buffer:
    VkDeviceSize size;
    struct radeon_winsys_bo * bo ;
    VkDeviceSize offset;


当pCreateInfo->flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT为真时立刻创建bo
否则通过绑定allocate_memory分配的内存获取


# radv_MapMemory

直接返回buffer_map地址


# radv_BindBufferMemory

-> radv_BindBufferMemory2
    buffer->bo = mem->bo;


# radv_CreateDescriptorPool

radv_descriptor_pool:
    struct radeon_winsys_bo *bo;
	uint8_t *mapped_ptr;
	uint64_t current_offset;
	uint64_t size;

	uint8_t *host_memory_base;
	uint8_t *host_memory_ptr;
	uint8_t *host_memory_end;

	uint32_t entry_count;
	uint32_t max_entry_count;
	struct radv_descriptor_pool_entry entries[0];
};

pool->bo = buffer_create 
pool->mapped_ptr = (uint8_t*)device->ws->buffer_map(pool->bo);
pool->size = bo_size;
pool->max_entry_count = pCreateInfo->maxSets;


# radv_UpdateDescriptorSets
    -> radv_update_descriptor_sets




# radv_AcquireNextImage2KHR
    wsi_common_acquire_next_image2
        wsi_swapchain->acquire_next_image
            x11_acquire_next_image
                x11_acquire_next_image_from_queue

x11_acquire_next_image_from_queue(struct x11_swapchain *chain,
                                  uint32_t *image_index_out, uint64_t timeout)

调用wsi_queue_pull检索chain->acquire_queue获取可用图像的image_index.



# radv_BeginCommandBuffer

radv_reset_cmd_buffer

cmd_buffer->status = RADV_CMD_BUFFER_STATUS_RECORDING;

# radv_CmdUpdateBuffer


```
		radeon_emit(cmd_buffer->cs, PKT3(PKT3_WRITE_DATA, 2 + words, 0));
		radeon_emit(cmd_buffer->cs, S_370_DST_SEL(mec ?
		                                V_370_MEM_ASYNC : V_370_MEMORY_SYNC) |
		                            S_370_WR_CONFIRM(1) |
		                            S_370_ENGINE_SEL(V_370_ME));
		radeon_emit(cmd_buffer->cs, va);
		radeon_emit(cmd_buffer->cs, va >> 32);
		radeon_emit_array(cmd_buffer->cs, pData, words);

```



# radv_CmdBeginRenderPass

radv_cmd_buffer:
	struct radv_cmd_state state;

radv_cmd_state:
	/* Vertex descriptors */
	uint64_t                                      vb_va;
	unsigned                                      vb_size;

	bool predicating;
	uint32_t                                      dirty;

	uint32_t                                      prefetch_L2_mask;

	struct radv_pipeline *                        pipeline;
	struct radv_pipeline *                        emitted_pipeline;
	struct radv_pipeline *                        compute_pipeline;
	struct radv_pipeline *                        emitted_compute_pipeline;
	struct radv_framebuffer *                     framebuffer;
	struct radv_render_pass *                     pass;
	const struct radv_subpass *                         subpass;
	struct radv_dynamic_state                     dynamic;
	struct radv_attachment_state *                attachments;
	struct radv_streamout_state                  streamout;
	VkRect2D                                     render_area;


radv_attachment_state:
	VkImageAspectFlags                           pending_clear_aspects;
	uint32_t                                     cleared_views;
	VkClearValue                                 clear_value;
	VkImageLayout                                current_layout;



result = radv_cmd_state_setup_attachments(cmd_buffer, pass, pRenderPassBegin);
	for (uint32_t i = 0; i < pass->attachment_count; ++i) 
		struct radv_render_pass_attachment *att = &pass->attachments[i];
		VkImageAspectFlags att_aspects = vk_format_aspects(att->format);

		state->attachments[i].pending_clear_aspects = clear_aspects;
		state->attachments[i].cleared_views = 0;


# radv_CmdBindVertexBuffers

radv_vertex_binding:
	struct radv_buffer *                          buffer;
	VkDeviceSize                                 offset;

foreach bindingCount : i 
		vb[idx].buffer = radv_buffer_from_handle(pBuffers[i]);
		vb[idx].offset = pOffsets[i];

		radv_cs_add_buffer(cmd_buffer->device->ws, cmd_buffer->cs,
				   vb[idx].buffer->bo);

cmd_buffer->state.dirty |= RADV_CMD_DIRTY_VERTEX_BUFFER;


# radv_CmdBindPipeline

radv_cmd_buffer:
	struct radv_descriptor_state descriptors[VK_PIPELINE_BIND_POINT_RANGE_SIZE];

radv_descriptor_state:
	struct radv_descriptor_set *sets[MAX_SETS];
	uint32_t dirty;
	uint32_t valid;
	struct radv_push_descriptor_set push_set;
	bool push_dirty;
	uint32_t dynamic_buffers[4 * MAX_DYNAMIC_BUFFERS];



cmd_buffer->state.pipeline = pipeline;
cmd_buffer->state.dirty |= RADV_CMD_DIRTY_PIPELINE;

# radv_CmdBindDescriptorSets


foreach descriptorSetCount : i 
        //cmd_buffer->descriptors[bindpoint].sets[i]
		radv_bind_descriptor_set(cmd_buffer, pipelineBindPoint, set, idx);

        foreach set->layout->dynamic_offset_count
        ...



# radv_CmdSetViewporto

operation: radv_cmd_state* cmd_buffer->state

memcpy(state->dynamic.viewport.viewports + firstViewport, pViewports,
	       viewportCount * sizeof(*pViewports));
state->dirty |= RADV_CMD_DIRTY_DYNAMIC_VIEWPORT;


# radv_CmdPushConstants

operand: cmd_buffer->push_constants

operation:

memcpy(cmd_buffer->push_constants + offset, pValues, size);
cmd_buffer->push_constant_stages |= stageFlags;


# radv_CmdDraw

operand : cmd_buffer
build: 
    radv_draw_info info;

-> radv_draw(cmd_buffer, info)

1. 调用radv_emit_all_graphics_states下发所有状态pm4包
主要有以下几点:
* 根据RADV_CMD_DIRTY_PIPELINE ，下发图形管线状态
radv_emit_graphics_pipeline
    radv_update_multisample_state

	radeon_emit_array(cmd_buffer->cs, pipeline->cs.buf, pipeline->cs.cdw);

调用radv_emit_graphics_pipeline完成，在radv_emit_graphics_pipeline内部， 一是更新multisample状态, 二是将管线的pm4 即pipeline->cs合并到cmd_buffer的cs中

*根据 RADV_CMD_DIRTY_FRAMEBUFFER 下发帧缓冲区
radv_emit_framebuffer_state
    //针对8个颜色附件
    radv_emit_fb_color_state 主要是CB_COLOR_BASE寄存器
    // 下发深度模板相关
    radv_emit_fb_ds_state(cmd_buffer, &att->ds, image, layout);

* 如果存在索引buffer,下发PKT3_INDEX_BASE
radv_emit_index_buffer

* 下发draw寄存器pm4 
主要为
R_028AA8_IA_MULTI_VGT_PARAM,
R_028A94_VGT_MULTI_PRIM_IB_RESET_EN,
R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX,


* 调用si_write_scissors 发射裁剪状态

2. 调用si_emit_cache_flush 刷新缓存

3. 调用radv_upload_graphics_shader_descriptorsa发射shader寄存器地址


radv_cmd_buffer:
	struct radv_cmd_buffer_upload upload;


struct radv_cmd_buffer_upload {
	uint8_t *map;
	unsigned offset;
	uint64_t size;
	struct radeon_winsys_bo *upload_bo;
	struct list_head list;
};


```
static void
radv_upload_graphics_shader_descriptors(struct radv_cmd_buffer *cmd_buffer, bool pipeline_is_dirty)
{
    // vetex buffer
	radv_flush_vertex_descriptors(cmd_buffer, pipeline_is_dirty);
        // 申请upload的bo保存vbo描述符
		if (!radv_cmd_buffer_upload_alloc(cmd_buffer, count * 16, 256,
						  &vb_offset, &vb_ptr))

		for (i = 0; i < count; i++) {
			uint32_t *desc = &((uint32_t *)vb_ptr)[i * 4];
			struct radv_buffer *buffer = cmd_buffer->vertex_bindings[vb].buffer;
			va = radv_buffer_get_va(buffer->bo);
			desc[0] = va;
			desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32) | S_008F04_STRIDE(stride);
        }

		va = radv_buffer_get_va(cmd_buffer->upload.upload_bo);

        // 查找vbo在shader参数的位置 填入该参数位置相对应的USER_DATA_VS
        // 其中该地址由create_functin内部已经确定， 也可以通过查看vs main函数发现vetex_buffer的位置所在为4
		radv_emit_userdata_address(cmd_buffer, cmd_buffer->state.pipeline, MESA_SHADER_VERTEX,
					   AC_UD_VS_VERTEX_BUFFERS, va);
	        struct radv_userdata_info *loc = radv_lookup_user_sgpr(pipeline, stage, idx);


		cmd_buffer->state.vb_va = va;

	radv_flush_streamout_descriptors(cmd_buffer);

	radv_flush_descriptors(cmd_buffer, VK_SHADER_STAGE_ALL_GRAPHICS);

        //上传radv_descriptor_set 中的push_set的数据实际上传到cmd_buffer->upload
        if (descriptors_state->push_dirty)
            radv_flush_push_descriptors(cmd_buffer, bind_point);

        //上传indirect的描述符数据到cmd_buffer->upload
        //TODO 
        if (flush_indirect_descriptors)
            radv_flush_indirect_descriptor_sets(cmd_buffer, bind_point);


        if (cmd_buffer->state.pipeline) {
            radv_foreach_stage(stage, stages) {
                if (!cmd_buffer->state.pipeline->shaders[stage])
                    continue;

                // 上传descriptors_state中的sets数据
                // 连续上传起点为descriptors_state->dirty 
                //连续数为mask
                radv_emit_descriptor_pointers(cmd_buffer,
                                  cmd_buffer->state.pipeline,
                                  descriptors_state, stage);
            }
        }

        //如果存在计算管线上传计算管线的描述符集数据
        if (cmd_buffer->state.compute_pipeline &&
            (stages & VK_SHADER_STAGE_COMPUTE_BIT)) {
            radv_emit_descriptor_pointers(cmd_buffer,
                              cmd_buffer->state.compute_pipeline,
                              descriptors_state,
                              MESA_SHADER_COMPUTE);
        }


        
    //上传push_contants数据
	radv_flush_constants(cmd_buffer, VK_SHADER_STAGE_ALL_GRAPHICS);
        radv_foreach_stage(stage, stages) {
            shader = radv_get_shader(pipeline, stage);

            /* Avoid redundantly emitting the address for merged stages. */
            if (shader && shader != prev_shader) {
                radv_emit_userdata_address(cmd_buffer, pipeline, stage,
                               AC_UD_PUSH_CONSTANTS, va);

                prev_shader = shader;
            }
        }


}

```

4. 调用radv_emit_draw_packets发射radv_draw_info


radv_emit_draw_packets(struct radv_cmd_buffer *cmd_buffer,

该函数和si_emit_draw_packets 功能基本相同
流程如下:
如果是indirect绘制:
    下发PKT3_SET_BASE寄存器，该寄存器保存的即挽额indirect buffer bo 地址
如果不是indirect绘制:
    首先下发vtx_emit_num, vertex_offset, first_instance
    如果存在ibo，调用radv_cs_emit_draw_indexed_packet 通过PKT3_INDEX_2下发ibo地址
    如果不存在ibo,
        如果没有使用view_mask,直接调用radv_cs_emit_draw_packet通过PKT3_DRAW_INDEX_AUTO下发draw count参数
        如果使用view_mask，
            遍历view_mask 掩码值,
              调用radv_emit_view_index将mask的viewindex下发到user_data寄存器中
              再调用radv_cs_emit_draw_packet下发count



 
 
# radv_CmdEndRenderPass

	radv_cmd_buffer_resolve_subpass(cmd_buffer);

# radv_EndCommandBuffer

    



# radv_QueueSubmit

radv_amdgpu_winsys_cs_submit
    radv_amdgpu_winsys_cs_submit_fallback
    
在调用radv_amdgpu_winsys_cs_submit之前，首先通过radv_get_preamble_cs获取 initial_preamble_cs , initial_flush_preamble_cs, continue_preamble_cs，
initial_preamble_cs为queue->
内部会根据传入scratch_size, compute_scratch_size, esgs_ring_size, gsvs_ring_size决定是否要重新申请对应大小的bo,之后下发esgs, gsvsring寄存器参数,并设定queue 相关


radv_amdgpu_winsys_cs_submit_fallback:
1. 该函数对每条cs调用radv_amdgpu_cs_sumit进行下发


# radv_QueuePresentKHR

wsi_common_queue_present
      result = wsi->QueueSubmit(queue, 1, &submit_info, swapchain->fences[0]);

        // x11_queue_present
      result = swapchain->queue_present(swapchain,
                                        pPresentInfo->pImageIndices[i],
                                        region);
# radv_CreateXcbSurfaceKHR

VkIcdSurfaceXcb *surface;
    
wsi_create_xcb_surface




radv_subpass {
    uint32_t input_count;
    uint32_t color_count;
    struct radv_subpass_attachment* input_attachments;
    struct radv_subpass_attachment* color_attachments;
    struct radv_subpass_attachment* resolve_attachments;
    struct radv_subpass_attachment depth_stencil_sttachment;

    bool has_resolve;

    struct radv_subpass_barrier start_barrier;
    uint32_t view_mask;
    VkSampleCountFlagsBits max_sample_count;
}


# radv_CreateEvent

radv_event:
    struct radeon_winsys_bo *bo;
    uint64_t*  map;


event->bo = device->ws->buffer_create(device->ws, 8, 8,
                      RADEON_DOMAIN_GTT,
                      RADEON_FLAG_VA_UNCACHED | RADEON_FLAG_CPU_ACCESS | RADEON_FLAG_NO_INTERPROCESS_SHARING);

event->map = (uint64_t*)device->ws->buffer_map(event->bo);

# radv_CmdPipelineBarrier

radv_barrier_info :
    uint32_t eventCount;
    const VkEvent *pEvents;
    vkPipelineStageFlags srcStageMask;



VkPipelineColorBlendStateCreateInfo {
    VkStructureType sType;
    const void * pNext;
    VkPipelineColorBlendStateCreateFlags flags;
    VkBool32 logicOpEnable;
    VkLogicOp logicOp;
    const VkPippelineColorBlendAtachmentState *pAttachments;
    float  blendConstants[4];
}

VkPipelineColorBlendAttachmentState {
    VkBool32 blendEnable;
    VkBlendFactor srcColorBlendFactor;
    VkBlendFactor dstColorBlendFactor;
    VkBlendOp     colorBlendOp;
    VkBlendFactor srcAlphaBlendFactor;
    VkBlendFactor dstAlphaBlendFactor；
    VkBlendOp     alphaBlendOp;
    VkColorComponentFlags colorWriteMask;


}
VkPipelineDynamicStateCreateInfo * pDynamicState;

typedef struct VkPipelineDynamicStateCreateInfo 
    uint32_t dynamicStateCount;
    const VkDynamicState* pDynamicStates;

const void *pNext;
VkPipelineDynamicStateCreateInfo flags;
uint32_t dynamicStateCount;


enum VkDynamicState {
    viewport, 
    scissor,
    line_width,
    sample_locations;
}
    
typedef struct VkPipelineMultisampleStateCreateInfo {
    VkStructureType sType;
    const void *pNext;
    VkPipelineMulstisampleStateCreateFlags flags;
    VkSampleCountFlagBits rasterizationSamples;
    VkBool32 sampleShadingEnable;
    floawt minSampleShading;
    const VkSampleMask * pSampleMask;
    VkBool32 alphaToConverageEnable;
    VkBool32 alphaToOneEnable;
}

typdef struct VkRenderPassBeginInfo {
const void *pNext;
VkRenderPass renderPass;
VkFramebuffer framebuffer;
VkRect2D renderArea;
uint32_t clearValueCount;
VkClearValue * pClearValues;
}
struct radv_subpass {
    uint32_t input_count;
    uint32_t color_count;
    struct radv_subpass_attachment 
}

struct radv_subpass_attachment {

}

typedef struct VkAttachmentDescription {
    VKAttachmentDescriptionFlags flags;
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkAttachmentLoadOp loadOp;
    VKAttachmentStoreOp storeOp;
    VkAttachmentLoadOp stencilLoadOp;
    VkAttachmentStoreOp stencilStoreOp;
    VkImageLayout initialLayout;
    VkImageLayout finalLayout;
}

typedef struct VkSubpassDependency {
    uint32_t srcSubpass;
    uint32_t dstSubpass;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkAccessFlags srcAccessFlags,
    VkAccessFlags dstAccessFlags,
    VkDependencyFlags dependencyFlags
}

VkSubpassDescription {
    VkSubpassDescriptionFlags flags;
    VkPipelineBindPoint pipelineBindPoint;
    uint32_t inputAttachmentCount;
    const VkAttachmentReference * pInputAttachments;
    uint32_t colorAttachmentCount;
    const VkAttachmentReference* pColorAttachments;
    const VkAttachmentRefernce* pResolveAttachments;
    const VkAttachmentReference * pDepthStencilAttachnments;
    uint32_t preserverAttachmentCount;
    const uint32_t * pPreserverAttachments;
}

vkAttachmetnDescritpion {
    vkattachmentdescriptionflags flags;
    vkformatfoarmatg;
    vkformat
    vkattachnetdescriptionFlags flags;
    {

    }
    vksamplecountflagsbits samples;
    vksampleCountFlagsBits samples;
    vkattachmentloadop loadop
    vkattachmentstoreop storeop
    vkaloadop stencil loadopo
    vkastoreop stencilstoreop
    vkimagelayout initiallayout
    vkimagelayout finallayout


}
vksubpassdescritpin {
    subpassdescritpionfalgs flags;
    vkpipelinebindpoint pipelinebindpoint;
    uint32_t inputattachnmentcoutn;
    const vkattachmentreferencxe* inputattachemntm;
    uint32_t colorattachmentcount;
    const vkattachmentreference* colorattachmets;
    const vkattachmentreference* resolveattachmnet;
    const vkattachjmentreferce * depthstencilattac hemnt;
    uint32_t preserverattachmentcount;
    const uint32_t * pPreserverAttachments;
}
vk subpssdependency {
    uint32_t srcsubpass;
    uint32_t dstsubpass;
    vkpipelinestageflags srcstagmask;
    vkpipe;inestageflags dststagmask;
    vkaccessflags srcaccessmask;
    vkaccessflags dstaccessmask;
    vkdependency depencyFlags;
}
VkAcquireNextImageKHR{
    VkDevice device,
    VkSwapchainKHR swapchian,
    uint64_t timeout,
    VkSemaphore semaphore,
    VkFence fence ,
    uint32_t * pImageIndex)
}

VkPipelineStageFlags src_stage_mask;
vkaccessflags srcaccess_mask;
vkaccessflags dst_mask;
radv_subpass_barrier end_barrier;


ssytp
pNext
uint32_t stagecount



shaderstagestate pstages
vertexinpoutstateA*
inputassemblystate*
tessellationstate
viewportstate
rasterizationinof
colorblend
multisample
pipelinelayout
renderpass
dynamicstaqte
renderpass

subpass->view_mask;


radv_pipeline_init_dynamic_state(radv_pipeline* pipeline, 
states = needed_states;
radv_subpass *subpass = &pass->
radv_shader_variant *ps 


VkPipelineVertexInputStateCreateInfo {
    sStype
    pNext
    flags
    vertexbinddescriptioncount
    vertexbinddescirpions
}

struct radv_vertex


# radv_CreatePipelineCache

radv_pipeline_cache  

struct radv_pipeline_cache {
	struct radv_device *                          device;
	pthread_mutex_t                              mutex;

	uint32_t                                     total_size;
	uint32_t                                     table_size;
	uint32_t                                     kernel_count;
	struct cache_entry **                        hash_table;
	bool                                         modified;

	VkAllocationCallbacks                        alloc;
};


cache = vk_alloc2(&device->alloc, pAllocator,

radv_pipeline_cache_init(radv_pipeline_cache* cache, radv_device *device);


radv_pipeline_cache_load(struct radv_pipeline_cache *cache,
			 const void *data, size_t size)
radv_pipeline_cache_load(struct radv_pipeline_cache \*cache,

