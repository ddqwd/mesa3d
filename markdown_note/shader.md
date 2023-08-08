
link_program 


#0  si_create_shader_selector (ctx=0x555555589a50, state=0x555555aad310) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:2216
#1  0x00007ffff5e70e5e in st_create_vp_variant (key=0x7fffffffdea0, stvp=0x555555a8de30, st=<optimized out>) at ../src/mesa/state_tracker/st_program.c:605
#2  st_get_vp_variant (key=0x7fffffffdea0, stvp=0x555555a8de30, st=<optimized out>) at ../src/mesa/state_tracker/st_program.c:629
#3  st_get_vp_variant (st=<optimized out>, stvp=0x555555a8de30, key=0x7fffffffdea0) at ../src/mesa/state_tracker/st_program.c:614
#4  0x00007ffff5e7233d in st_precompile_shader_variant (st=st@entry=0x555555a86330, prog=prog@entry=0x555555a8de30) at ../src/mesa/state_tracker/st_program.c:1936
#5  0x00007ffff5ee16dd in st_program_string_notify (ctx=<optimized out>, target=<optimized out>, prog=0x555555a8de30) at ../src/mesa/state_tracker/st_cb_program.c:250
#6  0x00007ffff5f04e24 in st_link_shader (ctx=0x555555a5d230, prog=0x555555a93840) at ../src/mesa/program/program.h:141
#7  0x00007ffff5dc1cd9 in _mesa_glsl_link_shader (ctx=ctx@entry=0x555555a5d230, prog=prog@entry=0x555555a93840) at ../src/mesa/program/ir_to_mesa.cpp:3174
#8  0x00007ffff5d21b1d in link_program (no_error=<optimized out>, shProg=<optimized out>, ctx=<optimized out>) at ../src/mesa/main/shaderapi.c:1206
si_get_shader_name (shader=0x7fffe4000b20, processor=0) at ../src/gallium/drivers/radeonsi/si_shader.c:5500
#1  0x00007ffff5ae5700 in si_compile_tgsi_shader (sscreen=sscreen@entry=0x55555561e5b0, compiler=compiler@entry=0x55555561ecc8, shader=shader@entry=0x7fffe4000b20, debug=debug@entry=0x555555a8e4f0)
    at ../src/gallium/drivers/radeonsi/si_shader.c:6742
#2  0x00007ffff5aa16fb in si_init_shader_selector_async (job=job@entry=0x555555a8e4d0, thread_index=thread_index@entry=0) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:2088
#3  0x00007ffff600aec3 in util_queue_thread_func (input=input@entry=0x55555561a2c0) at ../src/util/u_queue.c:286
#4  0x00007ffff600a9a7 in impl_thrd_routine (p=<optimized out>) at ../src/../include/c11/threads_posix.h:87
#5  0x00007ffff6d89fa3 in start_thread (arg=<optimized out>) at pthread_create.c:486
#6  0x00007ffff790f06f in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
#9  link_program_error (shProg=0x555555a93840, ctx=0x555555a5d230) at ../src/mesa/main/shaderapi.c:1286
#10 link_program_error (ctx=0x555555a5d230, shProg=0x555555a93840) at ../src/mesa/main/shaderapi.c:1284
#11 0x000055555555654c in linkProgram(unsigned int, unsigned int) ()
#12 0x0000555555556704 in init() ()
#13 0x0000555555556840 in main ()



这段代码是一个用于异步初始化着色器选择器的函数 `si_init_shader_selector_async`。让我们来分析它的流程：

1. 首先，函数获取 `sel`（着色器选择器）的指针和相关的屏幕信息。

2. 然后，函数进行一些断言检查，确保调试相关的设置正确。

3. 接下来，函数检查是否使用了单片式着色器。如果不使用单片式着色器，则会创建一个 `si_shader` 结构体的实例，并对其进行初始化。

4. 分配并初始化了一个 `shader` 结构体，并初始化了其中的一些成员变量。

5. 设置 `shader` 的选择器为当前的 `sel`。

6. 通过调用 `si_parse_next_shader_property` 函数，解析着色器的属性。

7. 如果 `sel` 中有 tokens 或 nir，那么通过调用 `si_get_ir_binary` 函数获取着色器的 IR 二进制数据。

8. 接下来，函数会尝试从着色器缓存中加载着色器。如果成功加载，就无需编译，直接使用缓存中的着色器。否则，需要进行编译。

9. 如果需要编译着色器，会调用 `si_compile_tgsi_shader` 函数来编译着色器。

10. 如果成功编译了着色器，会将它插入着色器缓存中。

11. 将 `shader` 设置为 `sel` 的主要着色器部分。

12. 如果非单片式着色器启用，会对一些已经转换为 `DEFAULT_VAL` 的输出进行处理，以避免在后续的着色器优化过程中尝试消除最终着色器中不存在的输出。

13. 如果 `sel` 的类型是 GEOMETRY 类型，会生成并设置一个 GS COPY 着色器。

总的来说，这个函数的主要作用是异步初始化着色器选择器。它根据不同的条件来选择是从缓存中加载着色器还是进行编译，然后设置相应的着色器部分，并处理一些特定情况下的输出。


这段代码是用于编译 TGSI 格式的着色器的函数 `si_compile_tgsi_shader`。让我们来逐步分析它：

1. 首先，函数从输入参数中获取着色器选择器 (`shader->selector`)、屏幕信息 (`sscreen`)、LLVM 编译器 (`compiler`) 和调试回调 (`debug`)。

2. 如果满足条件，则在进行 TGSI->LLVM 转换之前，将 TGSI 代码转储到控制台。这可以通过调用 `si_can_dump_shader` 函数来检查是否满足转储条件。

3. 初始化着色器上下文 (`ctx`)，并使用 `si_init_shader_ctx` 函数设置相关信息。

4. 清空着色器信息结构体中的 `vs_output_param_offset` 数组。

5. 设置着色器的 `uses_instanceid` 标志位，指示是否使用了 `instanceid`。

6. 调用 `si_compile_tgsi_main` 函数来执行 TGSI->LLVM 的转换，并生成主要的 LLVM 函数。

7. 如果着色器是单片式（`is_monolithic`），且类型为顶点着色器（`PIPE_SHADER_VERTEX`），则根据需要生成顶点着色器的 prolog 和 epilog，并构建包装函数。

8. 如果着色器是单片式且类型为细分控制着色器（`PIPE_SHADER_TESS_CTRL`），则根据需要生成细分控制着色器的 epilog，并构建包装函数。在 GFX9 架构之后的硬件上，还会生成作为细分控制着色器的顶点着色器（LS）的主要部分，并构建包装函数。

9. 如果着色器是单片式且类型为几何着色器（`PIPE_SHADER_GEOMETRY`），则根据需要生成几何着色器的 prolog，并构建包装函数。在 GFX9 架构之后的硬件上，还会生成作为几何着色器的顶点着色器（ES）的主要部分，并构建包装函数。

10. 如果着色器是单片式且类型为片段着色器（`PIPE_SHADER_FRAGMENT`），则根据需要生成片段着色器的 prolog 和 epilog，并构建包装函数。

11. 对生成的 LLVM 模块进行优化。

12. 对顶点着色器的输出进行优化处理。

13. 如果需要进行调试或转储着色器，会记录着色器的私有内存 VGPR 数量。

14. 确保输入参数是指针而不是整数。

15. 将 LLVM 模块编译成字节码。

16. 验证计算着色器的 SGPR 和 VGPR 使用情况，以检测编译器的错误。如果超出硬件限制，则输出错误信息。

17. 对于计算着色器，计算最大的 SIMD 波数。

18. 计算片段着色器的输入 VGPR 数量。

19. 计算着色器的最大 SIMD 波数，并进行一些统计和调试输出。
这段代码是 `si_compile_tgsi_main` 函数，它是 `si_compile_tgsi_shader` 函数的辅助函数，用于处理特定类型的 TGSI 着色器。让我们逐步分析它：

1. 首先，函数从上下文中获取着色器 (`ctx->shader`) 和着色器选择器 (`ctx->shader->selector`)。

2. 根据上下文的类型 (`ctx->type`)，根据不同的着色器类型，设置不同的函数指针和功能：
   - 顶点着色器 (`PIPE_SHADER_VERTEX`)：设置加载输入函数 (`ctx->load_input`) 和发射输出函数 (`ctx->abi.emit_outputs`)。
   - 细分控制着色器 (`PIPE_SHADER_TESS_CTRL`)：设置输入和输出的数据获取函数 (`bld_base->emit_fetch_funcs`)，以及加载细分曲面参数和输出细分控制着色器数据的函数。
   - 细分评估着色器 (`PIPE_SHADER_TESS_EVAL`)：设置输入数据获取函数，加载细分曲面参数和坐标的函数，以及发射输出函数。
   - 几何着色器 (`PIPE_SHADER_GEOMETRY`)：设置输入数据获取函数，加载输入数据的函数，发射顶点和图元的函数，以及发射输出函数。
   - 片段着色器 (`PIPE_SHADER_FRAGMENT`)：设置加载输入函数、发射输出函数、插值参数查找函数、加载样本位置的函数、加载样本屏蔽掩码的函数和发射丢弃指令的函数。
   - 计算着色器 (`PIPE_SHADER_COMPUTE`)：设置加载本地工作组大小的函数。

3. 创建 LLVM 函数。

4. 预加载环形缓冲区。

5. 对于 GFX9 架构的合并着色器：
   - 如果着色器不是单片式 (`!shader->is_monolithic`)，且满足一定条件，则设置 EXEC 寄存器，以便于在片段着色器或顶点着色器的主函数中使用。
   - 如果是细分控制着色器 (`PIPE_SHADER_TESS_CTRL`) 或几何着色器 (`PIPE_SHADER_GEOMETRY`)，则创建一个 if 语句块，用于执行屏障之前的代码。
   - 如果是细分控制着色器 (`PIPE_SHADER_TESS_CTRL`) 或几何着色器 (`PIPE_SHADER_GEOMETRY`)，则在块内执行屏障。

6. 对于细分控制着色器 (`PIPE_SHADER_TESS_CTRL`)，如果细分曲面参数在所有调用中都被定义，则为每个细分曲面参数创建 LLVM 寄存器。

7. 对于几何着色器 (`PIPE_SHADER_GEOMETRY`)，为每个输出顶点创建 LLVM 寄存器。

8. 如果需要在丢弃指令之后执行正确的导数计算，则为推迟执行的丢弃指令创建 LLVM 寄存器，并将其初始化为 `true`。

9. 根据着色器选择器的信息，将 TGSI 代码或 NIR 中间表示翻译为 LLVM 代码。

10. 使用 `si_llvm_build_ret` 函数构建返回语句。

11. 如果成功翻译，则返回 `true`，否则返回 `false`。

这个函数的作用是根据不同类型的 TGSI 着色器设置相应的函数指针，并将 TGSI 或 NIR 着色器代码翻译为 LLVM 代码。它还进行一些特定类型的着色器的预处理和设置。
20. 返回编译结果，0 表示成功，-1 表示失败。

总的来说，这个函数的作用是编译 TGSI 格式的着色器为字节码，并进行优化和验证。它根据着色器的类型和单片式标志生成不同的着色器部分，并构建包装函数。





