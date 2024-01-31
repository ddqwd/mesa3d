***Written by sj.yu***



# gs的内建变量及 hir生成

## gs hir的初始化及内建变量的设置

```c

void
_mesa_glsl_initialize_variables(exec_list *instructions,
				struct _mesa_glsl_parse_state *state)
{
   builtin_variable_generator gen(instructions, state);

   gen.generate_constants();
   gen.generate_uniforms();
   gen.generate_special_vars();

   gen.generate_varyings();

   switch (state->stage) {
   ...
   case MESA_SHADER_GEOMETRY:
      gen.generate_gs_special_vars();
      break;
   }
}


```


###  gs 专有内建常量的添加

```c

void
builtin_variable_generator::generate_constants()
{

    ...

   if (state->has_geometry_shader()) {
      add_const("gl_MaxVertexOutputComponents",
                state->Const.MaxVertexOutputComponents);
      add_const("gl_MaxGeometryInputComponents",
                state->Const.MaxGeometryInputComponents);
      add_const("gl_MaxGeometryOutputComponents",
                state->Const.MaxGeometryOutputComponents);
      add_const("gl_MaxFragmentInputComponents",
                state->Const.MaxFragmentInputComponents);
      add_const("gl_MaxGeometryTextureImageUnits",
                state->Const.MaxGeometryTextureImageUnits);
      add_const("gl_MaxGeometryOutputVertices",
                state->Const.MaxGeometryOutputVertices);
      add_const("gl_MaxGeometryTotalOutputComponents",
                state->Const.MaxGeometryTotalOutputComponents);
      add_const("gl_MaxGeometryUniformComponents",
                state->Const.MaxGeometryUniformComponents);

      /* Note: the GLSL 1.50-4.40 specs require
       * gl_MaxGeometryVaryingComponents to be present, and to be at least 64.
       * But they do not define what it means (and there does not appear to be
       * any corresponding constant in the GL specs).  However,
       * ARB_geometry_shader4 defines MAX_GEOMETRY_VARYING_COMPONENTS_ARB to
       * be the maximum number of components available for use as geometry
       * outputs.  So we assume this is a synonym for
       * gl_MaxGeometryOutputComponents.
       */
      add_const("gl_MaxGeometryVaryingComponents",
                state->Const.MaxGeometryOutputComponents);
   }

   if (state->is_version(420, 310)) {

      if (state->has_geometry_shader()) {
         add_const("gl_MaxGeometryAtomicCounterBuffers",
                   state->Const.MaxGeometryAtomicCounterBuffers);
      }
   }


   if (state->has_shader_image_load_store()) {

      if (state->has_geometry_shader()) {
         add_const("gl_MaxGeometryImageUniforms",
                   state->Const.MaxGeometryImageUniforms);
      }

   }

}
```

### gs 专有varying 的添加

```c
void
builtin_variable_generator::generate_varyings()
{

  if (state->stage == MESA_SHADER_GEOMETRY) {
      const glsl_type *per_vertex_in_type =
         this->per_vertex_in.construct_interface_instance();
      add_variable("gl_in", array(per_vertex_in_type, 0),
                   ir_var_shader_in, -1);
   }
  
}

```

```c

/**
 * 生成仅存在于几何着色器中的变量。
 */
void
builtin_variable_generator::generate_gs_special_vars()
{
   ir_variable *var;

   // 添加用于输出的变量，表示图元在层次中的位置
   var = add_output(VARYING_SLOT_LAYER, int_t, "gl_Layer");
   var->data.interpolation = INTERP_MODE_FLAT;

   // 如果是GLSL版本为410或启用了ARB_viewport_array或OES_viewport_array扩展，则添加视口索引变量
   if (state->is_version(410, 0) || state->ARB_viewport_array_enable ||
       state->OES_viewport_array_enable) {
      var = add_output(VARYING_SLOT_VIEWPORT, int_t, "gl_ViewportIndex");
      var->data.interpolation = INTERP_MODE_FLAT;
   }

   // 如果GLSL版本为400或320，或启用了ARB_gpu_shader5、OES_geometry_shader_enable或EXT_geometry_shader_enable扩展，则添加InvocationID系统值
   if (state->is_version(400, 320) || state->ARB_gpu_shader5_enable ||
       state->OES_geometry_shader_enable || state->EXT_geometry_shader_enable) {
      add_system_value(SYSTEM_VALUE_INVOCATION_ID, int_t, "gl_InvocationID");
   }

   /* 尽管 gl_PrimitiveID 在镶嵌控制着色器和镶嵌评估着色器中也会出现，
    * 但它在那里的功能与在几何着色器中的功能不同，因此我们将其
    * （以及其对应的 gl_PrimitiveIDIn）视为特殊的几何着色器变量。
    *
    * 注意，尽管将几何着色器输入变量的一般约定后缀为 "In" 没有被纳入
    * GLSL 1.50，但它在 gl_PrimitiveIDIn 的特定情况中是被使用的。
    * 因此，我们不需要将 gl_PrimitiveIDIn 视为 {ARB,EXT}_geometry_shader4
    * 扩展中的变量。
    */
   var = add_input(VARYING_SLOT_PRIMITIVE_ID, int_t, "gl_PrimitiveIDIn");
   var->data.interpolation = INTERP_MODE_FLAT;
   var = add_output(VARYING_SLOT_PRIMITIVE_ID, int_t, "gl_PrimitiveID");
   var->data.interpolation = INTERP_MODE_FLAT;
}



```

## gs hir生成时对layout专有标识符号的处理


### 特殊标识定义


```c
/**
 * Shader information needed by both gl_shader and gl_linked shader.
 */
struct gl_shader_info
{
    ...

    /**
     * GLSL 1.50 布局限定符导致的几何着色器状态。
     */
    struct {
       GLint VerticesOut;
       /**
        * 0 - 未在着色器中声明调用次数，或
        * 1 .. Const.MaxGeometryShaderInvocations
        */
       GLint Invocations;
       /**
        * GL_POINTS、GL_LINES、GL_LINES_ADJACENCY、GL_TRIANGLES，或
        * GL_TRIANGLES_ADJACENCY，或如果在该着色器中未设置则为 PRIM_UNKNOWN。
        */
       GLenum16 InputType;
       /**
        * GL_POINTS、GL_LINE_STRIP 或 GL_TRIANGLE_STRIP，或如果在该着色器中未设置则为 PRIM_UNKNOWN。
        */
       GLenum16 OutputType;
    } Geom;

    ...
}

```

### 处理流程

```c
static void
set_shader_inout_layout(struct gl_shader *shader,
		     struct _mesa_glsl_parse_state *state)
{

   switch (shader->Stage) {
   ...
   case MESA_SHADER_GEOMETRY:
      shader->info.Geom.VerticesOut = -1;
      if (state->out_qualifier->flags.q.max_vertices) {
         unsigned qual_max_vertices;
         if (state->out_qualifier->max_vertices->
               process_qualifier_constant(state, "max_vertices",
                                          &qual_max_vertices, true)) {

            if (qual_max_vertices > state->Const.MaxGeometryOutputVertices) {
               YYLTYPE loc = state->out_qualifier->max_vertices->get_location();
               _mesa_glsl_error(&loc, state,
                                "maximum output vertices (%d) exceeds "
                                "GL_MAX_GEOMETRY_OUTPUT_VERTICES",
                                qual_max_vertices);
            }
            shader->info.Geom.VerticesOut = qual_max_vertices;
         }
      }

      if (state->gs_input_prim_type_specified) {
         shader->info.Geom.InputType = state->in_qualifier->prim_type;
      } else {
         shader->info.Geom.InputType = PRIM_UNKNOWN;
      }

      if (state->out_qualifier->flags.q.prim_type) {
         shader->info.Geom.OutputType = state->out_qualifier->prim_type;
      } else {
         shader->info.Geom.OutputType = PRIM_UNKNOWN;
      }

      shader->info.Geom.Invocations = 0;
      if (state->in_qualifier->flags.q.invocations) {
         unsigned invocations;
         if (state->in_qualifier->invocations->
               process_qualifier_constant(state, "invocations",
                                          &invocations, false)) {

            YYLTYPE loc = state->in_qualifier->invocations->get_location();
            if (invocations > state->Const.MaxGeometryShaderInvocations) {
               _mesa_glsl_error(&loc, state,
                                "invocations (%d) exceeds "
                                "GL_MAX_GEOMETRY_SHADER_INVOCATIONS",
                                invocations);
            }
            shader->info.Geom.Invocations = invocations;
         }
      }
      break;
   ...
   default:
      /* Nothing to do. */
      break;
   }

   shader->bindless_sampler = state->bindless_sampler_specified;
   shader->bindless_image = state->bindless_image_specified;
   shader->bound_sampler = state->bound_sampler_specified;
   shader->bound_image = state->bound_image_specified;
}

```


### 处理stream


```c
static void
apply_layout_qualifier_to_variable(const struct ast_type_qualifier *qual,
                                   ir_variable *var,
                                   struct _mesa_glsl_parse_state *state,
                                   YYLTYPE *loc)
{

    ...

    if (state->stage == MESA_SHADER_GEOMETRY &&
       qual->flags.q.out && qual->flags.q.stream) {
      unsigned qual_stream;
      if (process_qualifier_constant(state, loc, "stream", qual->stream,
                                     &qual_stream) &&
          validate_stream_qualifier(loc, state, qual_stream)) {
         var->data.stream = qual_stream;
      }
   }

   ...
}

```


## gs hir对EmitVertex内建函数的添加

