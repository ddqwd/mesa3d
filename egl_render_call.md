
## 使用tc

amdgpu_winsys_create->
	amdgpu_device_initalize
	do_winsys_init ->
		ac_query_gpu_ifno
			-> amdgpu_query_info
			-> amdgpu_query_buffer_size_alignment
			-> amdgpu_query_hw_ip_info
			-> amdgpu_query_firmware_version
			-> amdgpu_query_sw_info
			-> amdgpu_query_gds_info

	amdgpu_bo_create
		-> amdgpu_create_bo 
			-> amdgpu_va_arange_alloc
			-> amdgpu_bo_va_op_raw
			-> amdgpu_bo_export

	radeonsi_create_create 
		-> si_create_context
			-> amdgpu_ctx_create
				-> amdgpu_cs_ctx_create
				-> amdgpu_bo_alloc
				-> amdgpu_bo_cpu_map

			-> amdgpu_cs_create 
				-> amdgpu_cs_chunk_fence_info_to_data
				-> amdgpu_get_new_ib
					-> amdgpu_ib_new_buffer
						->amdgpu_bo_create
							-> pb_slab_alloc
								-> amdgpu_bo_slab_alloc
									-> amdgpu_bo_create
										-> amdgpu_create_bo
											-> amdgpu_va_arange_alloc
											-> amdgpu_bo_va_op_raw
											-> amdgpu_bo_export
											-> amdgpu_bo_cpu_map
								
			
			-> si_init_config 	
				-> si_buffer_subdata
					-> si_buffer_transfer_map
						-> amdgpu_bo_map
							-> amdgpu_cpu_map
				
			-> pipe_buffer_write
				-> si_buffer_subdata
					-> si_buffer_transfer_map
						-> amdgpu_bo_map
							-> amdgpu_cpu_map

	

egl_create_context
	dri2_create_context
		driCreateContextAtrribs
			dri_create_context
				st_api_create_context
					si_pipe_create_context
						si_create_context
							amdgpu_ctx_create
								amdgpu_cs_ctx_create
								amdgpu_bo_alloc
								amdgpu_bo_cpu_map	
							amdgpu_cs_create 

							amdgpu_bo_map
								amdgpu_bo_cpu_map


							si_init_state_functions 
								si_init_config
									si_buffer_subdata


					st_create_context
						st_create_context_priv
							vbo_use_buffer_objects
								st_bufferobj_data
									bufferobj_data
										si_buffer_create
											si_alloc_resource
												amdgpu_bo_create


eglMakeCurrenet
	dri2_make_current
		driBindContext
			dri_make_context
				st_api_make_current
					st_framebuffer_validate
						dri2_allocate_textures
							dri_image_drawable_get_buffers
								dri2_drm_image_get_buffers
									get_back_bo
										gbm_bo_create
											gbm_dri_bo_create
												dri2_create_image
													dri2_create_image_common
														si_texture_create
															si_texture_create_object
																si_alloc_resource
			
												dri2_query_image
													si_texture_get_handle
														amdgpu_buffer_set_metadata
															amdgpu_bo_set_metadata
														amdgpu_bo_get_handle
															amdgpu_bo_export


_mesa_buffer_data
	buffer_data
		st_bufferobj_data
			pipe_buffer_write
				tc_buffer_subdata
					tc_buffer_subdata
						tc_transfer_map
							si_buffer_transfer_map
								
													
	
_mesa_TexImage2D
	teximage_err
		st_TexImage
			st_TexImage
				st_AllocTextureImageBuffer
					guess_and_alloc_texture
						st_texture_create
							si_texture_create
								si_texture_create_object
									si_alloc_resource



_mesa_RenderbufferStorage
	renderbuffer_storage
		_mesa_renderbuffer_storage
			st_renderbuffer_alloc_storage
				si_texture_create
					si_texture_create_object
						si_screen_clear_buffer
							si_flush_from_st
								si_flush_dma_cs
									amdgpu_cs_flush
amdgpu_cs_flush 
	amdgpu_cs_submit_ib
		amdgp_cs_chunk_fence_to_dep
		amdgpu_cs_submit_raw




eglSwapBuffers
	dri2_drm_swap_buffers
		dri_flush
			st_context_flush
				--> si_texture_transfer_map
						si_texture_create
						amdgpu_bo_amp
						
				--> si_clear
					si_draw_rectangle
						si_draw_vbo

				--> si_draw_vbo
					si_update_shaders
						si_shader_select_with_key
							si_build_shader_variant
								si_shader_create
									si_shader_binary_upload
										amdgpu_bo_map

				--> si_flush_fromt_st
					si_flush_dma_cs
						amdgpu_cs_flush
					si_flush_gfx_cs
						amdgpu_cs_flush

						

	






## 不使用tc

gbm_create_devicee->
amdgpu_winsys_create->
	amdgpu_device_initalize
	do_winsys_init ->
		ac_query_gpu_ifno
			-> amdgpu_query_info
			-> amdgpu_query_buffer_size_alignment
			-> amdgpu_query_hw_ip_info
			-> amdgpu_query_firmware_version
			-> amdgpu_query_sw_info
			-> amdgpu_query_gds_info