eakpoint 9 at 0x7ffff5ae5520: file ../src/gallium/drivers/radeonsi/si_shader.c, line 6751.
(gdb) r
The program being debugged has been started already.
Start it from the beginning? (y or n) y
Starting program: /home/shiji/mydemos/a.out 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
loader_gbretyfor id : No such file or directory
dirver = radeonsi
loader_gbretyfor id : No such file or directory
dirver = radeonsi
[New Thread 0x7ffff1d16700 (LWP 30895)]
[New Thread 0x7ffff1515700 (LWP 30896)]
[New Thread 0x7ffff0d14700 (LWP 30897)]
[New Thread 0x7fffebfff700 (LWP 30898)]
[New Thread 0x7fffeb7fe700 (LWP 30899)]
[New Thread 0x7fffeaffd700 (LWP 30900)]
[New Thread 0x7fffea7fc700 (LWP 30901)]
[New Thread 0x7fffe9ffb700 (LWP 30902)]
[New Thread 0x7fffe97fa700 (LWP 30903)]
[New Thread 0x7fffe8ff9700 (LWP 30904)]
[New Thread 0x7fffc7fff700 (LWP 30905)]
[New Thread 0x7fffc77fe700 (LWP 30906)]
[New Thread 0x7fffc6ffd700 (LWP 30907)]
[New Thread 0x7fffc67fc700 (LWP 30908)]
: GLSL source for vertex shader 1:
: #version 430
in vec3 pos;void main() {gl_Position = vec4(pos, 1);}
: GLSL IR for shader 1:
(
(declare (location=17 shader_out ) (array float 0) gl_ClipDistance)
(declare (location=12 shader_out ) float gl_PointSize)
(declare (location=0 shader_out ) vec4 gl_Position)
(declare (shader_in ) vec3 pos)
( function main
  (signature void
    (parameters
    )
    (
      (declare (temporary ) vec4 vec_ctor)
      (assign  (w) (var_ref vec_ctor)  (constant float (1.000000)) ) 
      (assign  (xyz) (var_ref vec_ctor)  (var_ref pos) ) 
      (assign  (xyzw) (var_ref gl_Position)  (var_ref vec_ctor) ) 
    ))

)

)
: 

: GLSL source for fragment shader 2:
: #version 430
out vec4 FragColor;
void main() {FragColor = vec4(1, 0, 0, 1);}
: GLSL IR for shader 2:
(
(declare (location=23 shader_in flat) int gl_ViewportIndex)
(declare (location=22 shader_in flat) int gl_Layer)
(declare (location=21 shader_in flat) int gl_PrimitiveID)
(declare (location=25 shader_in ) vec2 gl_PointCoord)
(declare (location=17 shader_in ) (array float 0) gl_ClipDistance)
(declare (shader_out ) vec4 FragColor)
( function main
  (signature void
    (parameters
    )
    (
      (assign  (xyzw) (var_ref FragColor)  (constant vec4 (1.000000 0.000000 0.000000 1.000000)) ) 
    ))

)

)
: 


Thread 1 "a.out" hit Breakpoint 6, 0x00007ffff7fa7880 in glLinkProgramARB () from /home/shiji/mesa-18.3.6/build/install/lib/x86_64-linux-gnu/libGL.so.1
(gdb) c
Continuing.

Thread 1 "a.out" hit Breakpoint 7, link_program (no_error=false, shProg=0x555555aa8a60, ctx=0x555555a5d1f0) at ../src/mesa/main/shaderapi.c:1286
1286	   link_program(ctx, shProg, false);
(gdb) c
Continuing.

Thread 1 "a.out" hit Breakpoint 7, link_program (no_error=<optimized out>, shProg=<optimized out>, ctx=<optimized out>) at ../src/mesa/main/shaderapi.c:1284
1284	link_program_error(struct gl_context *ctx, struct gl_shader_program *shProg)
(gdb) c
Continuing.

Thread 1 "a.out" hit Breakpoint 8, st_link_shader (ctx=0x555555a5d1f0, prog=0x555555aa8a60) at ../src/mesa/state_tracker/st_glsl_to_tgsi.cpp:7304
7304	   struct pipe_screen *pscreen = ctx->st->pipe->screen;
(gdb) c
Continuing.
: 
: GLSL IR for linked vertex program 3:
(
(declare (location=0 shader_out ) vec4 gl_Position)
(declare (location=16 shader_in ) vec3 pos)
( function main
  (signature void
    (parameters
    )
    (
      (declare (temporary ) vec4 vec_ctor)
      (assign  (w) (var_ref vec_ctor)  (constant float (1.000000)) ) 
      (assign  (xyz) (var_ref vec_ctor)  (var_ref pos) ) 
      (assign  (xyzw) (var_ref gl_Position)  (var_ref vec_ctor) ) 
    ))

)

)
: 


Thread 1 "a.out" hit Breakpoint 5, si_create_shader_selector (ctx=0x555555589a50, state=0x555555aa9120) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:2216
2216		struct si_screen *sscreen = (struct si_screen *)ctx->screen;
(gdb) c
Continuing.
[Switching to Thread 0x7ffff1515700 (LWP 30896)]

Thread 3 "a.out:sh0" hit Breakpoint 4, si_init_shader_selector_async (job=job@entry=0x555555a8e2b0, thread_index=thread_index@entry=0) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:2042
2042		struct si_screen *sscreen = sel->screen;
(gdb) c
Continuing.

Thread 3 "a.out:sh0" hit Breakpoint 9, si_compile_tgsi_shader (sscreen=sscreen@entry=0x55555561e5b0, compiler=compiler@entry=0x55555561ecc8, shader=shader@entry=0x7fffe4000b20, 
    debug=debug@entry=0x555555a8e2d0) at ../src/gallium/drivers/radeonsi/si_shader.c:6751