```c
/**
 * External API (exposing the built-in module to the rest of the compiler):
 *  @{
 */
void
_mesa_glsl_initialize_builtin_functions()
{
   mtx_lock(&builtins_lock);
   builtins.initialize();
       create_builtins();
            builtin_builder::create_builtins()
}



builtin_builder::create_builtins() {

   ...

   add_function("EmitVertex",   _EmitVertex(),   NULL);
   add_function("EndPrimitive", _EndPrimitive(), NULL);
   add_function("EmitStreamVertex",
                _EmitStreamVertex(gs_streams, glsl_type::uint_type),
                _EmitStreamVertex(gs_streams, glsl_type::int_type),
                NULL);
   add_function("EndStreamPrimitive",
                _EndStreamPrimitive(gs_streams, glsl_type::uint_type),
                _EndStreamPrimitive(gs_streams, glsl_type::int_type),
                NULL);
}


```

```
ir_function_signature *
builtin_builder::_EmitVertex()
{
   MAKE_SIG(glsl_type::void_type, gs_only, 0);

   ir_rvalue *stream = new(mem_ctx) ir_constant(0, 1);
   body.emit(new(mem_ctx) ir_emit_vertex(stream));

   return sig;
}

```

* 如此成功加入ir_emit_vertex 指令变量

# gs shader_program的生成

```c
static struct gl_program *
st_new_program(struct gl_context *ctx, GLenum target, GLuint id,
               bool is_arb_asm)
    ...
    case GL_TESS_CONTROL_PROGRAM_NV:
       case GL_TESS_EVALUATION_PROGRAM_NV:
       case GL_GEOMETRY_PROGRAM_NV: {
          struct st_common_program *prog = rzalloc(NULL,
                                                   struct st_common_program);
          return _mesa_init_gl_program(&prog->Base, target, id, is_arb_asm);
       }

```

* tcs, tes, gs使用的同一个st_common_program 变量

# gs的驱动shader状态设置


```c
st_set_prog_affected_state_flags(struct gl_program *prog)
   ...

   case MESA_SHADER_GEOMETRY:
      states = &(st_common_program(prog))->affected_states;

      *states = ST_NEW_GS_STATE |
                ST_NEW_RASTERIZER;

      set_affected_state_flags(states, prog,
                               ST_NEW_GS_CONSTANTS,
                               ST_NEW_GS_SAMPLER_VIEWS,
                               ST_NEW_GS_SAMPLERS,
                               ST_NEW_GS_IMAGES,
                               ST_NEW_GS_UBOS,
                               ST_NEW_GS_SSBOS,
                               ST_NEW_GS_ATOMICS);
      break;


}

```
# 链接对gs的特殊处理


### gs 对inout layout限定符的处理

```c

/**
 * 执行几何着色器的 max_vertices 和 primitive type 布局限定符的交叉验证，
 * 并将它们传播到链接的几何着色器和链接的着色器程序。
 */
static void
link_gs_inout_layout_qualifiers(struct gl_shader_program *prog,
                                struct gl_program *gl_prog,
                                struct gl_shader **shader_list,
                                unsigned num_shaders)
{
   /* 只有 GLSL 1.50+ 版本的几何着色器才定义了输入/输出布局限定符。*/
   if (gl_prog->info.stage != MESA_SHADER_GEOMETRY ||
       prog->data->Version < 150)
      return;

   int vertices_out = -1;

   gl_prog->info.gs.invocations = 0;
   gl_prog->info.gs.input_primitive = PRIM_UNKNOWN;
   gl_prog->info.gs.output_primitive = PRIM_UNKNOWN;

   /* 根据 GLSL 1.50 规范，第 46 页：
    *
    *     "程序中的所有几何着色器输出布局声明都必须声明相同的布局和
    *      max_vertices 的值。程序中必须至少有一个几何着色器输出布局
    *      声明，但不要求所有几何着色器（编译单元）都声明它。"
    */

   for (unsigned i = 0; i < num_shaders; i++) {
      struct gl_shader *shader = shader_list[i];

      if (shader->info.Geom.InputType != PRIM_UNKNOWN) {
         if (gl_prog->info.gs.input_primitive != PRIM_UNKNOWN &&
             gl_prog->info.gs.input_primitive !=
             shader->info.Geom.InputType) {
            linker_error(prog, "定义了冲突的几何着色器输入类型\n");
            return;
         }
         gl_prog->info.gs.input_primitive = shader->info.Geom.InputType;
      }

      if (shader->info.Geom.OutputType != PRIM_UNKNOWN) {
         if (gl_prog->info.gs.output_primitive != PRIM_UNKNOWN &&
             gl_prog->info.gs.output_primitive !=
             shader->info.Geom.OutputType) {
            linker_error(prog, "定义了冲突的几何着色器输出类型\n");
            return;
         }
         gl_prog->info.gs.output_primitive = shader->info.Geom.OutputType;
      }

      if (shader->info.Geom.VerticesOut != -1) {
         if (vertices_out != -1 &&
             vertices_out != shader->info.Geom.VerticesOut) {
            linker_error(prog, "定义了冲突的几何着色器输出顶点数量 (%d 和 %d)\n",
                         vertices_out, shader->info.Geom.VerticesOut);
            return;
         }
         vertices_out = shader->info.Geom.VerticesOut;
      }

      if (shader->info.Geom.Invocations != 0) {
         if (gl_prog->info.gs.invocations != 0 &&
             gl_prog->info.gs.invocations !=
             (unsigned) shader->info.Geom.Invocations) {
            linker_error(prog, "定义了冲突的几何着色器调用次数 (%d 和 %d)\n",
                         gl_prog->info.gs.invocations,
                         shader->info.Geom.Invocations);
            return;
         }
         gl_prog->info.gs.invocations = shader->info.Geom.Invocations;
      }
   }

   /* 现在只执行阶段内 -> 阶段间的传播，因为我们已经知道我们在执行
    * 正确类型的着色器程序来执行这个操作。
    */
   if (gl_prog->info.gs.input_primitive == PRIM_UNKNOWN) {
      linker_error(prog,
                   "几何着色器没有声明原始输入类型\n");
      return;
   }

   if (gl_prog->info.gs.output_primitive == PRIM_UNKNOWN) {
      linker_error(prog,
                   "几何着色器没有声明原始输出类型\n");
      return;
   }

   if (vertices_out == -1) {
      linker_error(prog,
                   "几何着色器没有声明 max_vertices\n");
      return;
   } else {
      gl_prog->info.gs.vertices_out = vertices_out;
   }

   if (gl_prog->info.gs.invocations == 0)
      gl_prog->info.gs.invocations = 1;
}


```
###  gs 输入数组大小的设置

```c
link_intrastage_shaders(void *mem_ctx,
                        struct gl_context *ctx,
                        struct gl_shader_program *prog,
                        struct gl_shader **shader_list,
                        unsigned num_shaders,
                        bool allow_missing_main)
{
    ...
   /* Set the size of geometry shader input arrays */
   if (linked->Stage == MESA_SHADER_GEOMETRY) {
      unsigned num_vertices =
         vertices_per_prim(gl_prog->info.gs.input_primitive);
      array_resize_visitor input_resize_visitor(num_vertices, prog,
                                                MESA_SHADER_GEOMETRY);
      foreach_in_list(ir_instruction, ir, linked->ir) {
         ir->accept(&input_resize_visitor);
            [array_resize_visitor::visit] 
            unsigned size = var->type->length;
            var->type = glsl_type::get_array_instance(var->type->fields.array,
                                                      this->num_vertices);
            var->data.max_array_access = this->num_vertices - 1;

            return visit_continue;

      }
   }

}

```
# Mesa gl_Program 生成 By glsl_to_tgsi_visitor

## 对gs outstream的处理

```c
void
glsl_to_tgsi_visitor::visit(ir_dereference_variable *ir)
{
   variable_storage *entry;
   ir_variable *var = ir->var;
   bool remove_array;

   if (handle_bound_deref(ir->as_dereference()))
      return;

   entry = find_variable_storage(ir->var);

   if (!entry) {
      switch (var->data.mode) {
      case ir_var_uniform:
        ...
      case ir_var_shader_in: {
        ...
      case ir_var_shader_out: {
         ...

         decl->mesa_index = var->data.location + FRAG_RESULT_MAX * var->data.index;
         decl->base_type = type_without_array->base_type;
         decl->usage_mask = u_bit_consecutive(component, num_components);

         if (var->data.stream & (1u << 31)) {
            decl->gs_out_streams = var->data.stream & ~(1u << 31);
         } else {
            assert(var->data.stream < 4);
            decl->gs_out_streams = 0;
            for (unsigned i = 0; i < num_components; ++i)
               decl->gs_out_streams |= var->data.stream << (2 * (component + i));
         }
         break;
      }
   }

}


```

## tgsi EmitVertex 指令的生成

```
void
glsl_to_tgsi_visitor::visit(ir_emit_vertex *ir)
{
   assert(this->prog->Target == GL_GEOMETRY_PROGRAM_NV);

   ir->stream->accept(this);
   emit_asm(ir, TGSI_OPCODE_EMIT, undef_dst, this->result);
}

```
# gs tgsi生成的额外处理


```c
st_program_string_notify( struct gl_context *ctx,
                                           GLenum target,
                                           struct gl_program *prog )

    ...

   else if (target == GL_GEOMETRY_PROGRAM_NV) {
      struct st_common_program *stgp = st_common_program(prog);

      st_release_basic_variants(st, stgp->Base.Target, &stgp->variants,
                                &stgp->tgsi);
      if (!st_translate_geometry_program(st, stgp))
         return false;

      if (st->gp == stgp)
	 st->dirty |= stgp->affected_states;
   }
   ...
}

```

## gs tgsi额外属性配置

