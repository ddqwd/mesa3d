
_mesa_uniform
	validate_uniform
		validate_uniform_parameters --->  recognize number is whether negative or not
			
	copy_uniforms_to_storage




glUniform必须在program link之后，这样才可获取gl_shader_program的NumUniformRemapTable.

关于NumUniformRemapTable:
应该是在link_setup_uniform_remap_tables 函数内部设置

link_shaders
	link_varyings_and_uniforms
		link_and_validate_uniforms
			link_invalidate_variable_locations

			link_assign_uniform_locations
				link_assign_uniform_storage
					link_set_uniform_initializers
						link_setup_uniform_remap_tables
