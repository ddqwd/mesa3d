
##  初步分析

对于radeon_dri驱动  
shader编译过程     
source -> nir->tgsi  
tgsi-> llvm ir  
llvm ir ->  GCN   
分别对应llvm的前端 后端   


## GLSL 调试环境变量

设置MESA_GLSL 环境变量， 有关选项如下代码。

```c

#define GLSL_DUMP      0x1  /**< 将着色器转储到标准输出 */
#define GLSL_LOG       0x2  /**< 将着色器写入文件 */
#define GLSL_UNIFORMS  0x4  /**< 打印glUniform调用 */
#define GLSL_NOP_VERT  0x8  /**< 强制使用无操作的顶点着色器 */
#define GLSL_NOP_FRAG 0x10  /**< 强制使用无操作的片段着色器 */
#define GLSL_USE_PROG 0x20  /**< 记录glUseProgram调用 */
#define GLSL_REPORT_ERRORS 0x40  /**< 打印编译错误 */
#define GLSL_DUMP_ON_ERROR 0x80 /**< 编译错误时将着色器转储到标准错误输出 */
#define GLSL_CACHE_INFO 0x100 /**< 打印有关着色器缓存的调试信息 */
#define GLSL_CACHE_FALLBACK 0x200 /**< 强制使用着色器缓存的备用路径 */

/**
 * Return mask of GLSL_x flags by examining the MESA_GLSL env var.
 */
GLbitfield
_mesa_get_shader_flags(void)
{
   GLbitfield flags = 0x0;
   const char *env = getenv("MESA_GLSL");

   if (env) {
      if (strstr(env, "dump_on_error"))
         flags |= GLSL_DUMP_ON_ERROR;
      else if (strstr(env, "dump"))
         flags |= GLSL_DUMP;
      if (strstr(env, "log"))
         flags |= GLSL_LOG;
      if (strstr(env, "cache_fb"))
         flags |= GLSL_CACHE_FALLBACK;
      if (strstr(env, "cache_info"))
         flags |= GLSL_CACHE_INFO;
      if (strstr(env, "nopvert"))
         flags |= GLSL_NOP_VERT;
      if (strstr(env, "nopfrag"))
         flags |= GLSL_NOP_FRAG;
      if (strstr(env, "uniform"))
         flags |= GLSL_UNIFORMS;
      if (strstr(env, "useprog"))
         flags |= GLSL_USE_PROG;
      if (strstr(env, "errors"))
         flags |= GLSL_REPORT_ERRORS;
   }

   return flags;
}

```

##  Shader使用的相关步骤

使用OpenGL的着色器涉及以下主要步骤：

1. 创建着色器对象：使用`glCreateShader`函数创建一个新的着色器对象，并指定着色器类型，如顶点着色器（`GL_VERTEX_SHADER`）或片段着色器（`GL_FRAGMENT_SHADER`）。

2. 设置着色器源码：使用`glShaderSource`函数将源代码绑定到着色器对象上。源代码应该是一个字符串数组，包含着色器程序的源码。

3. 编译着色器：使用`glCompileShader`函数对着色器对象进行编译。OpenGL将源代码编译成机器可执行的形式。可以使用`glGetShaderiv`和`glGetShaderInfoLog`来检查编译状态和获取编译日志。

4. 创建着色器程序：使用`glCreateProgram`函数创建一个新的着色器程序对象。

5. 将着色器附加到着色器程序：使用`glAttachShader`函数将编译好的着色器对象附加到着色器程序对象上。

6. 链接着色器程序：使用`glLinkProgram`函数链接着色器程序。链接阶段将把附加的着色器对象合并到单个可执行程序中。可以使用`glGetProgramiv`和`glGetProgramInfoLog`来检查链接状态和获取链接日志。

7. 使用着色器程序：使用`glUseProgram`函数激活着色器程序，使其成为当前OpenGL上下文的一部分。

8. 设置着色器属性和uniform变量：使用`glVertexAttribPointer`和`glEnableVertexAttribArray`等函数设置顶点属性，并使用`glUniform`函数设置uniform变量的值。

9. 绘制：使用`glDrawArrays`或`glDrawElements`等函数执行绘制命令，使用当前激活的着色器程序渲染图形。