```
/**
 * Translate a geometry program to create a new variant.
 */
bool
st_translate_geometry_program(struct st_context *st,
                              struct st_common_program *stgp)
{
   struct ureg_program *ureg;

   /* We have already compiled to NIR so just return */
   if (stgp->shader_program) {
      ...
      return true;
   }

   ureg = ureg_create_with_screen(PIPE_SHADER_GEOMETRY, st->pipe->screen);
   if (ureg == NULL)
      return false;

   ureg_property(ureg, TGSI_PROPERTY_GS_INPUT_PRIM,
                 stgp->Base.info.gs.input_primitive);
   ureg_property(ureg, TGSI_PROPERTY_GS_OUTPUT_PRIM,
                 stgp->Base.info.gs.output_primitive);
   ureg_property(ureg, TGSI_PROPERTY_GS_MAX_OUTPUT_VERTICES,
                 stgp->Base.info.gs.vertices_out);
   ureg_property(ureg, TGSI_PROPERTY_GS_INVOCATIONS,
                 stgp->Base.info.gs.invocations);

   st_translate_program_common(st, &stgp->Base, stgp->glsl_to_tgsi, ureg,
                               PIPE_SHADER_GEOMETRY, &stgp->tgsi);

   free_glsl_to_tgsi_visitor(stgp->glsl_to_tgsi);
   stgp->glsl_to_tgsi = NULL;
   return true;
}

```


## gs ureg_program 生成
```

/**
 * Translate a program. This is common code for geometry and tessellation
 * shaders.
 */
static void
st_translate_program_common(struct st_context *st,
                            struct gl_program *prog,
                            struct glsl_to_tgsi_visitor *glsl_to_tgsi,
                            struct ureg_program *ureg,
                            unsigned tgsi_processor,
                            struct pipe_shader_state *out_state)
{
   st_translate_program(st->ctx,
                        tgsi_processor,
                        ureg,
                        glsl_to_tgsi,
                        prog,
                        /* inputs */
                        num_inputs,
                        inputMapping,
                        inputSlotToAttr,
                        input_semantic_name,
                        input_semantic_index,
                        NULL,
                        /* outputs */
                        num_outputs,
                        outputMapping,
                        output_semantic_name,
                        output_semantic_index);

   if (tgsi_processor == PIPE_SHADER_COMPUTE) {
   } else {
      struct st_common_program *stcp = (struct st_common_program *) prog;
      out_state->tokens = ureg_get_tokens(ureg, &stcp->num_tgsi_tokens);
   }
   ureg_destroy(ureg);

   st_translate_stream_output_info(glsl_to_tgsi,
                                   outputMapping,
                                   &out_state->stream_output);

}
```

### gs ureg生成时对gs_out_streams的处理

```c
extern "C" enum pipe_error
st_translate_program(
   struct gl_context *ctx,
   enum pipe_shader_type procType,
   struct ureg_program *ureg,
   glsl_to_tgsi_visitor *program,
   const struct gl_program *proginfo,
   GLuint numInputs,
   const ubyte inputMapping[],
   const ubyte inputSlotToAttr[],
   const ubyte inputSemanticName[],
   const ubyte inputSemanticIndex[],
   const ubyte interpMode[],
   GLuint numOutputs,
   const ubyte outputMapping[],
   const ubyte outputSemanticName[],
   const ubyte outputSemanticIndex[])
{
 
    ...

     dst = ureg_DECL_output_layout(ureg,
                 (enum tgsi_semantic) outputSemanticName[slot],
                 outputSemanticIndex[slot],
                 decl->gs_out_streams,
                 slot, tgsi_usage_mask, decl->array_id, decl->size, decl->invariant);

    ...


}

struct ureg_dst
ureg_DECL_output_layout(struct ureg_program *ureg,
                        enum tgsi_semantic semantic_name,
                        unsigned semantic_index,
                        unsigned streams,
                        unsigned index,
                        unsigned usage_mask,
                        unsigned array_id,
                        unsigned array_size,
                        boolean invariant)
{
   unsigned i;

   assert(usage_mask != 0);
   assert(!(streams & 0x03) || (usage_mask & 1));
   assert(!(streams & 0x0c) || (usage_mask & 2));
   assert(!(streams & 0x30) || (usage_mask & 4));
   assert(!(streams & 0xc0) || (usage_mask & 8));

   for (i = 0; i < ureg->nr_outputs; i++) {
      if (ureg->output[i].semantic_name == semantic_name &&
          ureg->output[i].semantic_index == semantic_index) {
         if (ureg->output[i].array_id == array_id) {
            ureg->output[i].usage_mask |= usage_mask;
            goto out;
         }
         assert((ureg->output[i].usage_mask & usage_mask) == 0);
      }
   }

   if (ureg->nr_outputs < UREG_MAX_OUTPUT) {
      ureg->output[i].semantic_name = semantic_name;
      ureg->output[i].semantic_index = semantic_index;
      ureg->output[i].usage_mask = usage_mask;
      ureg->output[i].first = index;
      ureg->output[i].last = index + array_size - 1;
      ureg->output[i].array_id = array_id;
      ureg->output[i].invariant = invariant;
      ureg->nr_output_regs = MAX2(ureg->nr_output_regs, index + array_size);
      ureg->nr_outputs++;
   }
   else {
      set_bad( ureg );
      i = 0;
   }

out:
   ureg->output[i].streams |= streams;

   return ureg_dst_array_register(TGSI_FILE_OUTPUT, ureg->output[i].first,
                                  array_id);
}

```

# gs llvm IR生成

IR生成时机有两处:

1. 通过预编译生成IR

2. 通过ST_NEW_GS_STATE 生成

无论哪种通过都是通过st_get_basic_variant 生成


### 管线当前使用GS程序的设置

pipe层 触发_NEW_PROGRAM状态后

```c
static GLbitfield
update_program(struct gl_context *ctx)
{
   ...
   if (gsProg) {
      /* Use GLSL geometry shader */
      _mesa_reference_program(ctx, &ctx->GeometryProgram._Current, gsProg);
   } else {
      /* No geometry program */
      _mesa_reference_program(ctx, &ctx->GeometryProgram._Current, NULL);
   }

}

```
### gs basic variant的生成

```c
/**
 * Get/create a basic program variant.
 */
struct st_basic_variant *
st_get_basic_variant(struct st_context *st,
                     unsigned pipe_shader,
                     struct st_common_program *prog)
{

    ...

   if (!v) {
      /* create new */
      v = CALLOC_STRUCT(st_basic_variant);
      if (v) {

	 if (prog->tgsi.type == PIPE_SHADER_IR_NIR) {
        ...
	 } else
	    tgsi = prog->tgsi;
         /* fill in new variant */
         switch (pipe_shader) {
         case PIPE_SHADER_GEOMETRY:
            v->driver_shader = pipe->create_gs_state(pipe, &tgsi);
                    [jump si_create_shader_selector]
      }
   }

   return v;
}

```
### gs 在生成LLVM IR 对sel中的特殊限定符变量的设置

```c

static void *si_create_shader_selector(struct pipe_context *ctx,
				       const struct pipe_shader_state *state)
{
	struct si_screen *sscreen = (struct si_screen *)ctx->screen;
	struct si_context *sctx = (struct si_context*)ctx;
	struct si_shader_selector *sel = CALLOC_STRUCT(si_shader_selector);
	int i;

	if (!sel)
		return NULL;

	pipe_reference_init(&sel->reference, 1);
	sel->screen = sscreen;
	sel->compiler_ctx_state.debug = sctx->debug;
	sel->compiler_ctx_state.is_debug_context = sctx->is_debug;

	sel->so = state->stream_output;

	sel->type = sel->info.processor;
	p_atomic_inc(&sscreen->num_shaders_created);
	si_get_active_slot_masks(&sel->info,
				 &sel->active_const_and_shader_buffers,
				 &sel->active_samplers_and_images);

	/* Record which streamout buffers are enabled. */
	for (i = 0; i < sel->so.num_outputs; i++) {
		sel->enabled_streamout_buffer_mask |=
			(1 << sel->so.output[i].output_buffer) <<
			(sel->so.output[i].stream * 4);
	}
	if (state->type == PIPE_SHADER_IR_TGSI) {
		sel->tokens = tgsi_dup_tokens(state->tokens);

		tgsi_scan_shader(state->tokens, &sel->info);
	} else {
        ....
	}

	switch (sel->type) {
	case PIPE_SHADER_GEOMETRY:
		sel->gs_output_prim =
			sel->info.properties[TGSI_PROPERTY_GS_OUTPUT_PRIM];
		sel->gs_max_out_vertices =
			sel->info.properties[TGSI_PROPERTY_GS_MAX_OUTPUT_VERTICES];
		sel->gs_num_invocations =
			sel->info.properties[TGSI_PROPERTY_GS_INVOCATIONS];
		sel->gsvs_vertex_size = sel->info.num_outputs * 16;
		sel->max_gsvs_emit_size = sel->gsvs_vertex_size *
					  sel->gs_max_out_vertices;

		sel->max_gs_stream = 0;
		for (i = 0; i < sel->so.num_outputs; i++)
			sel->max_gs_stream = MAX2(sel->max_gs_stream,
						  sel->so.output[i].stream);

		sel->gs_input_verts_per_prim =
			u_vertices_per_prim(sel->info.properties[TGSI_PROPERTY_GS_INPUT_PRIM]);
		break;
	case PIPE_SHADER_TESS_CTRL:
	case PIPE_SHADER_VERTEX:
	case PIPE_SHADER_TESS_EVAL:
        ...
		sel->esgs_itemsize = util_last_bit64(sel->outputs_written) * 16;
		sel->lshs_vertex_stride = sel->esgs_itemsize;

		break;


	si_schedule_initial_compile(sctx, sel->info.processor, &sel->ready,
				    &sel->compiler_ctx_state, sel,
				    si_init_shader_selector_async);
	return sel;
}



```