//
//	amdgpu_bo_create
//		-> amdgpu_create_bo 
//			-> amdgpu_va_arange_alloc
//			-> amdgpu_bo_va_op_raw
//			-> amdgpu_bo_export

	radeonsi_screen_create 
		-> si_create_context
			pipe_buffer_create
				si_alloc_resource
					amdgpu_bo_create

			amdgpu_ctx_create
			 -> amdgpu_cs_ctx_create
			 -> amdgpu_bo_alloc
			 -> amdgpu_bo_cpu_map

			amdgpu_cs_create 
			 -> amdgpu_cs_chunk_fence_info_to_data
			 -> amdgpu_get_new_ib
			 	-> amdgpu_ib_new_buffer
			 		->amdgpu_bo_create
			 			-> pb_slab_alloc
			 				-> amdgpu_bo_slab_alloc
			 					-> amdgpu_bo_create
			 						-> amdgpu_create_bo
			 							-> amdgpu_va_arange_alloc
			 							-> amdgpu_bo_va_op_raw
			 							-> amdgpu_bo_export
			 							-> amdgpu_bo_cpu_map
			 				
			
			si_init_config 	
			 -> si_buffer_subdata
			 	-> si_buffer_transfer_map
			 		-> amdgpu_bo_map
			 			-> amdgpu_cpu_map
			 
			pipe_buffer_write
				-> si_buffer_subdata
					-> si_buffer_transfer_map
						-> amdgpu_bo_map
							-> amdgpu_cpu_map

	

egl_create_context
	dri2_create_context
		driCreateContextAtrribs
			dri_create_context
				st_api_create_context
					si_pipe_create_context
						si_create_context
							amdgpu_ctx_create
								amdgpu_cs_ctx_create
								amdgpu_bo_alloc
								amdgpu_bo_cpu_map	
							amdgpu_cs_create 

							amdgpu_bo_map
								amdgpu_bo_cpu_map


							si_init_state_functions 
								si_init_config
									si_buffer_subdata


					st_create_context
						st_create_context_priv
							vbo_use_buffer_objects
								st_bufferobj_data
									bufferobj_data
										si_buffer_create
											si_alloc_resource
												amdgpu_bo_create


eglMakeCurrenet
	dri2_make_current
		driBindContext
			dri_make_context
				st_api_make_current
					st_framebuffer_validate
						dri2_allocate_textures
							dri_image_drawable_get_buffers
								dri2_drm_image_get_buffers
									get_back_bo
										gbm_bo_create
											gbm_dri_bo_create
												dri2_create_image
													dri2_create_image_common
														si_texture_create
															si_texture_create_object
																si_alloc_resource
			
												dri2_query_image
													si_texture_get_handle
														amdgpu_buffer_set_metadata
															amdgpu_bo_set_metadata
														amdgpu_bo_get_handle
															amdgpu_bo_export


_mesa_buffer_data
	buffer_data
		st_bufferobj_data
			bufferobj_data
				pipe_buffer_write
					si_buffer_subdata
						si_buffer_transfer_map
							amdgpu_bo_map

								
													
	
_mesa_TexImage2D
	teximage_err
		st_TexImage
			st_TexImage
				st_AllocTextureImageBuffer
					guess_and_alloc_texture
						st_texture_create
							si_texture_create
								si_texture_create_object
									si_alloc_resource



_mesa_RenderbufferStorage
	renderbuffer_storage
		_mesa_renderbuffer_storage
			st_renderbuffer_alloc_storage
				si_texture_create
					si_texture_create_object
						si_screen_clear_buffer
							si_flush_from_st
								si_flush_dma_cs
									amdgpu_cs_flush



amdgpu_cs_flush 
	amdgpu_cs_submit_ib
		amdgp_cs_chunk_fence_to_dep
		amdgpu_cs_submit_raw





glClear
	st_clear
		si_clear
			util_blitter_clear
				util_blitter_clear_custom
					si_draw_rectangle
						si_draw_vbo
							si_update_shaders
								--> si_shader_binary_upload		
									amdgpu_bo_map


gl_draw_scene (glDrawElements)
	_mesa_exec_DrawElements
		_mesa_validated_drawrangeelements
			_mesa_validated_drawraneelemts
				st_draw_vbo
					u_vbuf_draw_vbo
						si_draw_vbo
							si_update_shaders
								si_shader_select
									si_shader_select_with_key 
										si_build_shader_variant
											si_shader_create
												si_shader_binary_upload
													amdgpu_bo_cpu_unmap

eglSwapBuffers
	dri2_drm_swap_buffers
		dri_flush
			st_context_flush
				--> si_texture_transfer_map
						si_texture_create
						amdgpu_bo_amp
						
				--> si_clear
					si_draw_rectangle
						si_draw_vboRADEON_ALL_BOS=true

				--> si_draw_vbo
					si_update_shaders
						si_shader_select_with_key
							si_build_shader_variant
								si_shader_create
									si_shader_binary_upload
										amdgpu_bo_map

				--> si_flush_fromt_st
					si_flush_dma_cs
						amdgpu_cs_flush
					si_flush_gfx_cs
						amdgpu_cs_flush

## 如果启用GALLIUM_DEBUG=flush，则在glDraw函数调用时flush gfx cs

siroywegpu_cs_query_fence_status