10. 清理和销毁：在不再需要时，使用`glDeleteShader`和`glDeleteProgram`等函数删除着色器对象和着色器程序对象，释放资源。

## OpenGL api 调用流程 

```
glCreateShader 
    |-`_mesa_CreateShader`  
        |- glCompileShader   
            |-`_mesa_CompileShader`   
               |-`_mesa_compile_shader`   
                   |- `_mesa_glsl_compile_shader`  

```
上述流程只写到`_mesa_glsl_compile_shader`,对于glsl-> nir这部分未涉及到相关驱动处理，所以采用glsl_compiler直接进行shader source源测试分析，而glsl_compiler可直接 调用到`_mesa_compiler_shader `。

生成glsl_compiler可执行文件配置

```
meson configure -Dtools="glsl"

```
如此可在compiler/glsl目录下看到glsl_compiler 

直接执行可看到

```bash

usage: ./glsl_compiler [options] <file.vert | file.tesc | file.tese | file.geom | file.frag | file.comp>

Possible options are:
    --dump-ast
    --dump-hir
    --dump-lir
    --dump-builder
    --link
    --just-log
    --lower-precision
    --version (mandatory)

```
- `--dump-ast`：将抽象语法树（AST）的信息打印出来。
- `--dump-hir`：将高级中间表示（HIR）的信息打印出来。
- `--dump-lir`：将低级中间表示（LIR）的信息打印出来。
- `--dump-builder`：将构建器的信息打印出来。
- `--link`：链接多个着色器文件。
- `--just-log`：仅输出日志信息，而不进行实际编译。
- `--lower-precision`：将浮点精度降低。
- `--version`：指定GLSL版本（必选参数）。


## 测试 vs shader test.vert

```glsl
 #version 430 core

 layout (location = 0) in vec3 position;
 layout (location = 1) in vec3 color;

 out vec3 fragmentColor;

 void main()
 {
     gl_Position = vec4(position, 1.0);
         fragmentColor = color;
 }
```

## glsl_compiler 主要调用流程

```
glsl_compiler main -> standalone_compile_shader -> compile_shader -> link_shader -> print_relavant

```
下面对主要函数进行分析 

`standalone_compile_shaderi`


1. `initialize_context()`: 初始化 GLSL 上下文。这个函数会根据传入的 API 参数，设定上下文的基本参数，比如 API 版本，API 类型（OpenGL 或 OpenGL ES）等。

2. `rzalloc()`: 为 gl_shader_program 结构体分配内存，并将内存初始化为0。这是 ralloc 库中的一个函数，ralloc 库是一个用于 GLSL 中内存管理的库。

3. `load_text_file()`: 从文件系统中加载一个文本文件。在这个上下文中，它被用来加载 GLSL 着色器源码。

4. `compile_shader()`: 编译一个给定的 GLSL 着色器。具体的编译步骤在这个函数内部进行。

5. `_mesa_clear_shader_program_data()`: 清理着色器程序数据，为链接着色器做准备。

6. `link_shaders()`: 链接一组 GLSL 着色器，生成一个可用的着色器程序。

7. `link_intrastage_shaders()`: 如果不进行完整的链接，就进行阶段内链接。这种情况下，只链接同一阶段的着色器。

8. `do_function_inlining()`: 进行函数内联优化。这是一种常见的编译器优化技术，可以消除函数调用的开销。

9. `do_common_optimization()`: 执行常见的优化操作，包括常数折叠，常数传播，死代码消除等。

10. `add_neg_to_sub_visitor()`, `dead_variable_visitor`: 这两个都是对 IR 进行访问的访问者，分别用于添加负数到减法操作和删除死变量。

11. `_mesa_print_builder_for_ir()`: 如果选项要求，就打印着色器的中间表示（IR）。



关于compile_shader->`_mesa_compiler_shader`


`_mesa_compiler_shader `

1. 在这个函数内，首先调用`disk_cache_compute_key()` 和 `disk_cache_has_key()` 来检查缓存。

2. 如果缓存不存在，那么它会创建一个新的`_mesa_glsl_parse_state`对象，并调用`glcpp_preprocess()`进行预处理。

3. 如果预处理成功，接着进行词法和语法解析，分别通过`_mesa_glsl_lexer_ctor()`, `_mesa_glsl_parse()`, `_mesa_glsl_lexer_dtor()`和`do_late_parsing_checks()`完成。