* esgs_itemsize 直接影响了之后esgs ring是否需要更新,可见当无输出时，为0

### gs 在tgsi_shader_info gsinfo的生成时对gs IN的处理


```c
void
tgsi_scan_shader(const struct tgsi_token *tokens,
                 struct tgsi_shader_info *info)
{

   ...
   while (!tgsi_parse_end_of_tokens(&parse)) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         scan_instruction(info, &parse.FullToken.FullInstruction,
                          &current_depth);
         break;
      case TGSI_TOKEN_TYPE_DECLARATION:
         scan_declaration(info, &parse.FullToken.FullDeclaration);
         break;
      case TGSI_TOKEN_TYPE_IMMEDIATE:
         scan_immediate(info);
         break;
      case TGSI_TOKEN_TYPE_PROPERTY:
         scan_property(info, &parse.FullToken.FullProperty);
         break;
      default:
         assert(!"Unexpected TGSI token type");
      }
   }

   /* 几何着色器中的 IN 声明的维度必须根据输入原始类型进行推导。*/
   if (procType == PIPE_SHADER_GEOMETRY) {
      // 获取输入原始类型
      unsigned input_primitive = info->properties[TGSI_PROPERTY_GS_INPUT_PRIM];
      // 根据原始类型计算每个图元的顶点数量
      int num_verts = u_vertices_per_prim(input_primitive);
      int j;
   
      // 设置 TGSI_FILE_INPUT 文件的数量和最大索引
      info->file_count[TGSI_FILE_INPUT] = num_verts;
      info->file_max[TGSI_FILE_INPUT] =
            MAX2(info->file_max[TGSI_FILE_INPUT], num_verts - 1);
   
      // 针对每个顶点设置相应的文件掩码
      for (j = 0; j < num_verts; ++j) {
         info->file_mask[TGSI_FILE_INPUT] |= (1 << j);
      }
   }

   tgsi_parse_free(&parse);
}

```

### gs LLVM IR main函数的生成


#### 指令emit函数注册


```c
static bool si_compile_tgsi_main(struct si_shader_context *ctx)
{
	struct si_shader *shader = ctx->shader;
	struct si_shader_selector *sel = shader->selector;
	struct lp_build_tgsi_context *bld_base = &ctx->bld_base;

	// TODO clean all this up!
	switch (ctx->type) {
    ...
	case PIPE_SHADER_GEOMETRY:
		bld_base->emit_fetch_funcs[TGSI_FILE_INPUT] = fetch_input_gs;
		ctx->abi.load_inputs = si_nir_load_input_gs;
		ctx->abi.emit_vertex = si_llvm_emit_vertex;
		ctx->abi.emit_primitive = si_llvm_emit_primitive;
		ctx->abi.emit_outputs = si_llvm_emit_gs_epilogue;
		bld_base->emit_epilogue = si_tgsi_emit_gs_epilogue;
		break;
    ...

	}

	ctx->abi.load_ubo = load_ubo;
	ctx->abi.load_ssbo = load_ssbo;

	create_function(ctx);
	preload_ring_buffers(ctx);
    ...

	if (ctx->type == PIPE_SHADER_GEOMETRY) {
		int i;
		for (i = 0; i < 4; i++) {
			ctx->gs_next_vertex[i] =
				ac_build_alloca(&ctx->ac, ctx->i32, "");
		}
	}

    ...

	if (sel->tokens) {
		if (!lp_build_tgsi_llvm(bld_base, sel->tokens)) {
			fprintf(stderr, "Failed to translate shader from TGSI to LLVM\n");
			return false;
		}
	} else {
	}

	si_llvm_build_ret(ctx, ctx->return_value);
	return true;
}
```

#### gs LLVM main IR 函数创建声明

```c
static void create_function(struct si_shader_context *ctx)
{
    ...
	switch (type) {
    ...

	case PIPE_SHADER_GEOMETRY:
		declare_global_desc_pointers(ctx, &fninfo);
		declare_per_stage_desc_pointers(ctx, &fninfo, true);
		ctx->param_gs2vs_offset = add_arg(&fninfo, ARG_SGPR, ctx->i32);
		ctx->param_gs_wave_id = add_arg(&fninfo, ARG_SGPR, ctx->i32);

		/* VGPRs */
		add_arg_assign(&fninfo, ARG_VGPR, ctx->i32, &ctx->gs_vtx_offset[0]);
		add_arg_assign(&fninfo, ARG_VGPR, ctx->i32, &ctx->gs_vtx_offset[1]);
		add_arg_assign(&fninfo, ARG_VGPR, ctx->i32, &ctx->abi.gs_prim_id);
		add_arg_assign(&fninfo, ARG_VGPR, ctx->i32, &ctx->gs_vtx_offset[2]);
		add_arg_assign(&fninfo, ARG_VGPR, ctx->i32, &ctx->gs_vtx_offset[3]);
		add_arg_assign(&fninfo, ARG_VGPR, ctx->i32, &ctx->gs_vtx_offset[4]);
		add_arg_assign(&fninfo, ARG_VGPR, ctx->i32, &ctx->gs_vtx_offset[5]);
		add_arg_assign(&fninfo, ARG_VGPR, ctx->i32, &ctx->abi.gs_invocation_id);
		break;

    ...
	}

	si_create_function(ctx, "main", returns, num_returns, &fninfo,
			   si_get_max_workgroup_size(shader));


}


```
##### gs 最大工作组的配置 

```c
static unsigned si_get_max_workgroup_size(const struct si_shader *shader)
{
	switch (shader->selector->type) {
	case PIPE_SHADER_TESS_CTRL:
		/* Return this so that LLVM doesn't remove s_barrier
		 * instructions on chips where we use s_barrier. */
		return shader->selector->screen->info.chip_class >= CIK ? 128 : 64;

	case PIPE_SHADER_GEOMETRY:
		return shader->selector->screen->info.chip_class >= GFX9 ? 128 : 64;

	case PIPE_SHADER_COMPUTE:
		break; /* see below */

	default:
		return 0;
	}

	const unsigned *properties = shader->selector->info.properties;
	unsigned max_work_group_size =
	               properties[TGSI_PROPERTY_CS_FIXED_BLOCK_WIDTH] *
	               properties[TGSI_PROPERTY_CS_FIXED_BLOCK_HEIGHT] *
	               properties[TGSI_PROPERTY_CS_FIXED_BLOCK_DEPTH];

	if (!max_work_group_size) {
		/* This is a variable group size compute shader,
		 * compile it for the maximum possible group size.
		 */
		max_work_group_size = SI_MAX_VARIABLE_THREADS_PER_BLOCK;
	}
	return max_work_group_size;
}

```


#### gs 中对esgs ring的预加载设置

```c

/**
 * 加载ESGS和GSVS环形缓冲区资源描述符并保存变量以供以后使用。
 */
static void preload_ring_buffers(struct si_shader_context *ctx)
{
   LLVMBuilderRef builder = ctx->ac.builder;

   LLVMValueRef buf_ptr = LLVMGetParam(ctx->main_fn, ctx->param_rw_buffers);

   // 对于SI架构及其以下版本或者是ES或几何着色器的情况
   if (ctx->screen->info.chip_class <= VI &&
       (ctx->shader->key.as_es || ctx->type == PIPE_SHADER_GEOMETRY)) {
      unsigned ring =
         ctx->type == PIPE_SHADER_GEOMETRY ? SI_GS_RING_ESGS : SI_ES_RING_ESGS;
      LLVMValueRef offset = LLVMConstInt(ctx->i32, ring, 0);

      ctx->esgs_ring = ac_build_load_to_sgpr(&ctx->ac, buf_ptr, offset);
   }

   // 对于GS复制着色器
   if (ctx->shader->is_gs_copy_shader) {
      LLVMValueRef offset = LLVMConstInt(ctx->i32, SI_RING_GSVS, 0);

      ctx->gsvs_ring[0] = ac_build_load_to_sgpr(&ctx->ac, buf_ptr, offset);
   } else if (ctx->type == PIPE_SHADER_GEOMETRY) {
      const struct si_shader_selector *sel = ctx->shader->selector;
      LLVMValueRef offset = LLVMConstInt(ctx->i32, SI_RING_GSVS, 0);
      LLVMValueRef base_ring;

      base_ring = ac_build_load_to_sgpr(&ctx->ac, buf_ptr, offset);

      /* GSVS环形的概念布局是
       *   v0c0 .. vLv0 v0c1 .. vLc1 ..
       * 但是实际的内存布局在跨线程时是交叉的：
       *   t0v0c0 .. t15v0c0 t0v1c0 .. t15v1c0 ... t15vLcL
       *   t16v0c0 ..
       * 根据实际情况重写缓冲区描述符。
       */
      LLVMTypeRef v2i64 = LLVMVectorType(ctx->i64, 2);
      uint64_t stream_offset = 0;

      for (unsigned stream = 0; stream < 4; ++stream) {
         unsigned num_components;
         unsigned stride;
         unsigned num_records;
         LLVMValueRef ring, tmp;

         num_components = sel->info.num_stream_output_components[stream];
         if (!num_components)
            continue;

         stride = 4 * num_components * sel->gs_max_out_vertices;

         /* 对于 <= CIK 的架构，对于步幅字段有一个限制。 */
         assert(stride < (1 << 14));

         num_records = 64;

         ring = LLVMBuildBitCast(builder, base_ring, v2i64, "");
         tmp = LLVMBuildExtractElement(builder, ring, ctx->i32_0, "");
         tmp = LLVMBuildAdd(builder, tmp,
                           LLVMConstInt(ctx->i64,
                                        stream_offset, 0), "");
         stream_offset += stride * 64;

         ring = LLVMBuildInsertElement(builder, ring, tmp, ctx->i32_0, "");
         ring = LLVMBuildBitCast(builder, ring, ctx->v4i32, "");
         tmp = LLVMBuildExtractElement(builder, ring, ctx->i32_1, "");
         tmp = LLVMBuildOr(builder, tmp,
                           LLVMConstInt(ctx->i32,
                                        S_008F04_STRIDE(stride) |
                                        S_008F04_SWIZZLE_ENABLE(1), 0), "");
         ring = LLVMBuildInsertElement(builder, ring, tmp, ctx->i32_1, "");
         ring = LLVMBuildInsertElement(builder, ring,
                                       LLVMConstInt(ctx->i32, num_records, 0),
                                       LLVMConstInt(ctx->i32, 2, 0), "");
         ring = LLVMBuildInsertElement(builder, ring,
                                       LLVMConstInt(ctx->i32,
                                                    S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) |
                                                    S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
                                                    S_008F0C_DST_SEL_Z(V_008F0C_SQ_SEL_Z) |
                                                    S_008F0C_DST_SEL_W(V_008F0C_SQ_SEL_W) |
                                                    S_008F0C_NUM_FORMAT(V_008F0C_BUF_NUM_FORMAT_FLOAT) |
                                                    S_008F0C_DATA_FORMAT(V_008F0C_BUF_DATA_FORMAT_32) |
                                                    S_008F0C_ELEMENT_SIZE(1) | /* element_size = 4 (bytes) */
                                                    S_008F0C_INDEX_STRIDE(1) | /* index_stride = 16 (elements) */
                                                    S_008F0C_ADD_TID_ENABLE(1),
                                                    0),
                                       LLVMConstInt(ctx->i32, 3, 0), "");

         ctx->gsvs_ring[stream] = ring;
      }
   } else if (ctx->type == PIPE_SHADER_TESS_EVAL) {
      ctx->tess_offchip_ring = get_tess_ring_descriptor(ctx, TESS_OFFCHIP_RING_TES);
   }
}

```