6751		struct si_shader_selector *sel = shader->selector;
(gdb) list
6746	int si_compile_tgsi_shader(struct si_screen *sscreen,
6747				   struct ac_llvm_compiler *compiler,
6748				   struct si_shader *shader,
6749				   struct pipe_debug_callback *debug)
6750	{
6751		struct si_shader_selector *sel = shader->selector;
6752		struct si_shader_context ctx;
6753		int r = -1;
6754	
6755		/* Dump TGSI code before doing TGSI->LLVM conversion in case the
(gdb) n
6757		if (si_can_dump_shader(sscreen, sel->info.processor) &&
(gdb) n
6759			if (sel->tokens)
(gdb) n
6760				tgsi_dump(sel->tokens, 0);
(gdb) n
VERT
PROPERTY NEXT_SHADER FRAG
DCL IN[0]
DCL OUT[0], POSITION
DCL TEMP[0], LOCAL
IMM[0] FLT32 {    1.0000,     0.0000,     0.0000,     0.0000}
  0: MOV TEMP[0].w, IMM[0].xxxx
  1: MOV TEMP[0].xyz, IN[0].xyzx
  2: MOV OUT[0], TEMP[0]
  3: END
6763			si_dump_streamout(&sel->so);
(gdb) n
6766		si_init_shader_ctx(&ctx, sscreen, compiler);
(gdb) n
6767		si_llvm_context_set_tgsi(&ctx, shader);
(gdb) b
Breakpoint 10 at 0x7ffff5ae558a: file ../src/gallium/drivers/radeonsi/si_shader.c, line 6767.
(gdb) delete b 10
Ambiguous delete command "b 10": bookmark, breakpoints.
(gdb) delete breakpoints 10
(gdb) n
6769		memset(shader->info.vs_output_param_offset, AC_EXP_PARAM_UNDEFINED,
(gdb) n
6772		shader->info.uses_instanceid = sel->info.uses_instanceid;
(gdb) n
6774		if (!si_compile_tgsi_main(&ctx)) {
(gdb) n
6779		if (shader->is_monolithic && ctx.type == PIPE_SHADER_VERTEX) {
(gdb) 
6961		si_llvm_optimize_module(&ctx);
(gdb) list
6956	
6957			si_build_wrapper_function(&ctx, parts, need_prolog ? 3 : 2,
6958						  need_prolog ? 1 : 0, 0);
6959		}
6960	
6961		si_llvm_optimize_module(&ctx);
6962	
6963		/* Post-optimization transformations and analysis. */
6964		si_optimize_vs_outputs(&ctx);
6965	
(gdb) p shader->is_monolithic 
$3 = false
(gdb) list
6966		if ((debug && debug->debug_message) ||
6967		    si_can_dump_shader(sscreen, ctx.type)) {
6968			ctx.shader->config.private_mem_vgprs =
6969				ac_count_scratch_private_memory(ctx.main_fn);
6970		}
6971	
6972		/* Make sure the input is a pointer and not integer followed by inttoptr. */
6973		assert(LLVMGetTypeKind(LLVMTypeOf(LLVMGetParam(ctx.main_fn, 0))) ==
6974		       LLVMPointerTypeKind);
6975	
(gdb) n
6964		si_optimize_vs_outputs(&ctx);
(gdb) n
6966		if ((debug && debug->debug_message) ||
(gdb) n
6968			ctx.shader->config.private_mem_vgprs =
(gdb) n
6973		assert(LLVMGetTypeKind(LLVMTypeOf(LLVMGetParam(ctx.main_fn, 0))) ==
(gdb) n
6980				    si_should_optimize_less(compiler, shader->selector));
(gdb) n

Thread 3 "a.out:sh0" hit Breakpoint 3, si_get_shader_name (shader=0x7fffe4000b20, processor=0) at ../src/gallium/drivers/radeonsi/si_shader.c:5500
5500		switch (processor) {
(gdb) n

Thread 3 "a.out:sh0" hit Breakpoint 3, si_get_shader_name (processor=0, shader=0x7fffe4000b20) at ../src/gallium/drivers/radeonsi/si_shader.c:5502
5502			if (shader->key.as_es)
(gdb) n
5504			else if (shader->key.as_ls)
(gdb) n
radeonsi: Compiling shader 1
Vertex Shader as VS LLVM IR:

; ModuleID = 'mesa-shader'
source_filename = "mesa-shader"
target datalayout = "e-p:64:64-p1:64:64-p2:32:32-p3:32:32-p4:64:64-p5:32:32-p6:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-S32-A5"
target triple = "amdgcn--"

define amdgpu_vs void @main([0 x <4 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <8 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <4 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <8 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), i32 inreg, i32 inreg, i32 inreg, i32 inreg, [0 x <4 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), i32, i32, i32, i32, i32) #0 {
main_body:
  %14 = getelementptr inbounds [0 x <4 x i32>], [0 x <4 x i32>] addrspace(6)* %8, i32 0, i32 0, !amdgpu.uniform !0
  %15 = load <4 x i32>, <4 x i32> addrspace(6)* %14, align 16, !invariant.load !0
  %16 = call nsz <4 x float> @llvm.amdgcn.buffer.load.format.v4f32(<4 x i32> %15, i32 %13, i32 0, i1 false, i1 false) #3
  %17 = extractelement <4 x float> %16, i32 0
  %18 = extractelement <4 x float> %16, i32 1
  %19 = extractelement <4 x float> %16, i32 2
  call void @llvm.amdgcn.exp.f32(i32 12, i32 15, float %17, float %18, float %19, float 1.000000e+00, i1 true, i1 false) #2
  ret void
}

; Function Attrs: nounwind readonly
declare <4 x float> @llvm.amdgcn.buffer.load.format.v4f32(<4 x i32>, i32, i32, i1, i1) #1

; Function Attrs: nounwind
declare void @llvm.amdgcn.exp.f32(i32, i32, float, float, float, float, i1, i1) #2

attributes #0 = { "no-signed-zeros-fp-math"="true" }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }
attributes #3 = { nounwind readnone }

!0 = !{}

si_compile_tgsi_shader (sscreen=sscreen@entry=0x55555561e5b0, compiler=compiler@entry=0x55555561ecc8, shader=shader@entry=0x7fffe4000b20, debug=debug@entry=0x555555a8e2d0)
    at ../src/gallium/drivers/radeonsi/si_shader.c:6981
6981		si_llvm_dispose(&ctx);
(gdb) n
6982		if (r) {
(gdb) n
6990		if (sel->type == PIPE_SHADER_COMPUTE) {
(gdb) n
7019		if (shader->config.scratch_bytes_per_wave && !is_merged_shader(&ctx))
(gdb) n
7023		if (ctx.type == PIPE_SHADER_FRAGMENT) {
(gdb) n
7066		si_calculate_max_simd_waves(shader);
(gdb) n
7067		si_shader_dump_stats_for_shader_db(shader, debug);
(gdb) n
si_init_shader_selector_async (job=job@entry=0x555555a8e2b0, thread_index=thread_index@entry=0) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:2097
2097					mtx_lock(&sscreen->shader_cache_mutex);
(gdb) n
2098					if (!si_shader_cache_insert_shader(sscreen, ir_binary, shader, true))
(gdb) n
2100					mtx_unlock(&sscreen->shader_cache_mutex);
(gdb) c
Continuing.
: 
: GLSL IR for linked fragment program 3:
(
(declare (location=4 shader_out ) vec4 FragColor)
( function main
  (signature void
    (parameters
    )
    (
      (assign  (xyzw) (var_ref FragColor)  (constant vec4 (1.000000 0.000000 0.000000 1.000000)) ) 
    ))

)

)
: 

[Switching to Thread 0x7ffff673ed40 (LWP 30894)]

Thread 1 "a.out" hit Breakpoint 5, si_create_shader_selector (ctx=0x555555589a50, state=0x7fffffffdd10) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:2216
2216		struct si_screen *sscreen = (struct si_screen *)ctx->screen;
(gdb) n
2218		struct si_shader_selector *sel = CALLOC_STRUCT(si_shader_selector);
(gdb) n
2221		if (!sel)
(gdb) n
57	   p_atomic_set(&dst->count, count);
(gdb) n
2225		sel->screen = sscreen;
(gdb) n
2226		sel->compiler_ctx_state.debug = sctx->debug;
(gdb) n
2227		sel->compiler_ctx_state.is_debug_context = sctx->is_debug;
(gdb) n
2229		sel->so = state->stream_output;
(gdb) n
2231		if (state->type == PIPE_SHADER_IR_TGSI) {
(gdb) n
2232			sel->tokens = tgsi_dup_tokens(state->tokens);
(gdb) n
2233			if (!sel->tokens) {
(gdb) n
2238			tgsi_scan_shader(state->tokens, &sel->info);
(gdb) n
2239			tgsi_scan_tess_ctrl(state->tokens, &sel->info, &sel->tcs_info);
(gdb) n
2251		sel->type = sel->info.processor;
(gdb) n
2252		p_atomic_inc(&sscreen->num_shaders_created);
(gdb) n
2253		si_get_active_slot_masks(&sel->info,
(gdb) n
2258		for (i = 0; i < sel->so.num_outputs; i++) {
(gdb) n
2265		sel->vs_needs_prolog = sel->type == PIPE_SHADER_VERTEX &&
(gdb) n
2269		sel->force_correct_derivs_after_kill =
(gdb) c
Continuing.
[Switching to Thread 0x7ffff0d14700 (LWP 30897)]

Thread 4 "a.out:sh1" hit Breakpoint 4, si_init_shader_selector_async (job=job@entry=0x555555aa25d0, thread_index=thread_index@entry=1) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:2042
2042		struct si_screen *sscreen = sel->screen;
(gdb) c
Continuing.

Thread 4 "a.out:sh1" hit Breakpoint 9, si_compile_tgsi_shader (sscreen=sscreen@entry=0x55555561e5b0, compiler=compiler@entry=0x55555561ecf8, shader=shader@entry=0x7fffe0000b20, 
    debug=debug@entry=0x555555aa25f0) at ../src/gallium/drivers/radeonsi/si_shader.c:6751
6751		struct si_shader_selector *sel = shader->selector;
(gdb) n
6757		if (si_can_dump_shader(sscreen, sel->info.processor) &&
(gdb) n
6759			if (sel->tokens)
(gdb) n
6760				tgsi_dump(sel->tokens, 0);
(gdb) n
FRAG
DCL OUT[0], COLOR
IMM[0] FLT32 {    1.0000,     0.0000,     0.0000,     0.0000}
  0: MOV OUT[0], IMM[0].xyyx
  1: END
6763			si_dump_streamout(&sel->so);
(gdb) 
6766		si_init_shader_ctx(&ctx, sscreen, compiler);
(gdb) 
6767		si_llvm_context_set_tgsi(&ctx, shader);
(gdb) 
6769		memset(shader->info.vs_output_param_offset, AC_EXP_PARAM_UNDEFINED,
(gdb) 
6772		shader->info.uses_instanceid = sel->info.uses_instanceid;
(gdb) 
6774		if (!si_compile_tgsi_main(&ctx)) {
(gdb) 
6779		if (shader->is_monolithic && ctx.type == PIPE_SHADER_VERTEX) {
(gdb) 
6961		si_llvm_optimize_module(&ctx);
(gdb) 
6964		si_optimize_vs_outputs(&ctx);
(gdb) 
6966		if ((debug && debug->debug_message) ||
(gdb) 
6968			ctx.shader->config.private_mem_vgprs =
(gdb) 
6973		assert(LLVMGetTypeKind(LLVMTypeOf(LLVMGetParam(ctx.main_fn, 0))) ==
(gdb) 
6980				    si_should_optimize_less(compiler, shader->selector));
(gdb) 

Thread 4 "a.out:sh1" hit Breakpoint 3, si_get_shader_name (shader=0x7fffe0000b20, processor=1) at ../src/gallium/drivers/radeonsi/si_shader.c:5500
5500		switch (processor) {
(gdb) 
radeonsi: Compiling shader 2
Pixel Shader LLVM IR:

; ModuleID = 'mesa-shader'
source_filename = "mesa-shader"
target datalayout = "e-p:64:64-p1:64:64-p2:32:32-p3:32:32-p4:64:64-p5:32:32-p6:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-S32-A5"
target triple = "amdgcn--"

define amdgpu_ps <{ i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float }> @main([0 x <4 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <8 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <4 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <8 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), float inreg, i32 inreg, <2 x i32>, <2 x i32>, <2 x i32>, <3 x i32>, <2 x i32>, <2 x i32>, <2 x i32>, float, float, float, float, float, i32, i32, float, i32) #0 {
main_body:
  %22 = bitcast float %4 to i32
  %23 = insertvalue <{ i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float }> undef, i32 %22, 4
  %24 = insertvalue <{ i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float }> %23, float 1.000000e+00, 5
  %25 = insertvalue <{ i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float }> %24, float 0.000000e+00, 6
  %26 = insertvalue <{ i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float }> %25, float 0.000000e+00, 7
  %27 = insertvalue <{ i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float }> %26, float 1.000000e+00, 8
  %28 = insertvalue <{ i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float }> %27, float %20, 19
  ret <{ i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float }> %28
}

attributes #0 = { "InitialPSInputAddr"="0xb077" "no-signed-zeros-fp-math"="true" }

si_compile_tgsi_shader (sscreen=sscreen@entry=0x55555561e5b0, compiler=compiler@entry=0x55555561ecf8, shader=shader@entry=0x7fffe0000b20, debug=debug@entry=0x555555aa25f0)
    at ../src/gallium/drivers/radeonsi/si_shader.c:6981
6981		si_llvm_dispose(&ctx);
(gdb) 
6982		if (r) {
(gdb) 
6990		if (sel->type == PIPE_SHADER_COMPUTE) {
(gdb) 
7019		if (shader->config.scratch_bytes_per_wave && !is_merged_shader(&ctx))
(gdb) 
7023		if (ctx.type == PIPE_SHADER_FRAGMENT) {
(gdb) 
7026			shader->info.ancillary_vgpr_index = -1;
(gdb) 
7028			if (G_0286CC_PERSP_SAMPLE_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7030			if (G_0286CC_PERSP_CENTER_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7031				shader->info.num_input_vgprs += 2;
(gdb) 
7032			if (G_0286CC_PERSP_CENTROID_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7033				shader->info.num_input_vgprs += 2;
(gdb) 
7034			if (G_0286CC_PERSP_PULL_MODEL_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7036			if (G_0286CC_LINEAR_SAMPLE_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7037				shader->info.num_input_vgprs += 2;
(gdb) 
7038			if (G_0286CC_LINEAR_CENTER_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7039				shader->info.num_input_vgprs += 2;
(gdb) 
7040			if (G_0286CC_LINEAR_CENTROID_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7041				shader->info.num_input_vgprs += 2;
(gdb) 
7042			if (G_0286CC_LINE_STIPPLE_TEX_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7044			if (G_0286CC_POS_X_FLOAT_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7046			if (G_0286CC_POS_Y_FLOAT_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7048			if (G_0286CC_POS_Z_FLOAT_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7050			if (G_0286CC_POS_W_FLOAT_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7052			if (G_0286CC_FRONT_FACE_ENA(shader->config.spi_ps_input_addr)) {
(gdb) 
7053				shader->info.face_vgpr_index = shader->info.num_input_vgprs;
(gdb) 
7054				shader->info.num_input_vgprs += 1;
(gdb) 
7056			if (G_0286CC_ANCILLARY_ENA(shader->config.spi_ps_input_addr)) {
(gdb) 
7057				shader->info.ancillary_vgpr_index = shader->info.num_input_vgprs;
(gdb) 
7058				shader->info.num_input_vgprs += 1;
(gdb) 
7060			if (G_0286CC_SAMPLE_COVERAGE_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7061				shader->info.num_input_vgprs += 1;
(gdb) 
7062			if (G_0286CC_POS_FIXED_PT_ENA(shader->config.spi_ps_input_addr))
(gdb) 
7063				shader->info.num_input_vgprs += 1;
(gdb) 
7066		si_calculate_max_simd_waves(shader);
(gdb) 
7067		si_shader_dump_stats_for_shader_db(shader, debug);
(gdb) 
si_init_shader_selector_async (job=job@entry=0x555555aa25d0, thread_index=thread_index@entry=1) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:2097
2097					mtx_lock(&sscreen->shader_cache_mutex);
(gdb) 
2098					if (!si_shader_cache_insert_shader(sscreen, ir_binary, shader, true))
(gdb) 
2100					mtx_unlock(&sscreen->shader_cache_mutex);
(gdb) 
2104			*si_get_main_shader_part(sel, &shader->key) = shader;
(gdb) 
2113			if ((sel->type == PIPE_SHADER_VERTEX ||
(gdb) 
2150		if (sel->type == PIPE_SHADER_GEOMETRY) {
(gdb) 
util_queue_thread_func (input=input@entry=0x55555561b3f0) at ../src/util/u_queue.c:287
287	         util_queue_fence_signal(job.fence);
(gdb) 
[Switching to Thread 0x7ffff673ed40 (LWP 30894)]



 void
  317 st_init_draw_functions(struct dd_function_table *functions)
  318 {
  319    functions->Draw = st_draw_vbo;
  320    functions->DrawIndirect = st_indirect_draw_vbo;
  321 }
  322 




 54  * Swaps the buffers for the current window (if any)
 55  */
 56 void FGAPIENTRY glutSwapBuffers( void )
 57 {
 58     FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutSwapBuffers" );
 59     FREEGLUT_EXIT_IF_NO_WINDOW ( "glutSwapBuffers" );
 60 
 61     /*
 62      * "glXSwapBuffers" already performs an implicit call to "glFlush". What
 63      * about "SwapBuffers"?
 64      */
 65     glFlush( );
 66     if( ! fgStructure.CurrentWindow->Window.DoubleBuffered )
 67         return;
 68 
 69     fgPlatformGlutSwapBuffers( &fgDisplay.pDisplay, fgStructure.CurrentWindow );
 70 
 71     /* GLUT_FPS env var support */
 72     if( fgState.FPSInterval )
 73     {
 74         GLint t = glutGet( GLUT_ELAPSED_TIME );
 75         fgState.SwapCount++;
 76         if( fgState.SwapTime == 0 )
 77             fgState.SwapTime = t;
 78         else if( t - fgState.SwapTime > fgState.FPSInterval )
 79         {
 80             float time = 0.001f * ( t - fgState.SwapTime );
 81             float fps = ( float )fgState.SwapCount / time;
 82             fprintf( stderr,
 83                      "freeglut: %d frames in %.2f seconds = %.2f FPS\n",
 84                      fgState.SwapCount, time, fps );
 85             fgState.SwapTime = t;
 86             fgState.SwapCount = 0;
 87         }
 88     }
 89 }



static const struct debug_named_value debug_options[] = {
	/* Shader logging options: */
	{ "vs", DBG(VS), "Print vertex shaders" },
	{ "ps", DBG(PS), "Print pixel shaders" },
	{ "gs", DBG(GS), "Print geometry shaders" },
	{ "tcs", DBG(TCS), "Print tessellation control shaders" },
	{ "tes", DBG(TES), "Print tessellation evaluation shaders" },
	{ "cs", DBG(CS), "Print compute shaders" },
	{ "noir", DBG(NO_IR), "Don't print the LLVM IR"},
	{ "notgsi", DBG(NO_TGSI), "Don't print the TGSI"},
	{ "noasm", DBG(NO_ASM), "Don't print disassembled shaders"},
	{ "preoptir", DBG(PREOPT_IR), "Print the LLVM IR before initial optimizations" },

	/* Shader compiler options the shader cache should be aware of: */
	{ "unsafemath", DBG(UNSAFE_MATH), "Enable unsafe math shader optimizations" },
	{ "sisched", DBG(SI_SCHED), "Enable LLVM SI Machine Instruction Scheduler." },
	{ "gisel", DBG(GISEL), "Enable LLVM global instruction selector." },

	/* Shader compiler options (with no effect on the shader cache): */
	{ "checkir", DBG(CHECK_IR), "Enable additional sanity checks on shader IR" },
	{ "nir", DBG(NIR), "Enable experimental NIR shaders" },
	{ "mono", DBG(MONOLITHIC_SHADERS), "Use old-style monolithic shaders compiled on demand" },
	{ "nooptvariant", DBG(NO_OPT_VARIANT), "Disable compiling optimized shader variants." },

	/* Information logging options: */
	{ "info", DBG(INFO), "Print driver information" },
	{ "tex", DBG(TEX), "Print texture info" },
	{ "compute", DBG(COMPUTE), "Print compute info" },
	{ "vm", DBG(VM), "Print virtual addresses when creating resources" },

	/* Driver options: */
	{ "forcedma", DBG(FORCE_DMA), "Use asynchronous DMA for all operations when possible." },
	{ "nodma", DBG(NO_ASYNC_DMA), "Disable asynchronous DMA" },
	{ "nowc", DBG(NO_WC), "Disable GTT write combining" },
	{ "check_vm", DBG(CHECK_VM), "Check VM faults and dump debug info." },
	{ "reserve_vmid", DBG(RESERVE_VMID), "Force VMID reservation per context." },
	{ "zerovram", DBG(ZERO_VRAM), "Clear VRAM allocations." },

	/* 3D engine options: */
	{ "switch_on_eop", DBG(SWITCH_ON_EOP), "Program WD/IA to switch on end-of-packet." },
	{ "nooutoforder", DBG(NO_OUT_OF_ORDER), "Disable out-of-order rasterization" },
	{ "nodpbb", DBG(NO_DPBB), "Disable DPBB." },
	{ "nodfsm", DBG(NO_DFSM), "Disable DFSM." },
	{ "dpbb", DBG(DPBB), "Enable DPBB." },
	{ "dfsm", DBG(DFSM), "Enable DFSM." },
	{ "nohyperz", DBG(NO_HYPERZ), "Disable Hyper-Z" },
	{ "norbplus", DBG(NO_RB_PLUS), "Disable RB+." },
	{ "no2d", DBG(NO_2D_TILING), "Disable 2D tiling" },
	{ "notiling", DBG(NO_TILING), "Disable tiling" },
	{ "nodcc", DBG(NO_DCC), "Disable DCC." },
	{ "nodccclear", DBG(NO_DCC_CLEAR), "Disable DCC fast clear." },
	{ "nodccfb", DBG(NO_DCC_FB), "Disable separate DCC on the main framebuffer" },
	{ "nodccmsaa", DBG(NO_DCC_MSAA), "Disable DCC for MSAA" },
	{ "nofmask", DBG(NO_FMASK), "Disable MSAA compression" },

	/* Tests: */
	{ "testdma", DBG(TEST_DMA), "Invoke SDMA tests and exit." },
	{ "testvmfaultcp", DBG(TEST_VMFAULT_CP), "Invoke a CP VM fault test and exit." },
	{ "testvmfaultsdma", DBG(TEST_VMFAULT_SDMA), "Invoke a SDMA VM fault test and exit." },
	{ "testvmfaultshader", DBG(TEST_VMFAULT_SHADER), "Invoke a shader VM fault test and exit." },
	{ "testdmaperf", DBG(TEST_DMA_PERF), "Test DMA performance" },
	{ "testgds", DBG(TEST_GDS), "Test GDS." },

	DEBUG_NAMED_VALUE_END /* must be last */
};


R600_DEBUG



	if (debug_get_bool_option("RADEON_DUMP_SHADERS", false))
		sscreen->debug_flags |= DBG_ALL_SHADERS;


*******************


(gdb) bt
#0  si_shader_create (sscreen=sscreen@entry=0x55555561e5b0, compiler=0x555555589f40, shader=shader@entry=0x555555a961c0, debug=0x555555a961c8) at ../src/gallium/drivers/radeonsi/si_shader.c:8039
#1  0x00007ffff5a9f38a in si_build_shader_variant (shader=0x555555a961c0, thread_index=<optimized out>, low_priority=<optimized out>) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1715
#2  0x00007ffff5aa2d31 in si_shader_select_with_key (thread_index=-1, state=0x55555558a670, state=0x55555558a670, key=0x7fffffffd9e0, compiler_state=<optimized out>, sscreen=0x55555561e5b0)
    at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1977
#3  si_shader_select (ctx=ctx@entry=0x555555589a50, state=state@entry=0x55555558a670, compiler_state=compiler_state@entry=0x7fffffffdad0) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1995
#4  0x00007ffff5aa47f2 in si_update_shaders (sctx=sctx@entry=0x555555589a50) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:3413
#5  0x00007ffff5a9c019 in si_draw_vbo (ctx=ctx@entry=0x555555589a50, info=info@entry=0x7fffffffdbc0) at ../src/gallium/drivers/radeonsi/si_state_draw.c:1362
#6  0x00007ffff5a9c95a in si_draw_rectangle (blitter=<optimized out>, vertex_elements_cso=<optimized out>, get_vs=<optimized out>, x1=<optimized out>, y1=<optimized out>, x2=<optimized out>, y2=500, 
    depth=<optimized out>, num_instances=1, type=UTIL_BLITTER_ATTRIB_COLOR, attrib=0x7fffffffdc60) at ../src/gallium/drivers/radeonsi/si_state_draw.c:1576
#7  0x00007ffff61292fc in util_blitter_clear_custom (blitter=0x5555555c9540, width=500, height=500, num_layers=1, clear_buffers=clear_buffers@entry=4, color=color@entry=0x555555a60b3c, 
    depth=depth@entry=1, stencil=0, custom_dsa=0x0, custom_blend=0x0) at ../src/gallium/auxiliary/util/u_blitter.c:1438
#8  0x00007ffff6129430 in util_blitter_clear (blitter=<optimized out>, width=<optimized out>, height=<optimized out>, num_layers=<optimized out>, clear_buffers=clear_buffers@entry=4, 
    color=color@entry=0x555555a60b3c, depth=depth@entry=1, stencil=0) at ../src/gallium/auxiliary/util/u_blitter.c:1455
#9  0x00007ffff5ac0edf in si_clear (ctx=0x555555589a50, buffers=4, color=<optimized out>, depth=<optimized out>, stencil=0) at ../src/gallium/drivers/radeonsi/si_clear.c:633
#10 0x00007ffff5ed660f in st_Clear (ctx=0x555555a5ea60, mask=2) at ../src/mesa/state_tracker/st_cb_clear.c:452
#11 0x00005555555567c9 in display() ()
#12 0x00007ffff7d18844 in ?? () from /lib/x86_64-linux-gnu/libglut.so.3
#13 0x00007ffff7d1c2e9 in fgEnumWindows () from /lib/x86_64-linux-gnu/libglut.so.3
#14 0x00007ffff7d18e6d in glutMainLoopEvent () from /lib/x86_64-linux-gnu/libglut.so.3
#15 0x00007ffff7d196a5 in glutMainLoop () from /lib/x86_64-linux-gnu/libglut.so.3
#16 0x0000555555556851 in main ()





(gdb) c
Continuing.
SHADER KEY
  part.vs.prolog.instance_divisor_is_one = 0
  part.vs.prolog.instance_divisor_is_fetched = 0
  part.vs.prolog.ls_vgpr_fix = 0
  mono.vs.fix_fetch = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  as_es = 0
  as_ls = 0
  mono.u.vs_export_prim_id = 0
  opt.kill_outputs = 0x0
  opt.clip_disable = 0

Vertex Shader as VS:
Shader main disassembly:
main:
BB0_0:
	v_mov_b32_e32 v1, s3                                                                ; 7E020203
	v_mov_b32_e32 v2, s2                                                                ; 7E040202
	v_cmp_gt_u32_e32 vcc, 2, v0                                                         ; 7D980082
	v_cndmask_b32_e32 v3, v1, v2, vcc                                                   ; 00060501
	v_cmp_eq_u32_e32 vcc, 1, v0                                                         ; 7D940081
	v_cndmask_b32_e32 v0, v2, v1, vcc                                                   ; 00000302
	v_cvt_f32_i32_sdwa v1, sext(v3) dst_sel:DWORD dst_unused:UNUSED_PAD src0_sel:WORD_0 ; 7E020AF9 000C0603
	v_cvt_f32_i32_sdwa v0, sext(v0) dst_sel:DWORD dst_unused:UNUSED_PAD src0_sel:WORD_1 ; 7E000AF9 000D0600
	v_mov_b32_e32 v2, 1.0                                                               ; 7E0402F2
	v_mov_b32_e32 v3, s4                                                                ; 7E060204
	exp pos0 v1, v0, v3, v2 done                                                        ; C40008CF 02030001
	s_waitcnt expcnt(0)                                                                 ; BF8C0F0F
	v_mov_b32_e32 v0, s5                                                                ; 7E000205
	v_mov_b32_e32 v1, s6                                                                ; 7E020206
	v_mov_b32_e32 v2, s7                                                                ; 7E040207
	v_mov_b32_e32 v3, s8                                                                ; 7E060208
	exp param0 v0, v1, v2, v3                                                           ; C400020F 03020100
	s_endpgm                                                                            ; BF810000

*** SHADER STATS ***
SGPRS: 16
VGPRS: 8
Spilled SGPRs: 0
Spilled VGPRs: 0
Private memory VGPRs: 0
Code Size: 108 bytes
LDS: 0 blocks
Scratch: 0 bytes per wave
Max Waves: 8
********************

























Thread 1 "a.out" hit Breakpoint 6, 0x00007ffff7fa5240 in glClear () from /home/shiji/mesa-18.3.6/build/install/lib/x86_64-linux-gnu/libGL.so.1
(gdb) c
Continuing.
Mesa: shiji_glClear 0x4000

Thread 1 "a.out" hit Breakpoint 5, si_shader_create (sscreen=sscreen@entry=0x55555561e580, compiler=0x555555589f40, shader=shader@entry=0x555555a96000, debug=0x555555a96008)
    at ../src/gallium/drivers/radeonsi/si_shader.c:8039
8039		struct si_shader_selector *sel = shader->selector;
(gdb) bt5
Undefined command: "bt5".  Try "help".
(gdb) bt
#0  si_shader_create (sscreen=sscreen@entry=0x55555561e580, compiler=0x555555589f40, shader=shader@entry=0x555555a96000, debug=0x555555a96008) at ../src/gallium/drivers/radeonsi/si_shader.c:8039
#1  0x00007ffff5a9f38a in si_build_shader_variant (shader=0x555555a96000, thread_index=<optimized out>, low_priority=<optimized out>) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1715
#2  0x00007ffff5aa2d31 in si_shader_select_with_key (thread_index=-1, state=0x55555558a670, state=0x55555558a670, key=0x7fffffffd9e0, compiler_state=<optimized out>, sscreen=0x55555561e580)
    at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1977
#3  si_shader_select (ctx=ctx@entry=0x555555589a50, state=state@entry=0x55555558a670, compiler_state=compiler_state@entry=0x7fffffffdad0) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1995
#4  0x00007ffff5aa47f2 in si_update_shaders (sctx=sctx@entry=0x555555589a50) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:3413
#5  0x00007ffff5a9c019 in si_draw_vbo (ctx=ctx@entry=0x555555589a50, info=info@entry=0x7fffffffdbc0) at ../src/gallium/drivers/radeonsi/si_state_draw.c:1362
#6  0x00007ffff5a9c95a in si_draw_rectangle (blitter=<optimized out>, vertex_elements_cso=<optimized out>, get_vs=<optimized out>, x1=<optimized out>, y1=<optimized out>, x2=<optimized out>, y2=500, 
    depth=<optimized out>, num_instances=1, type=UTIL_BLITTER_ATTRIB_COLOR, attrib=0x7fffffffdc60) at ../src/gallium/drivers/radeonsi/si_state_draw.c:1576
#7  0x00007ffff61292fc in util_blitter_clear_custom (blitter=0x5555555c9540, width=500, height=500, num_layers=1, clear_buffers=clear_buffers@entry=4, color=color@entry=0x555555a607dc, 
    depth=depth@entry=1, stencil=0, custom_dsa=0x0, custom_blend=0x0) at ../src/gallium/auxiliary/util/u_blitter.c:1438
#8  0x00007ffff6129430 in util_blitter_clear (blitter=<optimized out>, width=<optimized out>, height=<optimized out>, num_layers=<optimized out>, clear_buffers=clear_buffers@entry=4, 
    color=color@entry=0x555555a607dc, depth=depth@entry=1, stencil=0) at ../src/gallium/auxiliary/util/u_blitter.c:1455
#9  0x00007ffff5ac0edf in si_clear (ctx=0x555555589a50, buffers=4, color=<optimized out>, depth=<optimized out>, stencil=0) at ../src/gallium/drivers/radeonsi/si_clear.c:633
#10 0x00007ffff5ed660f in st_Clear (ctx=0x555555a5e700, mask=2) at ../src/mesa/state_tracker/st_cb_clear.c:452
#11 0x00005555555567c9 in display() ()
#12 0x00007ffff7d18844 in ?? () from /lib/x86_64-linux-gnu/libglut.so.3
#13 0x00007ffff7d1c2e9 in fgEnumWindows () from /lib/x86_64-linux-gnu/libglut.so.3
#14 0x00007ffff7d18e6d in glutMainLoopEvent () from /lib/x86_64-linux-gnu/libglut.so.3
#15 0x00007ffff7d196a5 in glutMainLoop () from /lib/x86_64-linux-gnu/libglut.so.3
#16 0x0000555555556851 in main ()
(gdb) c
Continuing.
SHADER KEY
  part.vs.prolog.instance_divisor_is_one = 0
  part.vs.prolog.instance_divisor_is_fetched = 0
  part.vs.prolog.ls_vgpr_fix = 0
  mono.vs.fix_fetch = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  as_es = 0
  as_ls = 0
  mono.u.vs_export_prim_id = 0
  opt.kill_outputs = 0x0
  opt.clip_disable = 0

Vertex Shader as VS:
Shader main disassembly:
main:
BB0_0:
	v_mov_b32_e32 v1, s3                                                                ; 7E020203
	v_mov_b32_e32 v2, s2                                                                ; 7E040202
	v_cmp_gt_u32_e32 vcc, 2, v0                                                         ; 7D980082
	v_cndmask_b32_e32 v3, v1, v2, vcc                                                   ; 00060501
	v_cmp_eq_u32_e32 vcc, 1, v0                                                         ; 7D940081
	v_cndmask_b32_e32 v0, v2, v1, vcc                                                   ; 00000302
	v_cvt_f32_i32_sdwa v1, sext(v3) dst_sel:DWORD dst_unused:UNUSED_PAD src0_sel:WORD_0 ; 7E020AF9 000C0603
	v_cvt_f32_i32_sdwa v0, sext(v0) dst_sel:DWORD dst_unused:UNUSED_PAD src0_sel:WORD_1 ; 7E000AF9 000D0600
	v_mov_b32_e32 v2, 1.0                                                               ; 7E0402F2
	v_mov_b32_e32 v3, s4                                                                ; 7E060204
	exp pos0 v1, v0, v3, v2 done                                                        ; C40008CF 02030001
	s_waitcnt expcnt(0)                                                                 ; BF8C0F0F
	v_mov_b32_e32 v0, s5                                                                ; 7E000205
	v_mov_b32_e32 v1, s6                                                                ; 7E020206
	v_mov_b32_e32 v2, s7                                                                ; 7E040207
	v_mov_b32_e32 v3, s8                                                                ; 7E060208
	exp param0 v0, v1, v2, v3                                                           ; C400020F 03020100
	s_endpgm                                                                            ; BF810000

*** SHADER STATS ***
SGPRS: 16
VGPRS: 8
Spilled SGPRs: 0
Spilled VGPRs: 0
Private memory VGPRs: 0
Code Size: 108 bytes
LDS: 0 blocks
Scratch: 0 bytes per wave
Max Waves: 8
********************



Thread 1 "a.out" hit Breakpoint 5, si_shader_create (sscreen=sscreen@entry=0x55555561e580, compiler=0x555555589f40, shader=shader@entry=0x555555a96830, debug=0x555555a96838)
    at ../src/gallium/drivers/radeonsi/si_shader.c:8039
8039		struct si_shader_selector *sel = shader->selector;
(gdb) bt
#0  si_shader_create (sscreen=sscreen@entry=0x55555561e580, compiler=0x555555589f40, shader=shader@entry=0x555555a96830, debug=0x555555a96838) at ../src/gallium/drivers/radeonsi/si_shader.c:8039
#1  0x00007ffff5a9f38a in si_build_shader_variant (shader=0x555555a96830, thread_index=<optimized out>, low_priority=<optimized out>) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1715
#2  0x00007ffff5aa2d31 in si_shader_select_with_key (thread_index=-1, state=0x55555558a650, state=0x55555558a650, key=0x7fffffffd9e0, compiler_state=<optimized out>, sscreen=0x55555561e580)
    at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1977
#3  si_shader_select (ctx=ctx@entry=0x555555589a50, state=state@entry=0x55555558a650, compiler_state=compiler_state@entry=0x7fffffffdad0) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1995
#4  0x00007ffff5aa3ba1 in si_update_shaders (sctx=sctx@entry=0x555555589a50) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:3445
#5  0x00007ffff5a9c019 in si_draw_vbo (ctx=ctx@entry=0x555555589a50, info=info@entry=0x7fffffffdbc0) at ../src/gallium/drivers/radeonsi/si_state_draw.c:1362
#6  0x00007ffff5a9c95a in si_draw_rectangle (blitter=<optimized out>, vertex_elements_cso=<optimized out>, get_vs=<optimized out>, x1=<optimized out>, y1=<optimized out>, x2=<optimized out>, y2=500, 
    depth=<optimized out>, num_instances=1, type=UTIL_BLITTER_ATTRIB_COLOR, attrib=0x7fffffffdc60) at ../src/gallium/drivers/radeonsi/si_state_draw.c:1576
#7  0x00007ffff61292fc in util_blitter_clear_custom (blitter=0x5555555c9540, width=500, height=500, num_layers=1, clear_buffers=clear_buffers@entry=4, color=color@entry=0x555555a607dc, 
    depth=depth@entry=1, stencil=0, custom_dsa=0x0, custom_blend=0x0) at ../src/gallium/auxiliary/util/u_blitter.c:1438
#8  0x00007ffff6129430 in util_blitter_clear (blitter=<optimized out>, width=<optimized out>, height=<optimized out>, num_layers=<optimized out>, clear_buffers=clear_buffers@entry=4, 
    color=color@entry=0x555555a607dc, depth=depth@entry=1, stencil=0) at ../src/gallium/auxiliary/util/u_blitter.c:1455
#9  0x00007ffff5ac0edf in si_clear (ctx=0x555555589a50, buffers=4, color=<optimized out>, depth=<optimized out>, stencil=0) at ../src/gallium/drivers/radeonsi/si_clear.c:633
#10 0x00007ffff5ed660f in st_Clear (ctx=0x555555a5e700, mask=2) at ../src/mesa/state_tracker/st_cb_clear.c:452
#11 0x00005555555567c9 in display() ()
#12 0x00007ffff7d18844 in ?? () from /lib/x86_64-linux-gnu/libglut.so.3
#13 0x00007ffff7d1c2e9 in fgEnumWindows () from /lib/x86_64-linux-gnu/libglut.so.3
#14 0x00007ffff7d18e6d in glutMainLoopEvent () from /lib/x86_64-linux-gnu/libglut.so.3
#15 0x00007ffff7d196a5 in glutMainLoop () from /lib/x86_64-linux-gnu/libglut.so.3
#16 0x0000555555556851 in main ()
(gdb) c
Continuing.
radeonsi: Compiling shader 1
Fragment Shader Epilog LLVM IR:

; ModuleID = 'mesa-shader'
source_filename = "mesa-shader"
target datalayout = "e-p:64:64-p1:64:64-p2:32:32-p3:32:32-p4:64:64-p5:32:32-p6:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-S32-A5"
target triple = "amdgcn--"

define amdgpu_ps void @ps_epilog(i32 inreg, i32 inreg, i32 inreg, i32 inreg, float inreg, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float) #0 {
main_body:
  %20 = call nsz <2 x half> @llvm.amdgcn.cvt.pkrtz(float %5, float %6) #3
  %21 = call nsz <2 x half> @llvm.amdgcn.cvt.pkrtz(float %7, float %8) #3
  %22 = bitcast <2 x half> %20 to <2 x i16>
  %23 = bitcast <2 x half> %21 to <2 x i16>
  call void @llvm.amdgcn.exp.compr.v2i16(i32 0, i32 15, <2 x i16> %22, <2 x i16> %23, i1 true, i1 true) #2
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare <2 x half> @llvm.amdgcn.cvt.pkrtz(float, float) #1

; Function Attrs: nounwind
declare void @llvm.amdgcn.exp.compr.v2i16(i32, i32, <2 x i16>, <2 x i16>, i1, i1) #2

attributes #0 = { "InitialPSInputAddr"="0xffffff" "no-signed-zeros-fp-math"="true" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind }
attributes #3 = { nounwind readnone }

SHADER KEY
  part.ps.prolog.color_two_side = 0
  part.ps.prolog.flatshade_colors = 0
  part.ps.prolog.poly_stipple = 0
  part.ps.prolog.force_persp_sample_interp = 0
  part.ps.prolog.force_linear_sample_interp = 0
  part.ps.prolog.force_persp_center_interp = 0
  part.ps.prolog.force_linear_center_interp = 0
  part.ps.prolog.bc_optimize_for_persp = 0
  part.ps.prolog.bc_optimize_for_linear = 0
  part.ps.epilog.spi_shader_col_format = 0x4
  part.ps.epilog.color_is_int8 = 0x0
  part.ps.epilog.color_is_int10 = 0x0
  part.ps.epilog.last_cbuf = 0
  part.ps.epilog.alpha_func = 7
  part.ps.epilog.alpha_to_one = 0
  part.ps.epilog.poly_line_smoothing = 0
  part.ps.epilog.clamp_color = 0

Pixel Shader:
Shader main disassembly:
main:
BB0_0:
	s_mov_b32 m0, s5                     ; BEFC0005
	v_interp_mov_f32_e32 v0, p0, attr0.x ; D4020002
	v_interp_mov_f32_e32 v1, p0, attr0.y ; D4060102
	v_interp_mov_f32_e32 v2, p0, attr0.z ; D40A0202
	v_interp_mov_f32_e32 v3, p0, attr0.w ; D40E0302
Shader epilog disassembly:
ps_epilog:
BB0_0:
	v_cvt_pkrtz_f16_f32 v0, v0, v1        ; D2960000 00020300
	v_cvt_pkrtz_f16_f32 v1, v2, v3        ; D2960001 00020702
	exp mrt0 v0, v0, v1, v1 done compr vm ; C4001C0F 00000100
	s_endpgm                              ; BF810000

*** SHADER CONFIG ***
SPI_PS_INPUT_ADDR = 0xf077
SPI_PS_INPUT_ENA  = 0x0020
*** SHADER STATS ***
SGPRS: 8
VGPRS: 24
Spilled SGPRs: 0
Spilled VGPRs: 0
Private memory VGPRs: 0
Code Size: 68 bytes
LDS: 0 blocks
Scratch: 0 bytes per wave
Max Waves: 8
********************



Thread 1 "a.out" hit Breakpoint 7, 0x00007ffff7fa5fa0 in glDrawArraysEXT () from /home/shiji/mesa-18.3.6/build/install/lib/x86_64-linux-gnu/libGL.so.1
(gdb) bt
#0  0x00007ffff7fa5fa0 in glDrawArraysEXT () from /home/shiji/mesa-18.3.6/build/install/lib/x86_64-linux-gnu/libGL.so.1
#1  0x00005555555567dd in display() ()
#2  0x00007ffff7d18844 in ?? () from /lib/x86_64-linux-gnu/libglut.so.3
#3  0x00007ffff7d1c2e9 in fgEnumWindows () from /lib/x86_64-linux-gnu/libglut.so.3
#4  0x00007ffff7d18e6d in glutMainLoopEvent () from /lib/x86_64-linux-gnu/libglut.so.3
#5  0x00007ffff7d196a5 in glutMainLoop () from /lib/x86_64-linux-gnu/libglut.so.3
#6  0x0000555555556851 in main ()
(gdb) c
Continuing.

Thread 1 "a.out" hit Breakpoint 5, si_shader_create (sscreen=sscreen@entry=0x55555561e580, compiler=0x555555589f40, shader=shader@entry=0x555555ae0ce0, debug=0x555555ae0ce8)
    at ../src/gallium/drivers/radeonsi/si_shader.c:8039
8039		struct si_shader_selector *sel = shader->selector;
(gdb) bt
#0  si_shader_create (sscreen=sscreen@entry=0x55555561e580, compiler=0x555555589f40, shader=shader@entry=0x555555ae0ce0, debug=0x555555ae0ce8) at ../src/gallium/drivers/radeonsi/si_shader.c:8039
#1  0x00007ffff5a9f38a in si_build_shader_variant (shader=0x555555ae0ce0, thread_index=<optimized out>, low_priority=<optimized out>) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1715
#2  0x00007ffff5aa2d31 in si_shader_select_with_key (thread_index=-1, state=0x55555558a670, state=0x55555558a670, key=0x7fffffffd940, compiler_state=<optimized out>, sscreen=0x55555561e580)
    at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1977
#3  si_shader_select (ctx=ctx@entry=0x555555589a50, state=state@entry=0x55555558a670, compiler_state=compiler_state@entry=0x7fffffffda30) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1995
#4  0x00007ffff5aa47f2 in si_update_shaders (sctx=sctx@entry=0x555555589a50) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:3413
#5  0x00007ffff5a9c019 in si_draw_vbo (ctx=0x555555589a50, info=0x7fffffffdd10) at ../src/gallium/drivers/radeonsi/si_state_draw.c:1362
#6  0x00007ffff593d809 in u_vbuf_draw_vbo (mgr=0x555555a9f830, info=0x7fffffffdd10) at ../src/gallium/auxiliary/util/u_vbuf.c:1182
#7  0x00007ffff5e6c1e7 in st_draw_vbo (ctx=<optimized out>, prims=<optimized out>, nr_prims=<optimized out>, ib=0x0, index_bounds_valid=<optimized out>, min_index=<optimized out>, 
    max_index=<optimized out>, tfb_vertcount=0x0, stream=0, indirect=0x0) at ../src/mesa/state_tracker/st_draw.c:236
#8  0x00007ffff5de6a62 in _mesa_draw_arrays (drawID=0, baseInstance=0, numInstances=1, count=3, start=0, mode=4, ctx=0x555555a5e700) at ../src/mesa/main/draw.c:408
#9  _mesa_draw_arrays (ctx=0x555555a5e700, mode=4, start=0, count=3, numInstances=1, baseInstance=0, drawID=0) at ../src/mesa/main/draw.c:385
#10 0x00007ffff5de7775 in _mesa_exec_DrawArrays (mode=4, start=0, count=3) at ../src/mesa/main/draw.c:565
#11 0x00005555555567dd in display() ()
#12 0x00007ffff7d18844 in ?? () from /lib/x86_64-linux-gnu/libglut.so.3
#13 0x00007ffff7d1c2e9 in fgEnumWindows () from /lib/x86_64-linux-gnu/libglut.so.3
#14 0x00007ffff7d18e6d in glutMainLoopEvent () from /lib/x86_64-linux-gnu/libglut.so.3
#15 0x00007ffff7d196a5 in glutMainLoop () from /lib/x86_64-linux-gnu/libglut.so.3
#16 0x0000555555556851 in main ()
(gdb) c
Continuing.
radeonsi: Compiling shader 2
Vertex Shader Prolog LLVM IR:

; ModuleID = 'mesa-shader'
source_filename = "mesa-shader"
target datalayout = "e-p:64:64-p1:64:64-p2:32:32-p3:32:32-p4:64:64-p5:32:32-p6:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-S32-A5"
target triple = "amdgcn--"

define amdgpu_vs <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> @vs_prolog(i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32, i32, i32, i32) #0 {
main_body:
  %13 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> undef, i32 %0, 0
  %14 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %13, i32 %1, 1
  %15 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %14, i32 %2, 2
  %16 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %15, i32 %3, 3
  %17 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %16, i32 %4, 4
  %18 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %17, i32 %5, 5
  %19 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %18, i32 %6, 6
  %20 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %19, i32 %7, 7
  %21 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %20, i32 %8, 8
  %22 = bitcast i32 %9 to float
  %23 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %21, float %22, 9
  %24 = bitcast i32 %10 to float
  %25 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %23, float %24, 10
  %26 = bitcast i32 %11 to float
  %27 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %25, float %26, 11
  %28 = bitcast i32 %12 to float
  %29 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %27, float %28, 12
  %30 = add i32 %9, %5
  %31 = bitcast i32 %30 to float
  %32 = insertvalue <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %29, float %31, 13
  ret <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float }> %32
}

attributes #0 = { "no-signed-zeros-fp-math"="true" }

SHADER KEY
  part.vs.prolog.instance_divisor_is_one = 0
  part.vs.prolog.instance_divisor_is_fetched = 0
  part.vs.prolog.ls_vgpr_fix = 0
  mono.vs.fix_fetch = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  as_es = 0
  as_ls = 0
  mono.u.vs_export_prim_id = 0
  opt.kill_outputs = 0x0
  opt.clip_disable = 0

Vertex Shader as VS:
Shader prolog disassembly:
vs_prolog:
BB1_0:
	v_add_u32_e32 v4, vcc, s5, v0 ; 32080005
Shader main disassembly:
main:
BB0_0:
	s_mov_b32 s9, 0                                     ; BE890080
	s_load_dwordx4 s[0:3], s[8:9], 0x0                  ; C00A0004 00000000
	s_waitcnt lgkmcnt(0)                                ; BF8C007F
	buffer_load_format_xyzw v[0:3], v4, s[0:3], 0 idxen ; E00C2000 80000004
	s_waitcnt vmcnt(0)                                  ; BF8C0F70
	v_mov_b32_e32 v3, 1.0                               ; 7E0602F2
	exp pos0 v0, v1, v2, v3 done                        ; C40008CF 03020100
	s_endpgm                                            ; BF810000

*** SHADER STATS ***
SGPRS: 16
VGPRS: 8
Spilled SGPRs: 0
Spilled VGPRs: 0
Private memory VGPRs: 0
Code Size: 68 bytes
LDS: 0 blocks
Scratch: 0 bytes per wave
Max Waves: 8
********************



Thread 1 "a.out" hit Breakpoint 5, si_shader_create (sscreen=sscreen@entry=0x55555561e580, compiler=0x555555589f40, shader=shader@entry=0x555555af72f0, debug=0x555555af72f8)
    at ../src/gallium/drivers/radeonsi/si_shader.c:8039
8039		struct si_shader_selector *sel = shader->selector;
(gdb) bt
#0  si_shader_create (sscreen=sscreen@entry=0x55555561e580, compiler=0x555555589f40, shader=shader@entry=0x555555af72f0, debug=0x555555af72f8) at ../src/gallium/drivers/radeonsi/si_shader.c:8039
#1  0x00007ffff5a9f38a in si_build_shader_variant (shader=0x555555af72f0, thread_index=<optimized out>, low_priority=<optimized out>) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1715
#2  0x00007ffff5aa2d31 in si_shader_select_with_key (thread_index=-1, state=0x55555558a650, state=0x55555558a650, key=0x7fffffffd940, compiler_state=<optimized out>, sscreen=0x55555561e580)
    at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1977
#3  si_shader_select (ctx=ctx@entry=0x555555589a50, state=state@entry=0x55555558a650, compiler_state=compiler_state@entry=0x7fffffffda30) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:1995
#4  0x00007ffff5aa3ba1 in si_update_shaders (sctx=sctx@entry=0x555555589a50) at ../src/gallium/drivers/radeonsi/si_state_shaders.c:3445
#5  0x00007ffff5a9c019 in si_draw_vbo (ctx=0x555555589a50, info=0x7fffffffdd10) at ../src/gallium/drivers/radeonsi/si_state_draw.c:1362
#6  0x00007ffff593d809 in u_vbuf_draw_vbo (mgr=0x555555a9f830, info=0x7fffffffdd10) at ../src/gallium/auxiliary/util/u_vbuf.c:1182
#7  0x00007ffff5e6c1e7 in st_draw_vbo (ctx=<optimized out>, prims=<optimized out>, nr_prims=<optimized out>, ib=0x0, index_bounds_valid=<optimized out>, min_index=<optimized out>, 
    max_index=<optimized out>, tfb_vertcount=0x0, stream=0, indirect=0x0) at ../src/mesa/state_tracker/st_draw.c:236
#8  0x00007ffff5de6a62 in _mesa_draw_arrays (drawID=0, baseInstance=0, numInstances=1, count=3, start=0, mode=4, ctx=0x555555a5e700) at ../src/mesa/main/draw.c:408
#9  _mesa_draw_arrays (ctx=0x555555a5e700, mode=4, start=0, count=3, numInstances=1, baseInstance=0, drawID=0) at ../src/mesa/main/draw.c:385
#10 0x00007ffff5de7775 in _mesa_exec_DrawArrays (mode=4, start=0, count=3) at ../src/mesa/main/draw.c:565
#11 0x00005555555567dd in display() ()
#12 0x00007ffff7d18844 in ?? () from /lib/x86_64-linux-gnu/libglut.so.3
#13 0x00007ffff7d1c2e9 in fgEnumWindows () from /lib/x86_64-linux-gnu/libglut.so.3
#14 0x00007ffff7d18e6d in glutMainLoopEvent () from /lib/x86_64-linux-gnu/libglut.so.3
#15 0x00007ffff7d196a5 in glutMainLoop () from /lib/x86_64-linux-gnu/libglut.so.3
#16 0x0000555555556851 in main ()
(gdb) c
Continuing.
SHADER KEY
  part.ps.prolog.color_two_side = 0
  part.ps.prolog.flatshade_colors = 0
  part.ps.prolog.poly_stipple = 0
  part.ps.prolog.force_persp_sample_interp = 0
  part.ps.prolog.force_linear_sample_interp = 0
  part.ps.prolog.force_persp_center_interp = 0
  part.ps.prolog.force_linear_center_interp = 0
  part.ps.prolog.bc_optimize_for_persp = 0
  part.ps.prolog.bc_optimize_for_linear = 0
  part.ps.epilog.spi_shader_col_format = 0x4
  part.ps.epilog.color_is_int8 = 0x0
  part.ps.epilog.color_is_int10 = 0x0
  part.ps.epilog.last_cbuf = 0
  part.ps.epilog.alpha_func = 7
  part.ps.epilog.alpha_to_one = 0
  part.ps.epilog.poly_line_smoothing = 0
  part.ps.epilog.clamp_color = 0

Pixel Shader:
Shader main disassembly:
main:
BB0_0:
	v_mov_b32_e32 v0, 1.0 ; 7E0002F2
	v_mov_b32_e32 v1, 0   ; 7E020280
	v_mov_b32_e32 v2, 0   ; 7E040280
	v_mov_b32_e32 v3, 1.0 ; 7E0602F2
Shader epilog disassembly:
ps_epilog:
BB0_0:
	v_cvt_pkrtz_f16_f32 v0, v0, v1        ; D2960000 00020300
	v_cvt_pkrtz_f16_f32 v1, v2, v3        ; D2960001 00020702
	exp mrt0 v0, v0, v1, v1 done compr vm ; C4001C0F 00000100
	s_endpgm                              ; BF810000

*** SHADER CONFIG ***
SPI_PS_INPUT_ADDR = 0xf077
SPI_PS_INPUT_ENA  = 0x0020
*** SHADER STATS ***
SGPRS: 8
VGPRS: 24
Spilled SGPRs: 0
Spilled VGPRs: 0
Private memory VGPRs: 0
Code Size: 64 bytes
LDS: 0 blocks
Scratch: 0 bytes per wave
Max Waves: 8
********************



Thread 1 "a.out" hit Breakpoint 1, 0x00007ffff7fa5400 in glFlush () from /home/shiji/mesa-18.3.6/build/install/lib/x86_64-linux-gnu/libGL.so.1
(gdb) bt
#0  0x00007ffff7fa5400 in glFlush () from /home/shiji/mesa-18.3.6/build/install/lib/x86_64-linux-gnu/libGL.so.1
#1  0x00007ffff7d0f1be in glutSwapBuffers () from /lib/x86_64-linux-gnu/libglut.so.3
#2  0x00005555555567e2 in display() ()
#3  0x00007ffff7d18844 in ?? () from /lib/x86_64-linux-gnu/libglut.so.3
#4  0x00007ffff7d1c2e9 in fgEnumWindows () from /lib/x86_64-linux-gnu/libglut.so.3
#5  0x00007ffff7d18e6d in glutMainLoopEvent () from /lib/x86_64-linux-gnu/libglut.so.3
#6  0x00007ffff7d196a5 in glutMainLoop () from /lib/x86_64-linux-gnu/libglut.so.3
#7  0x0000555555556851 in main ()
(gdb) c
Continuing.




void si_init_shader_functions(struct si_context *sctx)
{
	sctx->atoms.s.spi_map.emit = si_emit_spi_map;
	sctx->atoms.s.scratch_state.emit = si_emit_scratch_state;

	sctx->b.create_vs_state = si_create_shader_selector;
	sctx->b.create_tcs_state = si_create_shader_selector;
	sctx->b.create_tes_state = si_create_shader_selector;
	sctx->b.create_gs_state = si_create_shader_selector;
	sctx->b.create_fs_state = si_create_shader_selector;

	sctx->b.bind_vs_state = si_bind_vs_shader;
	sctx->b.bind_tcs_state = si_bind_tcs_shader;
	sctx->b.bind_tes_state = si_bind_tes_shader;
	sctx->b.bind_gs_state = si_bind_gs_shader;
	sctx->b.bind_fs_state = si_bind_ps_shader;

	sctx->b.delete_vs_state = si_delete_shader_selector;
	sctx->b.delete_tcs_state = si_delete_shader_selector;
	sctx->b.delete_tes_state = si_delete_shader_selector;
	sctx->b.delete_gs_state = si_delete_shader_selector;
	sctx->b.delete_fs_state = si_delete_shader_selector;
}

(gdb) p *sel 
$3 = {
  reference = {
    count = 1
  }, 
  screen = 0x55555561e5b0, 
  ready = {
    val = 0
  }, 
  compiler_ctx_state = {
    compiler = 0x0, 
    debug = {
      async = true, 
      debug_message = 0x7ffff6125930 <u_async_debug_message>, 
      data = 0x7fffffffd960
    }, 
    is_debug_context = false
  }, 
  mutex = {
    __data = {
      __lock = 0, 
      __count = 0, 
      __owner = 0, 
      __nusers = 0, 
      __kind = 0, 
      __spins = 0, 
      __elision = 0, 
      __list = {
        __prev = 0x0, 
        __next = 0x0
      }
    }, 
    __size = '\000' <repeats 39 times>, 
    __align = 0
  }, 
  first_variant = 0x0, 
  last_variant = 0x0, 
  main_shader_part = 0x7fffd0000b20, 
  main_shader_part_ls = 0x0, 
  main_shader_part_es = 0x0, 
  gs_copy_shader = 0x0, 
  tokens = 0x555555a96310, 
  nir = 0x0, 
  so = {
    num_outputs = 0, 
    stride = {0, 0, 0, 0}, 
    output = {{
        register_index = 0, 
        start_component = 0, 
        num_components = 0, 
        output_buffer = 0, 
        dst_offset = 0, 
        stream = 0
--Type <RET> for more, q to quit, c to continue without paging--
      } <repeats 64 times>}
  }, 
  info = {
    num_tokens = 25, 
    num_inputs = 2 '\002', 
    num_outputs = 2 '\002', 
    input_semantic_name = '\000' <repeats 79 times>, 
    input_semantic_index = '\000' <repeats 79 times>, 
    input_interpolate = '\000' <repeats 79 times>, 
    input_interpolate_loc = '\000' <repeats 79 times>, 
    input_usage_mask = "\017\017", '\000' <repeats 77 times>, 
    input_cylindrical_wrap = '\000' <repeats 79 times>, 
    output_semantic_name = "\000\005", '\000' <repeats 77 times>, 
    output_semantic_index = '\000' <repeats 79 times>, 
    output_usagemask = "\017\017", '\000' <repeats 77 times>, 
    output_streams = '\000' <repeats 79 times>, 
    num_system_values = 0 '\000', 
    system_value_semantic_name = '\000' <repeats 79 times>, 
    processor = 0 '\000', 
    file_mask = {0, 0, 3, 3, 0 <repeats 11 times>}, 
    file_count = {0, 0, 2, 2, 0 <repeats 11 times>}, 
    file_max = {-1, -1, 1, 1, -1 <repeats 11 times>}, 
    const_file_max = {-1 <repeats 32 times>}, 
    const_buffers_declared = 0, 
    samplers_declared = 0, 
    sampler_targets = '\022' <repeats 128 times>, 
    sampler_type = '\000' <repeats 127 times>, 
    num_stream_output_components = "\b\000\000", 
    input_array_first = '\000' <repeats 79 times>, 
    input_array_last = '\000' <repeats 79 times>, 
    output_array_first = '\000' <repeats 79 times>, 
    output_array_last = '\000' <repeats 79 times>, 
    array_max = {0 <repeats 15 times>}, 
    immediate_count = 0, 
    num_instructions = 3, 
    num_memory_instructions = 0, 
    opcode_count = {0, 2, 0 <repeats 115 times>, 1, 0 <repeats 132 times>}, 
    reads_pervertex_outputs = 0 '\000', 
    reads_perpatch_outputs = 0 '\000', 
    reads_tessfactor_outputs = 0 '\000', 
    colors_read = 0 '\000', 
    colors_written = 0 '\000', 
    reads_position = 1 '\001', 
    reads_z = 0 '\000', 
    reads_samplemask = 0 '\000', 
    reads_tess_factors = 0 '\000', 
    writes_z = 0 '\000', 
    writes_stencil = 0 '\000', 
    writes_samplemask = 0 '\000', 
    writes_edgeflag = 0 '\000', 
    uses_kill = 0 '\000', 
    uses_persp_center = 0 '\000', 
--Type <RET> for more, q to quit, c to continue without paging--
    uses_persp_centroid = 0 '\000', 
    uses_persp_sample = 0 '\000', 
    uses_linear_center = 0 '\000', 
    uses_linear_centroid = 0 '\000', 
    uses_linear_sample = 0 '\000', 
    uses_persp_opcode_interp_centroid = 0 '\000', 
    uses_persp_opcode_interp_offset = 0 '\000', 
    uses_persp_opcode_interp_sample = 0 '\000', 
    uses_linear_opcode_interp_centroid = 0 '\000', 
    uses_linear_opcode_interp_offset = 0 '\000', 
    uses_linear_opcode_interp_sample = 0 '\000', 
    uses_instanceid = 0 '\000', 
    uses_vertexid = 0 '\000', 
    uses_vertexid_nobase = 0 '\000', 
    uses_basevertex = 0 '\000', 
    uses_primid = 0 '\000', 
    uses_frontface = 0 '\000', 
    uses_invocationid = 0 '\000', 
    uses_thread_id = "\000\000", 
    uses_block_id = "\000\000", 
    uses_block_size = 0 '\000', 
    uses_grid_size = 0 '\000', 
    writes_position = 1 '\001', 
    writes_psize = 0 '\000', 
    writes_clipvertex = 0 '\000', 
    writes_primid = 0 '\000', 
    writes_viewport_index = 0 '\000', 
    writes_layer = 0 '\000', 
    writes_memory = 0 '\000', 
    uses_doubles = 0 '\000', 
    uses_derivatives = 0 '\000', 
    uses_bindless_samplers = 0 '\000', 
    uses_bindless_images = 0 '\000', 
    clipdist_writemask = 0, 
    culldist_writemask = 0, 
    num_written_culldistance = 0, 
    num_written_clipdistance = 0, 
    images_declared = 0, 
    images_buffers = 0, 
    images_load = 0, 
    images_store = 0, 
    images_atomic = 0, 
    shader_buffers_declared = 0, 
    shader_buffers_load = 0, 
    shader_buffers_store = 0, shader_buffers_atomic = 0, 
    indirect_files = 0, 
    indirect_files_read = 0, 
    indirect_files_written = 0, 
    dim_indirect_files = 0, 
    const_buffers_indirect = 0, 
    properties = {0, 0, 0, 7, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0}, 
--Type <RET> for more, q to quit, c to continue without paging--
    max_depth = 0
  }, 
  tcs_info = {
    tessfactors_are_def_in_all_invocs = false
  }, 
  type = 0, 
  vs_needs_prolog = false, 
  force_correct_derivs_after_kill = false, 
  pa_cl_vs_out_cntl = 0, 
  clipdist_mask = 0 '\000', 
  culldist_mask = 0 '\000', 
  esgs_itemsize = 32, 
  lshs_vertex_stride = 36, 
  gs_input_verts_per_prim = 0, 
  gs_output_prim = 0, 
  gs_max_out_vertices = 0, 
  gs_num_invocations = 0, 
  max_gs_stream = 0, 
  gsvs_vertex_size = 0, 
  max_gsvs_emit_size = 0, 
  enabled_streamout_buffer_mask = 0, 
  color_attr_index = {0, 0}, 
  db_shader_control = 16, 
  colors_written_4bit = 0, 
  outputs_written_before_ps = 3, 
  outputs_written = 3, 
  patch_outputs_written = 0, 
  inputs_read = 0, 
  active_const_and_shader_buffers = 0, 
  active_samplers_and_images = 0
}




(gdb) p *sel->main_shader_part
$4 = {
  compiler_ctx_state = {
    compiler = 0x0, 
    debug = {
      async = false, 
      debug_message = 0x0, 
      data = 0x0
    }, 
    is_debug_context = false
  }, 
  selector = 0x555555a954d0, 
  previous_stage_sel = 0x0, 
  next_variant = 0x0, 
  prolog = 0x0, 
  previous_stage = 0x0, 
  prolog2 = 0x0, 
  epilog = 0x0, 
  pm4 = 0x0, 
  bo = 0x0, 
  scratch_bo = 0x0, 
  key = {
    part = {
      vs = {
        prolog = {
          instance_divisor_is_one = 0, 
          instance_divisor_is_fetched = 0, 
          ls_vgpr_fix = 0
        }
      }, 
      tcs = {
        ls_prolog = {
          instance_divisor_is_one = 0, 
          instance_divisor_is_fetched = 0, 
          ls_vgpr_fix = 0
        }, 
        ls = 0x0, 
        epilog = {
          prim_mode = 0, 
          invoc0_tess_factors_are_def = 0, 
          tes_reads_tess_factors = 0
        }
      }, 
      gs = {
        vs_prolog = {
          instance_divisor_is_one = 0, 
          instance_divisor_is_fetched = 0, 
          ls_vgpr_fix = 0
        }, 
        es = 0x0, 
        prolog = {
          tri_strip_adj_fix = 0, 
          gfx9_prev_is_vs = 0
--Type <RET> for more, q to quit, c to continue without paging--
        }
      }, 
      ps = {
        prolog = {
          color_two_side = 0, 
          flatshade_colors = 0, 
          poly_stipple = 0, 
          force_persp_sample_interp = 0, 
          force_linear_sample_interp = 0, 
          force_persp_center_interp = 0, 
          force_linear_center_interp = 0, 
          bc_optimize_for_persp = 0, 
          bc_optimize_for_linear = 0, 
          samplemask_log_ps_iter = 0
        }, 
        epilog = {
          spi_shader_col_format = 0, 
          color_is_int8 = 0, 
          color_is_int10 = 0, 
          last_cbuf = 0, 
          alpha_func = 0, 
          alpha_to_one = 0, 
          poly_line_smoothing = 0, 
          clamp_color = 0
        }
      }
    }, 
    as_es = 0, 
    as_ls = 0, 
    mono = {
      vs_fix_fetch = '\000' <repeats 15 times>, 
      u = {
        ff_tcs_inputs_to_copy = 0, 
        vs_export_prim_id = 0, 
        ps = {
          interpolate_at_sample_force_center = 0, 
          fbfetch_msaa = 0, 
          fbfetch_is_1D = 0, 
          fbfetch_layered = 0
        }
      }
    }, 
    opt = {
      kill_outputs = 0, 
      clip_disable = 0, 
      prefer_mono = 0
    }
  }, 
  ready = {
    val = 0
  }, 
  compilation_failed = false, 
--Type <RET> for more, q to quit, c to continue without paging--
  is_monolithic = false, 
  is_optimized = false, 
  is_binary_shared = false, 
  is_gs_copy_shader = false, 
  binary = {
    code_size = 88, 
    config_size = 0, 
    config_size_per_symbol = 0, 
    rodata_size = 0, 
    global_symbol_count = 0, 
    reloc_count = 0, 
    code = 0x7fffd0001980 "\003\002\002~\002\002\004~\202", 
    config = 0x0, 
    rodata = 0x0, 
    global_symbol_offsets = 0x0, 
    relocs = 0x0, 
    disasm_string = 0x7fffd00019e0 "main:\nBB0_0:\n\tv_mov_b32_e32 v1, s3", ' ' <repeats 64 times>, "; 7E020203\n\tv_mov_b32_e32 v2, s2", ' ' <repeats 64 times>, "; 7E04"..., 
    llvm_ir_string = 0x0
  }, 
  config = {
    num_sgprs = 16, 
    num_vgprs = 8, 
    spilled_sgprs = 0, 
    spilled_vgprs = 0, 
    private_mem_vgprs = 0, 
    lds_size = 0, 
    max_simd_waves = 8, 
    spi_ps_input_ena = 0, 
    spi_ps_input_addr = 0, 
    float_mode = 192, 
    scratch_bytes_per_wave = 0, 
    rsrc1 = 65, 
    rsrc2 = 0
  }, 
  info = {
    vs_output_param_offset = "\377\000", '\377' <repeats 38 times>, 
    num_input_sgprs = 9 '\t', 
    num_input_vgprs = 4 '\004', 
    face_vgpr_index = 0 '\000', 
    ancillary_vgpr_index = 0 '\000', 
    uses_instanceid = false, 
    nr_pos_exports = 1 '\001', 
    nr_param_exports = 1 '\001'
  }, 
  shader_log = 0x0, 
  shader_log_size = 0, 
  ctx_reg = {
    gs = {
      vgt_gsvs_ring_offset_1 = 0, 
      vgt_gsvs_ring_offset_2 = 0, 
      vgt_gsvs_ring_offset_3 = 0, 
      vgt_gs_out_prim_type = 0, 
--Type <RET> for more, q to quit, c to continue without paging--
      vgt_gsvs_ring_itemsize = 0, 
      vgt_gs_max_vert_out = 0, 
      vgt_gs_vert_itemsize = 0, 
      vgt_gs_vert_itemsize_1 = 0, 
      vgt_gs_vert_itemsize_2 = 0, 
      vgt_gs_vert_itemsize_3 = 0, 
      vgt_gs_instance_cnt = 0, 
      vgt_gs_onchip_cntl = 0, 
      vgt_gs_max_prims_per_subgroup = 0, 
      vgt_esgs_ring_itemsize = 0
    }, 
    vs = {
      vgt_gs_mode = 0, 
      vgt_primitiveid_en = 0, 
      vgt_reuse_off = 0, 
      spi_vs_out_config = 0, 
      spi_shader_pos_format = 0, 
      pa_cl_vte_cntl = 0
    }, 
    ps = {
      spi_ps_input_ena = 0, 
      spi_ps_input_addr = 0, 
      spi_baryc_cntl = 0, 
      spi_ps_in_control = 0, 
      spi_shader_z_format = 0, 
      spi_shader_col_format = 0, 
      cb_shader_mask = 0
    }
  }, 
  vgt_tf_param = 0, 
  vgt_vertex_reuse_block_cntl = 0
}

o
p sel->main_shader_part->binary->code



util_blitter_restore_vertex_states
o


0  si_get_shader_name (shader=0x7fffe4000b20, processor=0) at ../src/gallium/drivers/radeonsi/si_shader.c:5500
#1  0x00007ffff5ae5700 in si_compile_tgsi_shader (sscreen=sscreen@entry=0x55555561e5b0, compiler=compiler@entry=0x55555561ecc8, shader=shader@entry=0x7fffe4000b20, debug=debug@entry=0x555555a8e4f0)
    at ../src/gallium/drivers/radeonsi/si_shader.c:6742
#