4. 然后，如果指定了打印AST，会遍历并打印所有的AST节点。

5. 接着，会通过`_mesa_ast_to_hir()`将AST转换为HIR，并调用`validate_ir_tree()`来验证。

6. 如果没有错误，接着会调用`set_shader_inout_layout()`来设定着色器的输入输出布局。

7. 最后，如果没有错误并且着色器IR非空，会执行`assign_subroutine_indexes()`和`lower_subroutine()`对子程序进行处理，并可能调用`opt_shader_and_create_symbol_table()`来优化着色器并创建符号表。


`link_shaders`的主要任务是确保输入着色器集合可以形成一个完整和一致的着色器程序。

1. 检查是否至少有一个着色器连接到程序，如果没有，则链接失败。

2. `shader_cache_read_program_metadata`: 如果启用了着色器缓存，这个函数会尝试从缓存中读取程序元数据，避免了不必要的链接过程。

3. 将着色器按照其类型（顶点，片段，几何，计算等）进行分类，储存在 `shader_list` 中。

4. `validate_vertex_shader_executable`, `validate_tess_eval_shader_executable`, `validate_geometry_shader_executable`, `validate_fragment_shader_executable`: 这些函数对每个阶段的着色器进行验证，确保着色器在运行时不会产生错误。

5. `cross_validate_outputs_to_inputs`: 对比每个阶段的输出和下一个阶段的输入，确保一致性。如果发现阶段间接口不匹配，链接会失败。

6. `lower_named_interface_blocks`: 对每个阶段的接口块进行降级处理。

7. `lower_discard_flow`: 对于 GLSL 1.30 及以上版本，以及 GLSL ES 3.00 及以上版本，会对每个阶段的 discard 语句进行降级处理。

8. `disable_varying_optimizations_for_sso`: 如果是单独的着色器程序，关闭变化量优化。

9. `interstage_cross_validate_uniform_blocks`: 对 UBOs 和 SSBOs 进行跨阶段验证。

10. `linker_optimisation_loop`: 在为属性，uniforms 和变量分配存储之前进行优化。后续的优化可能会使一些变量变得无用。

11. `validate_sampler_array_indexing`: 对于一些特殊情况，检查并验证我们是否允许使用循环诱导变量进行采样器数组索引。

12. `validate_geometry_shader_emissions`: 检查并验证几何着色器中的流发射。

13. `store_fragdepth_layout`: 存储 FragDepth 布局。

14. `link_varyings_and_uniforms`: 链接所有的着色器变量和uniforms，并为它们分配存储空间。

15. `optimize_swizzles`: 链接变量可能会生成一些额外的、无用的切换，这个函数会对这些切换进行优化。

16. `validate_ir_tree`: 进行最后的验证步骤，确保 IR 在链接后没有被破坏。



--dump-builder的输出 

```c++
ir_variable *const r0001 = new(mem_ctx) ir_variable(glsl_type::vec4_type, "gl_Position", ir_var_shader_out);
body.emit(r0001);
ir_variable *const r0002 = new(mem_ctx) ir_variable(glsl_type::vec3_type, "position", ir_var_shader_in);
body.emit(r0002);
ir_variable *const r0003 = new(mem_ctx) ir_variable(glsl_type::vec3_type, "color", ir_var_shader_in);
body.emit(r0003);
ir_variable *const r0004 = new(mem_ctx) ir_variable(glsl_type::vec3_type, "fragmentColor", ir_var_shader_out);
body.emit(r0004);
ir_function_signature *
main(void *mem_ctx, builtin_available_predicate avail)
{
   ir_function_signature *const sig =
      new(mem_ctx) ir_function_signature(glsl_type::void_type, avail);
   ir_factory body(&sig->body, mem_ctx);
   sig->is_defined = true;

   ir_variable *const r0005 = body.make_temp(glsl_type::vec4_type, "vec_ctor");
   body.emit(assign(r0005, body.constant(1.000000f), 0x08));

   ir_swizzle *const r0006 = swizzle(r0002, MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_X), 3);
   body.emit(assign(r0005, r0006, 0x07));

   body.emit(assign(r0001, r0005, 0x0f));

   body.emit(assign(r0004, r0003, 0x07));

   return sig;
}
```
其中细节还需进一步探讨