#### gs LLVM IR main主体生成

```c
lp_build_tgsi_llvm(bld_base, sel->tokens)) (..)

```

# gs_copy_shader LLVM IR 的生成


GFX6-8：
每个软件阶段都有对应的硬件阶段。
LS和HS共享相同的LDS空间，因此LS可以将其输出存储到LDS，HS可以从LDS中读取。
HS、ES、GS的输出被存储在VRAM中，下一个阶段从VRAM中读取这些输出。
GS的输出需要传送到VRAM，因此必须通过在硬件VS阶段运行的GS复制着色器进行复制。

GCN上的传统几何流水线中, GCN GPU拥有用于顶点/几何处理的5个可编程硬件着色器阶段：LS（本地着色器）、HS（曲面细分着色器）、ES（导出着色器）、GS（几何着色器）、VS（顶点着色器）。这些硬件阶段并不完全映射到API中宣传的软件阶段。相反，驱动程序负责选择对于给定流水线需要使用哪些硬件阶段。
光栅化器只能消耗来自硬件VS的输出，因此流水线中的最后一个软件阶段必须被编译为硬件VS。这对于VS和TES来说是微不足道的（是的，曲面细分着色器在硬件中被视为“VS”），但GS则更为复杂。GS的输出被写入内存。驱动程序别无选择，只能编译一个单独的着色器，作为硬件VS运行，并将这个内存复制到光栅化器。在Mesa中，我们将其称为“GS复制着色器”（GS copy shader）。 （它不是API流水线的一部分，但是在使GS正常工作时是必需的。）


#### 生成点


```c
/**
 * Compile the main shader part or the monolithic shader as part of
 * si_shader_selector initialization. Since it can be done asynchronously,
 * there is no way to report compile failures to applications.
 */
static void si_init_shader_selector_async(void *job, int thread_index)
{
    ...
	/* The GS copy shader is always pre-compiled. */
	if (sel->type == PIPE_SHADER_GEOMETRY) {
		sel->gs_copy_shader = si_generate_gs_copy_shader(sscreen, compiler, sel, debug);
		if (!sel->gs_copy_shader) {
			fprintf(stderr, "radeonsi: can't create GS copy shader\n");
			return;
		}

		si_shader_vs(sscreen, sel->gs_copy_shader, sel);
	}
}

```


#### 生成gs_copy_shader LLVM IR

```c

/* 为与几何着色器配套的硬件顶点着色器生成代码 */
struct si_shader *
si_generate_gs_copy_shader(struct si_screen *sscreen,
                           struct ac_llvm_compiler *compiler,
                           struct si_shader_selector *gs_selector,
                           struct pipe_debug_callback *debug)
{
    struct si_shader_context ctx;
    struct si_shader *shader;
    LLVMBuilderRef builder;
    struct si_shader_output_values outputs[SI_MAX_VS_OUTPUTS];
    struct tgsi_shader_info *gsinfo = &gs_selector->info;
    int i, r;

    shader = CALLOC_STRUCT(si_shader);
    if (!shader)
        return NULL;

    /* 我们可以将fence保持永久信号，因为GS复制着色器只有在编译后才会在全局范围内可见。*/
    util_queue_fence_init(&shader->ready);

    shader->selector = gs_selector;
    shader->is_gs_copy_shader = true;

    si_init_shader_ctx(&ctx, sscreen, compiler);
    ctx.shader = shader;
    ctx.type = PIPE_SHADER_VERTEX;

    builder = ctx.ac.builder;

    create_function(&ctx);
    preload_ring_buffers(&ctx);

    LLVMValueRef voffset =
        LLVMBuildMul(ctx.ac.builder, ctx.abi.vertex_id,
                     LLVMConstInt(ctx.i32, 4, 0), "");

    /* 获取顶点流ID。*/
    LLVMValueRef stream_id;

    if (gs_selector->so.num_outputs)
        stream_id = si_unpack_param(&ctx, ctx.param_streamout_config, 24, 2);
    else
        stream_id = ctx.i32_0;

    /* 填充输出信息。 */
    for (i = 0; i < gsinfo->num_outputs; ++i) {
        outputs[i].semantic_name = gsinfo->output_semantic_name[i];
        outputs[i].semantic_index = gsinfo->output_semantic_index[i];

        for (int chan = 0; chan < 4; chan++) {
            outputs[i].vertex_stream[chan] =
                (gsinfo->output_streams[i] >> (2 * chan)) & 3;
        }
    }

    LLVMBasicBlockRef end_bb;
    LLVMValueRef switch_inst;

    end_bb = LLVMAppendBasicBlockInContext(ctx.ac.context, ctx.main_fn, "end");
    switch_inst = LLVMBuildSwitch(builder, stream_id, end_bb, 4);

    for (int stream = 0; stream < 4; stream++) {
        LLVMBasicBlockRef bb;
        unsigned offset;

        if (!gsinfo->num_stream_output_components[stream])
            continue;

        if (stream > 0 && !gs_selector->so.num_outputs)
            continue;

        bb = LLVMInsertBasicBlockInContext(ctx.ac.context, end_bb, "out");
        LLVMAddCase(switch_inst, LLVMConstInt(ctx.i32, stream, 0), bb);
        LLVMPositionBuilderAtEnd(builder, bb);

        /* 从GSVS环中获取顶点数据 */
        offset = 0;
        for (i = 0; i < gsinfo->num_outputs; ++i) {
            for (unsigned chan = 0; chan < 4; chan++) {
                if (!(gsinfo->output_usagemask[i] & (1 << chan)) ||
                    outputs[i].vertex_stream[chan] != stream) {
                    outputs[i].values[chan] = LLVMGetUndef(ctx.f32);
                    continue;
                }

                LLVMValueRef soffset = LLVMConstInt(ctx.i32,
                    offset * gs_selector->gs_max_out_vertices * 16 * 4, 0);
                offset++;

                outputs[i].values[chan] =
                    ac_build_buffer_load(&ctx.ac,
                                         ctx.gsvs_ring[0], 1,
                                         ctx.i32_0, voffset,
                                         soffset, 0, 1, 1,
                                         true, false);
            }
        }

        /* Streamout和导出。 */
        if (gs_selector->so.num_outputs) {
            si_llvm_emit_streamout(&ctx, outputs,
                                   gsinfo->num_outputs,
                                   stream);
        }

        if (stream == 0) {
            /* 顶点颜色夹持。
             *
             * 这使用在用户数据SGPR中加载的状态常量，添加了一个IF语句，
             * 如果常量为true，则夹持所有颜色。
             */
            struct lp_build_if_state if_ctx;
            LLVMValueRef v[2], cond = NULL;
            LLVMBasicBlockRef blocks[2];

            for (unsigned i = 0; i < gsinfo->num_outputs; i++) {
                if (gsinfo->output_semantic_name[i] != TGSI_SEMANTIC_COLOR &&
                    gsinfo->output_semantic_name[i] != TGSI_SEMANTIC_BCOLOR)
                    continue;

                /* 我们找到了颜色。 */
                if (!cond) {
                    /* 状态位于用户SGPR的第一位中。*/
                    cond = LLVMGetParam(ctx.main_fn,
                                        ctx.param_vs_state_bits);
                    cond = LLVMBuildTrunc(ctx.ac.builder, cond,
                                          ctx.i1, "");
                    lp_build_if(&if_ctx, &ctx.gallivm, cond);
                    /* 记住Phi的块。 */
                    blocks[0] = if_ctx.true_block;
                    blocks[1] = if_ctx.entry_block;
                }

                for (unsigned j = 0; j < 4; j++) {
                    /* 在true块中插入夹持。 */
                    v[0] = ac_build_clamp(&ctx.ac, outputs[i].values[j]);
                    v[1] = outputs[i].values[j];

                    /* 在endif块中插入Phi。 */
                    LLVMPositionBuilderAtEnd(ctx.ac.builder, if_ctx.merge_block);
                    outputs[i].values[j] = ac_build_phi(&ctx.ac, ctx.f32, 2, v, blocks);
                    LLVMPositionBuilderAtEnd(ctx.ac.builder, if_ctx.true_block);
                }
            }
            if (cond)
                lp_build_endif(&if_ctx);

            si_llvm_export_vs(&ctx, outputs, gsinfo->num_outputs);
        }

        LLVMBuildBr(builder, end_bb);
    }

    LLVMPositionBuilderAtEnd(builder, end_bb);

    LLVMBuildRetVoid(ctx.ac.builder);

    ctx.type = PIPE_SHADER_GEOMETRY; /* 覆盖用于着色器转储 */
    si_llvm_optimize_module(&ctx);

    r = si_compile_llvm(sscreen, &ctx.shader->binary,
                        &ctx.shader->config, ctx.compiler,
                        ctx.ac.module,
                        debug, PIPE_SHADER_GEOMETRY,
                        "GS Copy Shader", false);
    si_llvm_dispose(&ctx);

    if (r != 0) {
        FREE(shader);
        shader = NULL;
    }
    return shader;
}

```
* 将类型设置为vertex 实则就是同vertex as vs生成的代码一样， 不同的是没有num_input参数
* 日志通过" "GS Copy Shader" 识别


# gs 在si_update_shaders时配置的更新细节


```c

bool si_update_shaders(struct si_context *sctx)
{
	struct pipe_context *ctx = (struct pipe_context*)sctx;
	struct si_compiler_ctx_state compiler_state;
	struct si_state_rasterizer *rs = sctx->queued.named.rasterizer;
	struct si_shader *old_vs = si_get_vs_state(sctx);
	bool old_clip_disable = old_vs ? old_vs->key.opt.clip_disable : false;
	struct si_shader *old_ps = sctx->ps_shader.current;
	unsigned old_spi_shader_col_format =
		old_ps ? old_ps->key.part.ps.epilog.spi_shader_col_format : 0;
	int r;

	compiler_state.compiler = &sctx->compiler;
	compiler_state.debug = sctx->debug;
	compiler_state.is_debug_context = sctx->is_debug;

	/* Update stages before GS. */
	if (sctx->tes_shader.cso) {
        ...
	} else if (sctx->gs_shader.cso) {
		if (sctx->chip_class <= VI) {
			/* VS as ES */
			r = si_shader_select(ctx, &sctx->vs_shader,
					     &compiler_state);
			if (r)
				return false;
			si_pm4_bind_state(sctx, es, sctx->vs_shader.current->pm4);

			si_pm4_bind_state(sctx, ls, NULL);
			si_pm4_bind_state(sctx, hs, NULL);
		}
	} else {
        `...
	}

	/* Update GS. */
	if (sctx->gs_shader.cso) {
		r = si_shader_select(ctx, &sctx->gs_shader, &compiler_state);
		if (r)
			return false;
		si_pm4_bind_state(sctx, gs, sctx->gs_shader.current->pm4);
		si_pm4_bind_state(sctx, vs, sctx->gs_shader.cso->gs_copy_shader->pm4);

		if (!si_update_gs_ring_buffers(sctx))
			return false;
	} else {
		si_pm4_bind_state(sctx, gs, NULL);
		if (sctx->chip_class <= VI)
			si_pm4_bind_state(sctx, es, NULL);
	}

    ...

	sctx->do_update_shaders = false;
	return true;
}


```


## gs esgs,gsvs ring buffer的更新及描述符装填

```c

/* 初始化与ESGS / GSVS环形缓冲区相关的状态 */
static bool si_update_gs_ring_buffers(struct si_context *sctx)
{
    struct si_shader_selector *es = sctx->tes_shader.cso ? sctx->tes_shader.cso : sctx->vs_shader.cso;
    struct si_shader_selector *gs = sctx->gs_shader.cso;
    struct si_pm4_state *pm4;

    /* 芯片常量。*/
    unsigned num_se = sctx->screen->info.max_se;
    unsigned wave_size = 64;
    unsigned max_gs_waves = 32 * num_se; /* GCN架构每个SE最多32个波 */
    /* 在SI-CI上，该值来自VGT_GS_VERTEX_REUSE = 16。
     * 在VI+上，该值来自VGT_VERTEX_REUSE_BLOCK_CNTL = 30 (+2)。
     */
    unsigned gs_vertex_reuse = (sctx->chip_class >= VI ? 32 : 16) * num_se;
    unsigned alignment = 256 * num_se;
    /* 最大大小为63.999 MB每个SE。 */
    unsigned max_size = ((unsigned)(63.999 * 1024 * 1024) & ~255) * num_se;

    /* 计算最小大小。*/
    unsigned min_esgs_ring_size = align(es->esgs_itemsize * gs_vertex_reuse *
                                        wave_size, alignment);

    /* 这些是推荐的大小，而不是最小大小。 */
    unsigned esgs_ring_size = max_gs_waves * 2 * wave_size *
                              es->esgs_itemsize * gs->gs_input_verts_per_prim;
    unsigned gsvs_ring_size = max_gs_waves * 2 * wave_size *
                              gs->max_gsvs_emit_size;

    min_esgs_ring_size = align(min_esgs_ring_size, alignment);
    esgs_ring_size = align(esgs_ring_size, alignment);
    gsvs_ring_size = align(gsvs_ring_size, alignment);

    esgs_ring_size = CLAMP(esgs_ring_size, min_esgs_ring_size, max_size);
    gsvs_ring_size = MIN2(gsvs_ring_size, max_size);

    /* 如果着色器不使用它们，就不必分配某些环。
     * （例如，在ES和GS之间或GS和VS之间没有varyings）
     *
     * GFX9没有ESGS环。
     */
    bool update_esgs = sctx->chip_class <= VI &&
                       esgs_ring_size &&
                       (!sctx->esgs_ring ||
                        sctx->esgs_ring->width0 < esgs_ring_size);
    bool update_gsvs = gsvs_ring_size &&
                       (!sctx->gsvs_ring ||
                        sctx->gsvs_ring->width0 < gsvs_ring_size);

    if (!update_esgs && !update_gsvs)
        return true;

    if (update_esgs) {
        pipe_resource_reference(&sctx->esgs_ring, NULL);
        sctx->esgs_ring =
            pipe_aligned_buffer_create(sctx->b.screen,
                                       SI_RESOURCE_FLAG_UNMAPPABLE,
                                       PIPE_USAGE_DEFAULT,
                                       esgs_ring_size, alignment);
        if (!sctx->esgs_ring)
            return false;
    }

    if (update_gsvs) {
        pipe_resource_reference(&sctx->gsvs_ring, NULL);
        sctx->gsvs_ring =
            pipe_aligned_buffer_create(sctx->b.screen,
                                       SI_RESOURCE_FLAG_UNMAPPABLE,
                                       PIPE_USAGE_DEFAULT,
                                       gsvs_ring_size, alignment);
        if (!sctx->gsvs_ring)
            return false;
    }

    /* 创建"init_config_gs_rings"状态。*/
    pm4 = CALLOC_STRUCT(si_pm4_state);
    if (!pm4)
        return false;

    if (sctx->chip_class >= CIK) {
        if (sctx->esgs_ring) {
            assert(sctx->chip_class <= VI);
            si_pm4_set_reg(pm4, R_030900_VGT_ESGS_RING_SIZE,
                           sctx->esgs_ring->width0 / 256);
        }
        if (sctx->gsvs_ring)
            si_pm4_set_reg(pm4, R_030904_VGT_GSVS_RING_SIZE,
                           sctx->gsvs_ring->width0 / 256);
    } else {
        if (sctx->esgs_ring)
            si_pm4_set_reg(pm4, R_0088C8_VGT_ESGS_RING_SIZE,
                           sctx->esgs_ring->width0 / 256);
        if (sctx->gsvs_ring)
            si_pm4_set_reg(pm4, R_0088CC_VGT_GSVS_RING_SIZE,
                           sctx->gsvs_ring->width0 / 256);
    }

    /* 设置状态。*/
    if (sctx->init_config_gs_rings)
        si_pm4_free_state(sctx, sctx->init_config_gs_rings, ~0);
    sctx->init_config_gs_rings = pm4;

    if (!sctx->init_config_has_vgt_flush) {
        si_init_config_add_vgt_flush(sctx);
        si_pm4_upload_indirect_buffer(sctx, sctx->init_config);
    }

    /* 刷新上下文以重新发出init_config状态。*/
    sctx->initial_gfx_cs_size = 0; /* 强制刷新 */
    si_flush_gfx_cs(sctx, RADEON_FLUSH_ASYNC_START_NEXT_GFX_IB_NOW, NULL);

    /* 设置环绑定。*/
    if (sctx->esgs_ring) {
        assert(sctx->chip_class <= VI);
        si_set_ring_buffer(sctx, SI_ES_RING_ESGS,
                           sctx->esgs_ring, 0, sctx->esgs_ring->width0,
                           true, true, 4, 64, 0);
        si_set_ring_buffer(sctx, SI_GS_RING_ESGS,
                           sctx->esgs_ring, 0, sctx->esgs_ring->width0,
                           false, false, 0, 0, 0);
    }
    if (sctx->gsvs_ring) {
        si_set_ring_buffer(sctx, SI_RING_GSVS,
                           sctx->gsvs_ring, 0, sctx->gsvs_ring->width0,
                           false, false, 0, 0, 0);
    }

    return true;
}
```

* 该函数首先计算了esgs_ring, gs_ring buffer的所需最小尺寸min_esgs_ring_size,  esgs_ring_size， 在无es输出情况下，esgs_ring_size为0 ,在无gs输出情况下esgs_ring_size为0，为0表示无需使用.
* 之后， 如果需要更新esgs,gsvs则分别申请bo，并将大小设置在寄存器R_030900_VGT_ESGS_RING_SIZE, R_030904_VGT_GSVS_RING_SIZE中
* 最后通过si_set_ring_buffer 将esgs, gsvs写入rw对应的slot中供shader读取

### esgs ring buffer的写入

在预加载ring buffer后
```c

static void si_llvm_emit_es_epilogue(struct ac_shader_abi *abi,
				     unsigned max_outputs,
				     LLVMValueRef *addrs)
{
	struct si_shader_context *ctx = si_shader_context_from_abi(abi);
	struct si_shader *es = ctx->shader;
	struct tgsi_shader_info *info = &es->selector->info;
	LLVMValueRef soffset = LLVMGetParam(ctx->main_fn,
					    ctx->param_es2gs_offset);
	LLVMValueRef lds_base = NULL;
	unsigned chan;
	int i;

	for (i = 0; i < info->num_outputs; i++) {
		int param;

		if (info->output_semantic_name[i] == TGSI_SEMANTIC_VIEWPORT_INDEX ||
		    info->output_semantic_name[i] == TGSI_SEMANTIC_LAYER)
			continue;

		param = si_shader_io_get_unique_index(info->output_semantic_name[i],
						      info->output_semantic_index[i], false);

		for (chan = 0; chan < 4; chan++) {
			if (!(info->output_usagemask[i] & (1 << chan)))
				continue;

			LLVMValueRef out_val = LLVMBuildLoad(ctx->ac.builder, addrs[4 * i + chan], "");
			out_val = ac_to_integer(&ctx->ac, out_val);


			ac_build_buffer_store_dword(&ctx->ac,
						    ctx->esgs_ring,
						    out_val, 1, NULL, soffset,
						    (4 * param + chan) * 4,
						    1, 1, true, true);
		}
	}

}

```

### gsvs ring buffer的写入


在gs中使用EmitVertex之后发射函数

```c
/* Emit one vertex from the geometry shader */
static void si_tgsi_emit_vertex(
	const struct lp_build_tgsi_action *action,
	struct lp_build_tgsi_context *bld_base,
	struct lp_build_emit_data *emit_data)
{
	struct si_shader_context *ctx = si_shader_context(bld_base);
	unsigned stream = si_llvm_get_stream(bld_base, emit_data);

	si_llvm_emit_vertex(&ctx->abi, stream, ctx->outputs[0]);
}


/* 从几何着色器发射一个顶点 */
static void si_llvm_emit_vertex(struct ac_shader_abi *abi,
				unsigned stream,
				LLVMValueRef *addrs)
{
	struct si_shader_context *ctx = si_shader_context_from_abi(abi);
	struct tgsi_shader_info *info = &ctx->shader->selector->info;
	struct si_shader *shader = ctx->shader;
	struct lp_build_if_state if_state;
	LLVMValueRef soffset = LLVMGetParam(ctx->main_fn,
					    ctx->param_gs2vs_offset);
	LLVMValueRef gs_next_vertex;
	LLVMValueRef can_emit;
	unsigned chan, offset;
	int i;

	/* 将顶点属性值写入GSVS环 */
	gs_next_vertex = LLVMBuildLoad(ctx->ac.builder,
				       ctx->gs_next_vertex[stream],
				       "");

	/* 如果此线程已经发射了声明的最大顶点数，跳过写入：过多的顶点发射不应该有任何效果。
	 *
	 * 如果着色器没有对内存的写入，则将其终止。这将跳过进一步的内存加载，可能允许LLVM直接跳到末尾。
	 */
	can_emit = LLVMBuildICmp(ctx->ac.builder, LLVMIntULT, gs_next_vertex,
				 LLVMConstInt(ctx->i32,
					      shader->selector->gs_max_out_vertices, 0), "");

	bool use_kill = !info->writes_memory;
	if (use_kill) {
		ac_build_kill_if_false(&ctx->ac, can_emit);
	} else {
		lp_build_if(&if_state, &ctx->gallivm, can_emit);
	}

	offset = 0;
	for (i = 0; i < info->num_outputs; i++) {
		for (chan = 0; chan < 4; chan++) {
			if (!(info->output_usagemask[i] & (1 << chan)) ||
			    ((info->output_streams[i] >> (2 * chan)) & 3) != stream)
				continue;

			LLVMValueRef out_val = LLVMBuildLoad(ctx->ac.builder, addrs[4 * i + chan], "");
			LLVMValueRef voffset =
				LLVMConstInt(ctx->i32, offset *
					     shader->selector->gs_max_out_vertices, 0);
			offset++;

			voffset = LLVMBuildAdd(ctx->ac.builder, voffset, gs_next_vertex, "");
			voffset = LLVMBuildMul(ctx->ac.builder, voffset,
					       LLVMConstInt(ctx->i32, 4, 0), "");

			out_val = ac_to_integer(&ctx->ac, out_val);

			ac_build_buffer_store_dword(&ctx->ac,
						    ctx->gsvs_ring[stream],
						    out_val, 1,
						    voffset, soffset, 0,
						    1, 1, true, true);
		}
	}

	gs_next_vertex = LLVMBuildAdd(ctx->ac.builder, gs_next_vertex, ctx->i32_1, "");
	LLVMBuildStore(ctx->ac.builder, gs_next_vertex, ctx->gs_next_vertex[stream]);

	/* 如果写入了顶点数据，则发出顶点发射信号。 */
	if (offset) {
		ac_build_sendmsg(&ctx->ac, AC_SENDMSG_GS_OP_EMIT | AC_SENDMSG_GS | (stream << 8),
				 si_get_gs_wave_id(ctx));
	}

	if (!use_kill)
		lp_build_endif(&if_state);
}

```

### gs_copy_shader 对gsvs_ring的读取

see gs_copy_shader 

## gs LLVM IR prolog函数的生成

### 生成条件

```c


static int si_shader_select(struct pipe_context *ctx,
			    struct si_shader_ctx_state *state,
			    struct si_compiler_ctx_state *compiler_state)
	return si_shader_select_with_key(sctx->screen, state, compiler_state,
					 &key, -1);
	       r = si_shader_create(sscreen, compiler, shader, debug);
                ...
	         	/* Select prologs and/or epilogs. */
	         	switch (sel->type) {
                    ...
	         	case PIPE_SHADER_GEOMETRY:
	         		if (!si_shader_select_gs_parts(sscreen, compiler, shader, debug))
                        	shader->prolog2 = si_get_shader_part(sscreen, &sscreen->gs_prologs,
                        					    PIPE_SHADER_GEOMETRY, true,
                        					    &prolog_key, compiler, debug,
                        					    si_build_gs_prolog_function,
                        					    "Geometry Shader Prolog");
                            // for tri_strip_adj
                        	if (!shader->key.part.gs.prolog.tri_strip_adj_fix)
                        		return true;
	         		break;
```

### 生成

```c
/**
 * 构建GS前言函数。为带邻接的三角形条带旋转输入顶点。
 */
static void si_build_gs_prolog_function(struct si_shader_context *ctx,
					union si_shader_part_key *key)
{
	unsigned num_sgprs, num_vgprs;
	struct si_function_info fninfo;
	LLVMBuilderRef builder = ctx->ac.builder;
	LLVMTypeRef returns[48];
	LLVMValueRef func, ret;

	si_init_function_info(&fninfo);

	if (ctx->screen->info.chip_class >= GFX9) {
        ...
	} else {
		num_sgprs = GFX6_GS_NUM_USER_SGPR + 2;
		num_vgprs = 8;
	}

	for (unsigned i = 0; i < num_sgprs; ++i) {
		add_arg(&fninfo, ARG_SGPR, ctx->i32);
		returns[i] = ctx->i32;
	}

	for (unsigned i = 0; i < num_vgprs; ++i) {
		add_arg(&fninfo, ARG_VGPR, ctx->i32);
		returns[num_sgprs + i] = ctx->f32;
	}

	/* 创建函数。 */
	si_create_function(ctx, "gs_prolog", returns, num_sgprs + num_vgprs,
			   &fninfo, 0);
	func = ctx->main_fn;

	/* 为前言设置完整的EXEC掩码，因为我们只在这里处理寄存器。主着色器部分将设置正确的EXEC掩码。 */
	if (ctx->screen->info.chip_class >= GFX9 && !key->gs_prolog.is_monolithic)
		ac_init_exec_full_mask(&ctx->ac);

	/* 复制输入到输出。这应该是无操作，因为寄存器匹配，但它会防止编译器意外地覆盖它们。 */
	ret = ctx->return_value;
	for (unsigned i = 0; i < num_sgprs; i++) {
		LLVMValueRef p = LLVMGetParam(func, i);
		ret = LLVMBuildInsertValue(builder, ret, p, i, "");
	}
	for (unsigned i = 0; i < num_vgprs; i++) {
		LLVMValueRef p = LLVMGetParam(func, num_sgprs + i);
		p = ac_to_float(&ctx->ac, p);
		ret = LLVMBuildInsertValue(builder, ret, p, num_sgprs + i, "");
	}

	if (key->gs_prolog.states.tri_strip_adj_fix) {
		/* 为每个其他图元重新映射输入顶点。 */
		const unsigned gfx6_vtx_params[6] = {
			num_sgprs,
			num_sgprs + 1,
			num_sgprs + 3,
			num_sgprs + 4,
			num_sgprs + 5,
			num_sgprs + 6
		};
		const unsigned gfx9_vtx_params[3] = {
			num_sgprs,
			num_sgprs + 1,
			num_sgprs + 4,
		};
		LLVMValueRef vtx_in[6], vtx_out[6];
		LLVMValueRef prim_id, rotate;

		if (ctx->screen->info.chip_class >= GFX9) {
            ...
		} else {
			for (unsigned i = 0; i < 6; i++)
				vtx_in[i] = LLVMGetParam(func, gfx6_vtx_params[i]);
		}

		prim_id = LLVMGetParam(func, num_sgprs + 2);
		rotate = LLVMBuildTrunc(builder, prim_id, ctx->i1, "");

		for (unsigned i = 0; i < 6; ++i) {
			LLVMValueRef base, rotated;
			base = vtx_in[i];
			rotated = vtx_in[(i + 4) % 6];
			vtx_out[i] = LLVMBuildSelect(builder, rotate, rotated, base, "");
		}

		if (ctx->screen->info.chip_class >= GFX9) {
            ...
		} else {
			for (unsigned i = 0; i < 6; i++) {
				LLVMValueRef out;

				out = ac_to_float(&ctx->ac, vtx_out[i]);
				ret = LLVMBuildInsertValue(builder, ret, out,
							   gfx6_vtx_params[i], "");
			}
		}
	}

	LLVMBuildRet(builder, ret);
}

```


# 测试

## 测试draw 

使用piglit
triangle-strip-adj.shader_test

```

# 检查使用GL_TRIANGLE_STRIP_ADJACENCY传递到几何着色器的三角形是否具有正确的邻接信息。
#
# 该测试通过在条带中渲染4个具有邻接的三角形，然后检查每个几何着色器调用之间的6个顶点的关系来进行工作。
#
# 顶点在空间中的布局被选择为与GL 4.4规范第10.1.13节（带邻接的三角形）中的绘图匹配，
# 即（为了与规范中的图表匹配，从1开始计数）：
#
#     6  10
#     |\ |\
#     | \| \
#  2--3--7--11
#   \ |\ |\ |\
#    \| \| \| \
#     1--5--9--12
#      \ |\ |
#       \| \|
#        4  8
#
# 给定的几何着色器调用应该看到如下模式（为了与几何着色器中的数组索引匹配，从0开始计数）：
#
#  1--2--3
#   \ |\ |
#    \| \|
#     0--4
#      \ |
#       \|
#        5
#
# 因此，应满足以下数学关系：
#
# v2 - v1 = v3 - v2 = v4 - v0
# v0 - v1 = v4 - v2 = v5 - v0
# v0 - v2 = v4 - v3 = v5 - v4


[require]
GL >= 2.0
GLSL >= 1.50

[vertex shader]
in vec4 vertex;
out vec4 v;

void main()
{
  v = vertex;
}

[geometry shader]
layout(triangles_adjacency) in;
layout(triangle_strip, max_vertices = 3) out;

in vec4 v[6];
flat out int pass_to_fs;

void main()
{
  bool pass = true;
  if (distance(v[2] - v[1], v[3] - v[2]) > 1e-5) pass = false;
  if (distance(v[3] - v[2], v[4] - v[0]) > 1e-5) pass = false;
  if (distance(v[0] - v[1], v[4] - v[2]) > 1e-5) pass = false;
  if (distance(v[4] - v[2], v[5] - v[0]) > 1e-5) pass = false;
  if (distance(v[0] - v[2], v[4] - v[3]) > 1e-5) pass = false;
  if (distance(v[4] - v[3], v[5] - v[4]) > 1e-5) pass = false;

  for (int i = 0; i < 3; i++) {
    gl_Position = v[2*i];
    pass_to_fs = int(pass);
    EmitVertex();
  }
}

[fragment shader]
flat in int pass_to_fs;

void main()
{
  if (pass_to_fs != 0)
    gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
  else
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}

[vertex data]
vertex/float/2
-0.5 -0.25 #1
-1.0  0.25 #2
-0.5  0.25 #3
 0.0 -0.75 #4
 0.0 -0.25 #5
-0.5  0.75 #6
 0.0  0.25 #7
 0.5 -0.75 #8
 0.5 -0.25 #9
 0.0  0.75 #10
 0.5  0.25 #11
 1.0 -0.25 #12

[test]
#clear color 0.0 0.0 0.0 0.0
#clear
draw arrays GL_TRIANGLE_STRIP_ADJACENCY 0 12

# Probe inside each triangle
# relative probe rgba (0.33, 0.46) (0.0, 1.0, 0.0, 1.0)
# relative probe rgba (0.42, 0.54) (0.0, 1.0, 0.0, 1.0)
# relative probe rgba (0.58, 0.46) (0.0, 1.0, 0.0, 1.0)
# relative probe rgba (0.67, 0.54) (0.0, 1.0, 0.0, 1.0)

```


```
RW buffers slot 0 (GPU list):
        SQ_BUF_RSRC_WORD0 <- 0x00120000
        SQ_BUF_RSRC_WORD1 <- BASE_ADDRESS_HI = 1
                             STRIDE = 0
                             CACHE_SWIZZLE = 0
                             SWIZZLE_ENABLE = 1
        SQ_BUF_RSRC_WORD2 <- 0x00180000
        SQ_BUF_RSRC_WORD3 <- DST_SEL_X = SQ_SEL_X
                             DST_SEL_Y = SQ_SEL_Y
                             DST_SEL_Z = SQ_SEL_Z
                             DST_SEL_W = SQ_SEL_W
                             NUM_FORMAT = BUF_NUM_FORMAT_FLOAT
                             DATA_FORMAT = BUF_DATA_FORMAT_32
                             ELEMENT_SIZE = 1
                             INDEX_STRIDE = 3
                             ADD_TID_ENABLE = 1
                             ATC = 0
                             HASH_ENABLE = 0
                             HEAP = 0
                             MTYPE = 0
                             TYPE = SQ_RSRC_BUF

RW buffers slot 1 (GPU list):
        SQ_BUF_RSRC_WORD0 <- 0x00120000
        SQ_BUF_RSRC_WORD1 <- BASE_ADDRESS_HI = 1
                             STRIDE = 0
                             CACHE_SWIZZLE = 0
                             SWIZZLE_ENABLE = 0
        SQ_BUF_RSRC_WORD2 <- 0x00180000
        SQ_BUF_RSRC_WORD3 <- DST_SEL_X = SQ_SEL_X
                             DST_SEL_Y = SQ_SEL_Y
                             DST_SEL_Z = SQ_SEL_Z
                             DST_SEL_W = SQ_SEL_W
                             NUM_FORMAT = BUF_NUM_FORMAT_FLOAT
                             DATA_FORMAT = BUF_DATA_FORMAT_32
                             ELEMENT_SIZE = 0
                             INDEX_STRIDE = 0
                             ADD_TID_ENABLE = 0
                             ATC = 0
                             HASH_ENABLE = 0
                             HEAP = 0
                             MTYPE = 0
                             TYPE = SQ_RSRC_BUF

RW buffers slot 2 (GPU list):
        SQ_BUF_RSRC_WORD0 <- 0x002a0000


Buffer list (in units of pages = 4kB):
        Size    VM start page         VM end page           Usage
          32    0x0000000000100       0x0000000000120       DESCRIPTORS, SHADER_RINGS
     1048288    -- hole --
          32    0x0000000100000       0x0000000100020       QUERY, IB2, VERTEX_BUFFER, SHADER_BINARY, SHADER_RINGS
          32    0x0000000100020       0x0000000100040       IB1
          32    0x0000000100040       0x0000000100060       BORDER_COLORS
          32    0x0000000100060       0x0000000100080       TRACE
          32    -- hole --
          64    0x00000001000A0       0x00000001000E0       COLOR_BUFFER
          32    0x00000001000E0       0x0000000100100       QUERY
          32    0x0000000100100       0x0000000100120       SHADER_BINARY
         384    0x0000000100120       0x00000001002A0       SHADER_RINGS
         192    0x00000001002A0       0x0000000100360       SHADER_RINGS

....




------------------ IB2: Init GS rings begin ------------------
c0027900 SET_UCONFIG_REG:
00000240
00001800         VGT_ESGS_RING_SIZE <- 6144 (0x00001800)
00000c00         VGT_GSVS_RING_SIZE <- 3072 (0x00000c00)

------------------- IB2: Init GS rings end -------------------

```
## 测试图元类型

/**
 * \file primitive-types.c
 *
 * 验证几何着色器对于所有输入基元类型是否以正确的次数调用，
 * 并按正确的顺序传递输入顶点。
 *
 * 该测试使用一个简单的几何着色器，将每个输入顶点的gl_VertexID + 1
 * 复制到一个输出数组中，然后使用变换反馈（transform feedback）捕获结果
 * （加1是因为它对应于OpenGL规范中使用的基于1的编号，参见OpenGL规范2.6.1节（Primitive Types））。
 * 检查生成的数据是否与期望的顶点序列匹配。
 *
 * 作为附带的副作用，该测试验证实现为每个输入基元类型分配了正确的输入数组大小
 * （因为如果没有，几何着色器的编译将失败）。
 */

./glsl-1.50-geometry-primitive-types

##  
