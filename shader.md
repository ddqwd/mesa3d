# GLSL compiler源码分析

## 概述

>欢迎来到 Mesa 的 GLSL 编译器。以下是整个流程的简要概述：

>1) 基于 lex 和 yacc 的预处理器接收输入的着色器字符串，并生成一个新字符串，其中包含预处理后的着色器。这负责处理诸如 #if、#ifdef、#define 和预处理器宏调用之类的内容。请注意，#version、#extension 和其他一些内容将被直接传递。参见 glcpp/*。
>
>2) 基于 lex 和 yacc 的解析器接收预处理后的字符串并生成抽象语法树（AST）。在此阶段几乎不执行检查。请参阅 glsl_lexer.ll 和 glsl_parser.yy。
>
>3) AST 被转换为“HIR”（高级中间表示）。在此阶段生成构造函数，将函数调用解析为特定的函数签名，并执行所有语义检查。请参阅 ast_*.cpp 以获取转换的详细信息，并参阅 ir.h 以获取 IR 结构。
>
>4) 驱动程序（Mesa，或者独立二进制文件的 main.cpp）执行优化。这包括复制传播、死代码消除、常量折叠等。通常，驱动程序会循环调用优化，因为每次优化可能为其他优化提供额外的机会。请参阅 ir_*.cpp。
>
>5) 执行链接。这会检查顶点着色器的输出是否与片段着色器的输入匹配，并为 uniform、attribute 和 varying 分配位置。请参阅 linker.cpp。
>
>6) 此时，驱动程序可能会执行额外的优化，例如死代码消除先前无法删除函数或全局变量使用的情况，因为我们不知道将链接进来的其他代码是什么。
>
>7) 驱动程序从 IR 中生成代码，获取链接的着色器程序，并为每个阶段生成一个已编译的程序。请参阅 ../mesa/program/ir_to_mesa.cpp 以获取 Mesa IR 代码生成的详细信息。
>
>FAQ：
>
>Q: HIR、IR 和 LIR 有什么区别？
>
>A: 命名背后的想法是 ast_to_hir 将生成高级 IR（“HIR”），其中包含矩阵运算、结构赋值等内容。随后将进行一系列的降级传递，执行操作，例如将矩阵乘法拆分为一系列点乘/MAD 操作，将结构赋值拆分为组件赋值等，生成低级 IR（“LIR”）。
>
>然而，现在看来，每个驱动程序对 LIR 的需求可能不同。例如，915 代芯片组希望所有函数都内联，所有循环都展开，所有条件都展平，不允许变量数组访问，并拆分矩阵乘法。Mesa IR 后端对矩阵和结构赋值进行了拆分，但它可以支持函数调用和动态分支。965 顶点着色器 IR 后端甚至可能在不拆分的情况下处理一些矩阵运算，但 965 片段着色器 IR 后端希望将几乎所有操作都拆分成通道操作，并在其上执行优化。因此，没有一个单一的低级 IR 能够满足所有需求。因此，现在已不太使用这种低级 IR，每个驱动程序将执行一系列的降级传递，将 HIR 降低到它想要施加的任何限制，然后执行代码生成。
>
>Q: IR 的结构是如何设计的？
>
>A: 最好的入门方法是对一个着色器运行独立编译器：
>
>```bash
>./glsl_compiler --dump-lir ~/src/piglit/tests/shaders/glsl-orangebook-ch06-bump.frag
>```
>
>例如，`main()` 中的一个 `ir_instructions` 包含：
>
>```cpp
>(assign (constant bool (1)) (var_ref litColor) (expression vec3 * (var_ref SurfaceColor) (var_ref __retval)))
>```
>
>或更直观地：
>
>```
>                     (assign)
>                 /       |        \
>        (var_ref)  (expression *)  (constant bool 1)
>         /          /           \
>(litColor)      (var_ref)    (var_ref)
>                  /                  \
>           (SurfaceColor)          (__retval)
>```
>
>这是来自以下代码的：
>
>```cpp
>litColor = SurfaceColor * max(dot(normDelta, LightDir), 0.0);
>```
>
>（max 调用在此表达式树中未表示，因为它是一个被内联但未包含在此表达式树中的函数调用）
>
>每个节点都是 `ir_instruction` 的子类。特定的 `ir_instruction` 实例可能只在整个 IR 树中出现一次，除了 `ir_variables`，它们作为变量声明一次出现：
>
>```cpp
>(declare () vec3 normDelta)
>```
>
>并作为变量引用的目标多次出现：
>
>```cpp
>...
>(assign (constant bool (1)) (var_ref __retval) (expression float dot (var_ref normDelta) (var_ref LightDir) ) )
>...
>(assign (constant bool (1)) (var_ref __retval) (expression vec3 - (var_ref LightDir) (expression vec3 * (constant float (2.000000)) (expression vec3 * (expression float dot (var_ref normDelta) (var_ref LightDir) ) (var_ref normDelta) ) ) ) )
>...
>```
>
>每个节点都有一个类型。表达式可能涉及几种不同类型：
>
>```cpp
>(declare (uniform ) mat4 gl_ModelViewMatrix)
>((assign (constant bool (1)) (var_ref constructor_tmp) (expression vec4 * (var_ref gl_ModelViewMatrix) (var_ref gl_Vertex) ) )
>```
>
>表达式树可以是任意深度的，编译器试图保持其结构，以便诸如代
>
>数优化（（color * 1.0 == color）和（（mat1 * mat2）* vec == mat1 * （mat2 * vec）））或为代码生成识别操作模式（vec1 * vec2 + vec3 == mad(vec1, vec2, vec3)）等更容易。这会导致在实现某些优化时需要进行额外的技巧，比如在表达式树中导航时进行 CSE。
>
>Q: 为什么没有 SSA 表示？
>
>A: 将 IR 树转换为 SSA 形式使死代码消除、公共子表达式消除和许多其他优化变得更容易。然而，在我们主要以矢量为基础的语言中，有一些关键问题需要解决。我们在标量级别还是矢量级别上执行 SSA？如果我们在矢量级别执行，那么在处理诸如以下代码的情况时，我们将得到许多不同版本的变量：
>
>```cpp
>(assign (constant bool (1)) (swiz x (var_ref __retval) ) (var_ref a) )
>(assign (constant bool (1)) (swiz y (var_ref __retval) ) (var_ref b) )
>(assign (constant bool (1)) (swiz z (var_ref __retval) ) (var_ref c) )
>```
>
>如果每个组件的屏蔽更新都依赖于变量的先前值，那么我们在死代码消除方面可能会受到相当大的限制，并且可能无法识别常见表达式。另一方面，如果我们按通道操作，那么我们将倾向于优化对一个通道的操作，而不考虑其他通道，这样一台基于矢量的 GPU 将生成比不优化通道上的操作更糟糕的代码！
>
>再次强调，我们的优化需求显然受到目标架构的重大影响。目前，针对 Mesa IR 后端，SSA 对于生成优秀代码似乎并不重要，但我们确实希望在开发 965 片段着色器后端时执行一些基于 SSA 的优化。
>
>Q: 如何展开接受多个后端指令的指令？
>
>有时您必须在代码生成中执行扩展，例如，参见 ir_unop_sqrt 在 ir_to_mesa.cpp 中的处理。然而，在许多情况下，您可能希望对 IR 进行一遍处理，将非本机指令转换为一系列本机指令。例如，对于 Mesa 后端，我们有 ir_div_to_mul_rcp.cpp，因为 Mesa IR（和许多硬件后端）仅具有倒数指令，而没有除法。以这种方式实现非本机指令可以让常数折叠发生，因此（a / 2.0）在代码生成后变为（a * 0.5），而不是（a *（1.0 / 2.0））。
>
>Q: 如何处理与 IR 相关的特殊硬件指令？
>
>我们当前的理论是，如果多个目标具有某个操作的指令，那么我们可能应该能够在 IR 中表示它。通常，这以 ir_{bin,un}op 表达式类型的形式出现。例如，我们最初使用（a - floor(a)）实现 fract()，但 945 和 965 都有产生该结果的指令，并且这也将简化 mod() 的实现，因此添加了 ir_unop_fract。以下区域需要更新以添加新的表达式类型：
>
>- ir.h（新的枚举）
>- ir.cpp:operator_strs（用于 ir_reader）
>- ir_constant_expression.cpp（您可能希望能够进行常数折叠）
>- ir_validate.cpp（检查用户是否具有正确的类型）
>
>如果后端将看到新的表达式类型，则还可能需要更新后端：
>
>- ../mesa/program/ir_to_mesa.cpp
>
>然后，您可以从内置中使用新的表达式（如果所有后端都希望看到它），或者扫描 IR 并转换为使用新的表达式类型（参见 ir_mod_to_floor，例如）。
>
>Q: 编译器中的内存管理是如何处理的？
>
>A: Mesa 的 GLSL 编译器采用了 Samba 项目中开发的层次化内存分配器 "talloc"。该内存管理器设计用于简化垃圾回收，使得在优化通道等方面不必过多担心性能开销。它具有一些优秀的特性，包括低性能开销和易于调试的支持，这些支持可以轻松获得。
>
>一般来说，每个编译阶段都会创建一个 talloc 上下文，并从该上下文或其子上下文中分配内存。在阶段结束时，尚未释放的部分会被“偷”到一个新的上下文中，而旧的上下文则被释放，或者整个上下文保留供下一个阶段使用。
>
>对于 IR（Intermediate Representation，中间表示）的转换，使用一个临时上下文。在所有变换结束时，`reparent_ir` 函数将所有仍然存活的节点重新指定到着色器的 IR 列表下。旧上下文，其中包含死节点，将被释放。在开发单个 IR 转换通道时，这意味着您应该从临时上下文中分配指令节点，以便如果它变得死亡，它不会继续存在作为活动节点的子节点。目前，优化通道并未传递该临时上下文，因此它们通过调用附近 IR 节点上的 `talloc_parent()` 函数来找到它。由于 `talloc_parent()` 调用是昂贵的，因此许多通道会缓存首次调用的结果。清理所有优化通道以接受上下文参数，并避免调用 `talloc_parent()` 的工作，留作后续改进。

class ast_declaration : public ast_node {

class ast_function_definition : public ast_node {

class ast_interface_block : public ast_node {

class ast_jump_statement : public ast_node {

## ast 生成

yacc man] https://arcb.csc.ncsu.edu/~mueller/codeopt/codeopt00/y_man.pdf
[lex & yacc] http://www.nylxs.com/docs/lexandyacc.pdf

## IR 变量类的关系

IR变量类共有



```c
class ir_instruction : public exec_node {
class ir_rvalue : public ir_instruction {
class ir_dereference : public ir_rvalue {
class ir_dereference_variable : public ir_dereference {
```

### class exec_node

exec_node 一个双向循环链表

#### 接口

```cpp
   const exec_node *get_next() const;
   exec_node *get_next();
   const exec_node *get_prev() const;
   exec_node *get_prev();
   void remove();
   void self_link();
   void insert_after(exec_node *after);
   void insert_after(struct exec_list *after);
   void insert_before(exec_node *before);
   void insert_before(struct exec_list *before);
   void replace_with(exec_node *replacement);
   bool is_tail_sentinel() const;
   bool is_head_sentinel() const;
```

#### 成员变量

   struct exec_node *next;
   struct exec_node *prev;


### class ir_instruction : public exec_node

#### 构造函数

```c

protected:
   ir_instruction(enum ir_node_type t)
      : ir_type(t)
   {
   }
```

#### 普通函数接口

##### 打印 print
```c
   void print(void) const;
   void fprint(FILE *f) const;

```
##### 判断node type

```c
   bool is_rvalue() const
   bool is_dereference() const
   bool is_jump() const
```

##### dynamic_cast 实现

```
AS_BASE
AS_CHILD
```
#### 虚函数接口

##### 纯虚函数接口

```
   virtual void accept(ir_visitor *) = 0;
   virtual ir_visitor_status accept(ir_hierarchical_visitor *) = 0;
   virtual ir_instruction *clone(void *mem_ctx,
				 struct hash_table *ht) const = 0;
```

##### 虚函数接口

```c
/**
 * IR 相等性方法：如果引用的指令与当前指令返回相同的值，则返回 true。
 *
 * 这旨在用于常数传播和代数优化，尤其是对于右值。不计划支持其他指令类型（赋值、跳转、调用等）。
 */
virtual bool equals(const ir_instruction *ir,
                    enum ir_node_type ignore = ir_type_unset) const;

```
#### 成员变量

```c
   enum ir_node_type ir_type;

```

### 循环指令 ir_loop

```c
class ir_loop : public ir_instruction {

```
# exec_node

```c
struct exec_node {
   struct exec_node *next;
   struct exec_node *prev;
```
# exec_list

```c
struct exec_list {
   struct exec_node head_sentinel;
   struct exec_node tail_sentinel;

```

### ast

## struct ast_type_qualifier {

```c

struct ast_type_qualifier {
   DECLARE_RALLOC_CXX_OPERATORS(ast_type_qualifier);

   union flags {
      struct {
	 unsigned invariant:1;
         unsigned precise:1;
	 unsigned constant:1;
	 unsigned attribute:1;
	 unsigned varying:1;
	 unsigned in:1;
	 unsigned out:1;
	 unsigned centroid:1;
         unsigned sample:1;
	 unsigned patch:1;
	 unsigned uniform:1;
	 unsigned buffer:1;
	 unsigned shared_storage:1;
	 unsigned smooth:1;
	 unsigned flat:1;
	 unsigned noperspective:1;

	 /** \name Layout qualifiers for GL_ARB_fragment_coord_conventions */
	 /*@{*/
	 unsigned origin_upper_left:1;
	 unsigned pixel_center_integer:1;
	 /*@}*/

         /**
          * Flag set if GL_ARB_enhanced_layouts "align" layout qualifier is
          * used.
          */
         unsigned explicit_align:1;

	 /**
	  * Flag set if GL_ARB_explicit_attrib_location "location" layout
	  * qualifier is used.
	  */
	 unsigned explicit_location:1;
	 /**
	  * Flag set if GL_ARB_explicit_attrib_location "index" layout
	  * qualifier is used.
	  */
	 unsigned explicit_index:1;

	 /**
	  * Flag set if GL_ARB_enhanced_layouts "component" layout
	  * qualifier is used.
	  */
	 unsigned explicit_component:1;

         /**
          * Flag set if GL_ARB_shading_language_420pack "binding" layout
          * qualifier is used.
          */
         unsigned explicit_binding:1;

         /**
          * Flag set if GL_ARB_shader_atomic counter "offset" layout
          * qualifier is used.
          */
         unsigned explicit_offset:1;

         /** \name Layout qualifiers for GL_AMD_conservative_depth */
         /** \{ */
         unsigned depth_type:1;
         /** \} */

	 /** \name Layout qualifiers for GL_ARB_uniform_buffer_object */
	 /** \{ */
         unsigned std140:1;
         unsigned std430:1;
         unsigned shared:1;
         unsigned packed:1;
         unsigned column_major:1;
         unsigned row_major:1;
	 /** \} */

	 /** \name Layout qualifiers for GLSL 1.50 geometry shaders */
	 /** \{ */
	 unsigned prim_type:1;
	 unsigned max_vertices:1;
	 /** \} */

         /**
          * local_size_{x,y,z} flags for compute shaders.  Bit 0 represents
          * local_size_x, and so on.
          */
         unsigned local_size:3;

	 /** \name Layout qualifiers for ARB_compute_variable_group_size. */
	 /** \{ */
	 unsigned local_size_variable:1;
	 /** \} */

	 /** \name Layout and memory qualifiers for ARB_shader_image_load_store. */
	 /** \{ */
	 unsigned early_fragment_tests:1;
	 unsigned explicit_image_format:1;
	 unsigned coherent:1;
	 unsigned _volatile:1;
	 unsigned restrict_flag:1;
	 unsigned read_only:1; /**< "readonly" qualifier. */
	 unsigned write_only:1; /**< "writeonly" qualifier. */
	 /** \} */

         /** \name Layout qualifiers for GL_ARB_gpu_shader5 */
         /** \{ */
         unsigned invocations:1;
         unsigned stream:1; /**< Has stream value assigned  */
         unsigned explicit_stream:1; /**< stream value assigned explicitly by shader code */
         /** \} */

         /** \name Layout qualifiers for GL_ARB_enhanced_layouts */
         /** \{ */
         unsigned explicit_xfb_offset:1; /**< xfb_offset value assigned explicitly by shader code */
         unsigned xfb_buffer:1; /**< Has xfb_buffer value assigned  */
         unsigned explicit_xfb_buffer:1; /**< xfb_buffer value assigned explicitly by shader code */
         unsigned xfb_stride:1; /**< Is xfb_stride value yet to be merged with global values  */
         unsigned explicit_xfb_stride:1; /**< xfb_stride value assigned explicitly by shader code */
         /** \} */

	 /** \name Layout qualifiers for GL_ARB_tessellation_shader */
	 /** \{ */
	 /* tess eval input layout */
	 /* gs prim_type reused for primitive mode */
	 unsigned vertex_spacing:1;
	 unsigned ordering:1;
	 unsigned point_mode:1;
	 /* tess control output layout */
	 unsigned vertices:1;
	 /** \} */

         /** \name Qualifiers for GL_ARB_shader_subroutine */
	 /** \{ */
         unsigned subroutine:1;  /**< Is this marked 'subroutine' */
	 /** \} */

         /** \name Qualifiers for GL_KHR_blend_equation_advanced */
         /** \{ */
         unsigned blend_support:1; /**< Are there any blend_support_ qualifiers */
         /** \} */

         /**
          * Flag set if GL_ARB_post_depth_coverage layout qualifier is used.
          */
         unsigned post_depth_coverage:1;

         /**
          * Flags for the layout qualifers added by ARB_fragment_shader_interlock
          */

         unsigned pixel_interlock_ordered:1;
         unsigned pixel_interlock_unordered:1;
         unsigned sample_interlock_ordered:1;
         unsigned sample_interlock_unordered:1;

         /**
          * Flag set if GL_INTEL_conservartive_rasterization layout qualifier
          * is used.
          */
         unsigned inner_coverage:1;

         /** \name Layout qualifiers for GL_ARB_bindless_texture */
         /** \{ */
         unsigned bindless_sampler:1;
         unsigned bindless_image:1;
         unsigned bound_sampler:1;
         unsigned bound_image:1;
         /** \} */

         /** \name Layout qualifiers for GL_EXT_shader_framebuffer_fetch_non_coherent */
         /** \{ */
         unsigned non_coherent:1;
         /** \} */
      }
      /** \brief Set of flags, accessed by name. */
      q;

      /** \brief Set of flags, accessed as a bitmask. */
      bitset_t i;
   } flags;

   /** Precision of the type (highp/medium/lowp). */
   unsigned precision:2;

   /** Type of layout qualifiers for GL_AMD_conservative_depth. */
   unsigned depth_type:3;

   /**
    * Alignment specified via GL_ARB_enhanced_layouts "align" layout qualifier
    */
   ast_expression *align;

   /** Geometry shader invocations for GL_ARB_gpu_shader5. */
   ast_layout_expression *invocations;

   /**
    * Location specified via GL_ARB_explicit_attrib_location layout
    *
    * \note
    * This field is only valid if \c explicit_location is set.
    */
   ast_expression *location;
   /**
    * Index specified via GL_ARB_explicit_attrib_location layout
    *
    * \note
    * This field is only valid if \c explicit_index is set.
    */
   ast_expression *index;

   /**
    * Component specified via GL_ARB_enhaced_layouts
    *
    * \note
    * This field is only valid if \c explicit_component is set.
    */
   ast_expression *component;

   /** Maximum output vertices in GLSL 1.50 geometry shaders. */
   ast_layout_expression *max_vertices;

   /** Stream in GLSL 1.50 geometry shaders. */
   ast_expression *stream;

   /** xfb_buffer specified via the GL_ARB_enhanced_layouts keyword. */
   ast_expression *xfb_buffer;

   /** xfb_stride specified via the GL_ARB_enhanced_layouts keyword. */
   ast_expression *xfb_stride;

   /** global xfb_stride values for each buffer */
   ast_layout_expression *out_xfb_stride[MAX_FEEDBACK_BUFFERS];

   /**
    * Input or output primitive type in GLSL 1.50 geometry shaders
    * and tessellation shaders.
    */
   GLenum prim_type;

   /**
    * Binding specified via GL_ARB_shading_language_420pack's "binding" keyword.
    *
    * \note
    * This field is only valid if \c explicit_binding is set.
    */
   ast_expression *binding;

   /**
    * Offset specified via GL_ARB_shader_atomic_counter's or
    * GL_ARB_enhanced_layouts "offset" keyword, or by GL_ARB_enhanced_layouts
    * "xfb_offset" keyword.
    *
    * \note
    * This field is only valid if \c explicit_offset is set.
    */
   ast_expression *offset;

   /**
    * Local size specified via GL_ARB_compute_shader's "local_size_{x,y,z}"
    * layout qualifier.  Element i of this array is only valid if
    * flags.q.local_size & (1 << i) is set.
    */
   ast_layout_expression *local_size[3];

   /** Tessellation evaluation shader: vertex spacing (equal, fractional even/odd) */
   enum gl_tess_spacing vertex_spacing;

   /** Tessellation evaluation shader: vertex ordering (CW or CCW) */
   GLenum ordering;

   /** Tessellation evaluation shader: point mode */
   bool point_mode;

   /** Tessellation control shader: number of output vertices */
   ast_layout_expression *vertices;

   /**
    * Image format specified with an ARB_shader_image_load_store
    * layout qualifier.
    *
    * \note
    * This field is only valid if \c explicit_image_format is set.
    */
   GLenum image_format;

   /**
    * Base type of the data read from or written to this image.  Only
    * the following enumerants are allowed: GLSL_TYPE_UINT,
    * GLSL_TYPE_INT, GLSL_TYPE_FLOAT.
    *
    * \note
    * This field is only valid if \c explicit_image_format is set.
    */
   glsl_base_type image_base_type;

   /**
    * Return true if and only if an interpolation qualifier is present.
    */
   bool has_interpolation() const;

   /**
    * Return whether a layout qualifier is present.
    */
   bool has_layout() const;

   /**
    * Return whether a storage qualifier is present.
    */
   bool has_storage() const;

   /**
    * Return whether an auxiliary storage qualifier is present.
    */
   bool has_auxiliary_storage() const;

   /**
    * Return true if and only if a memory qualifier is present.
    */
   bool has_memory() const;

   /**
    * Return true if the qualifier is a subroutine declaration.
    */
   bool is_subroutine_decl() const;

   bool merge_qualifier(YYLTYPE *loc,
			_mesa_glsl_parse_state *state,
                        const ast_type_qualifier &q,
                        bool is_single_layout_merge,
                        bool is_multiple_layouts_merge = false);

   /**
    * Validate current qualifier against the global out one.
    */
   bool validate_out_qualifier(YYLTYPE *loc,
                               _mesa_glsl_parse_state *state);

   /**
    * Merge current qualifier into the global out one.
    */
   bool merge_into_out_qualifier(YYLTYPE *loc,
                                 _mesa_glsl_parse_state *state,
                                 ast_node* &node);

   /**
    * Validate current qualifier against the global in one.
    */
   bool validate_in_qualifier(YYLTYPE *loc,
                              _mesa_glsl_parse_state *state);

   /**
    * Merge current qualifier into the global in one.
    */
   bool merge_into_in_qualifier(YYLTYPE *loc,
                                _mesa_glsl_parse_state *state,
                                ast_node* &node);

   /**
    * Push pending layout qualifiers to the global values.
    */
   bool push_to_global(YYLTYPE *loc,
                       _mesa_glsl_parse_state *state);

   bool validate_flags(YYLTYPE *loc,
                       _mesa_glsl_parse_state *state,
                       const ast_type_qualifier &allowed_flags,
                       const char *message, const char *name);

   ast_subroutine_list *subroutine_list;
};



```

## ast_node

所有抽象语法节点的基类


```
   exec_node link;

```
通过link 组成二叉树

hir 转换成高级中间标识形式
## ast_declarator_list

ast_declarator_list : public ast_node

继承了hir

```
   ast_fully_specified_type *type;
   exec_list declarations;

```

## class ast_fully_specified_type : public ast_node 

## hir生成

```
// 构建hir
_mesa_ast_to_hir(exec_list *instructions=shader->ir, struct _mesa_glsl_parse_state *state)
   _mesa_glsl_initialize_variables(instructions, state);
        builtin_variable_generator gen(instructions, state);
        gen.generate_constants();
        gen.generate_uniforms();
        gen.generate_special_vars();
        gen.generate_varyings();
            ....
            if (state->has_clip_distance()) {
                add_varying(VARYING_SLOT_CLIP_DIST0, array(float_t, 0),
                   "gl_ClipDistance");
            ...

        switch (state->stage) {
        case MESA_SHADER_VERTEX:
           gen.generate_vs_special_vars();
               if (state->is_version(130, 300))
                  add_system_value(slot=SYSTEM_VALUE_VERTEX_ID, int_t, name="gl_VertexID");
                    return add_variable(name, type=int_t, ir_var_system_value, slot);
           break;
        }

   // 根据不同的语法类型调用对应功能的类处理
   foreach_list_typed (ast_node, ast, link, & state->translation_unit)
      ast->hir(instructions, state);
```

### ast->hir处理声明变量

```
ir_rvalue *ast_declarator_list::hir(exec_list *instructions,
                                    struct _mesa_glsl_parse_state *state)
{
   // 获取解析状态
   void *ctx = state;
   // 声明类型、类型名称等变量
   const struct glsl_type *decl_type;
   const char *type_name = NULL;
   ir_rvalue *result = NULL;

   // 获取glsl声明类型：
   this->type->specifier->hir(instructions, state);

   decl_type = this->type->glsl_type(& type_name, state);


   // 遍历声明列表中的每个声明
   foreach_list_typed (ast_declaration, decl, link, &this->declarations) {
      const struct glsl_type *var_type;
      ir_variable *var;
      const char *identifier = decl->identifier;

      // 处理子例程声明的情况
      if (this->type->qualifier.is_subroutine_decl()) {
      }

      // 处理数组类型
      var_type = process_array_type(&loc, decl_type, decl->array_specifier,
                                    state);
      // 创建IR变量
      var = new(ctx) ir_variable(var_type, identifier, ir_var_auto);

      // 根据类型限定符将含义保存在给ir_variable.data中
      apply_type_qualifier_to_variable(& this->type->qualifier, var, state,
                                       & loc, false);

      // 根据布局限定符将含义保存在给ir_variable.data中
      apply_layout_qualifier_to_variable(&this->type->qualifier, var, state,&loc);
            if (qual->flags.q.explicit_location) {
               apply_explicit_location(qual, var, state, loc);
                    unsigned qual_location;

                    // 获取uniform变量 location指定位置 
                    if (!process_qualifier_constant(state, loc, "location", qual->location,
                                   &qual_location)) {
                      return;
                   }

            } else if (qual->flags.q.explicit_index) {
            } else if (qual->flags.q.explicit_component) {
            }
            if (qual->flags.q.explicit_binding) {
               apply_explicit_binding(state, loc, var, var->type, qual);
            }
            apply_bindless_qualifier_to_variable(qual, var, state, loc);
            ....
 

      // 进行一系列语义检查，包括限定符的合法性、类型匹配等
      // ...

      exec_list initializer_instructions;

      // 处理初始化
      result = process_initializer(var, decl, this->type,
             &initializer_instructions, state);

      // 将变量添加到IR指令流
      instructions->push_head(var);
   }

   // 返回最后一个声明的rvalue
   return result;
}
```
## 
```

builtin_variable_generator::add_variable(const char *name,
                                         const glsl_type *type,
                                         enum ir_variable_mode mode, int slot)
   ir_variable *var = new(symtab) ir_variable(type, name, mode);
   switch (var->data.mode) {
   case ir_var_auto:
   case ir_var_shader_in:
      var->data.read_only = true;
      break;
    }
   // location 为SYSTEM_VALUE_VERTEX_ID
   var->data.location = slot;
   var->data.explicit_location = (slot >= 0);
   var->data.explicit_index = 0;
   instructions->push_tail(var);
   symtab->add_variable(var);
        bool glsl_symbol_table::add_variable(ir_variable *v)
	        int added = _mesa_symbol_table_add_symbol(table, v->name, entry);
                _mesa_hash_table_insert(table->ht, new_sym->name, new_sym);

   return var;
```


## 创建program

```

_mesa_CreateProgram(void)
   return create_shader_program(ctx);
   name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ShaderObjects, 1);
   shProg = _mesa_new_shader_program(name);
        shProg = rzalloc(NULL, struct gl_shader_program);
        shProg->data = _mesa_create_shader_program_data();
             struct gl_shader_program_data *data;
             data = rzalloc(NULL, struct gl_shader_program_data);
      init_shader_program(shProg);
         prog->AttributeBindings = string_to_uint_map_ctor();
         prog->FragDataBindings = string_to_uint_map_ctor();
         prog->FragDataIndexBindings = string_to_uint_map_ctor();
   return name;
```

## 附着program

```
_mesa_AttachShader_no_error(GLuint program, GLuint shader)
   attach_shader_no_error(ctx, program, shader);
   shProg = _mesa_lookup_shader_program(ctx, program);
   sh = _mesa_lookup_shader(ctx, shader);
   attach_shader(ctx, shProg, sh);
        GLuint n = shProg->NumShaders;
        shProg->Shaders = realloc(shProg->Shaders,
                             (n + 1) * sizeof(struct gl_shader *));
        _mesa_reference_shader(ctx, &shProg->Shaders[n], sh);
        shProg->NumShaders++;
    
```

## 链接shader

```c
_mesa_LinkProgram_no_error(GLuint programObj)
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program(ctx, programObj);
   link_program_no_error(ctx, shProg);
        link_program(ctx, shProg, true);
        _mesa_glsl_link_shader(ctx, shProg);

            // 重复
            prog->data = _mesa_create_shader_program_data();

            link_shaders(ctx, prog);

            if (prog->data->LinkStatus && !ctx->Driver.LinkShader(ctx, prog)) {
                            [jump functions->LinkShader = st_link_shader;]
               prog->data->LinkStatus = LINKING_FAILURE;
            }

link_shaders(struct gl_context *ctx, struct gl_shader_program *prog)
   void *mem_ctx = ralloc_context(NULL); // temporary linker context
   struct gl_shader **shader_list[MESA_SHADER_STAGES];
   unsigned num_shaders[MESA_SHADER_STAGES];

   for (int i = 0; i < MESA_SHADER_STAGES; i++) 
      shader_list[i] = (struct gl_shader **)
         calloc(prog->NumShaders, sizeof(struct gl_shader *));
      num_shaders[i] = 0;

   for (unsigned i = 0; i < prog->NumShaders; i++) {
      gl_shader_stage shader_type = prog->Shaders[i]->Stage;
      shader_list[shader_type][num_shaders[shader_type]] = prog->Shaders[i];
      num_shaders[shader_type]++;

    // 链接所有shader
   for (int stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      if (num_shaders[stage] > 0) 
         gl_linked_shader *const sh =
            link_intrastage_shaders(mem_ctx, ctx, prog, shader_list[stage],
                                    num_shaders[stage], false);
         prog->_LinkedShaders[stage] = sh;
         prog->data->linked_stages |= 1 << stage;

   check_explicit_uniform_locations(ctx, prog);
   // 检查...


   //Store the gl_FragDepth layout in the gl_shader_program struct.
   store_fragdepth_layout(prog);

   // 链接变量和uniforms
   link_varyings_and_uniforms(first, last, ctx, prog, mem_ctx)


   /* Process UBOs */
   if (!interstage_cross_validate_uniform_blocks(prog, false))
      goto done;

   /* Process SSBOs */
   if (!interstage_cross_validate_uniform_blocks(prog, true))
      goto done;

```

###  gl_shader_program生成 

```c
/**
 * 将一组同一阶段的着色器合并，生成一个链接后的着色器
 *
 * \note
 * 如果此函数提供了单个着色器，则会克隆它，并返回新的着色器。
 */
struct gl_linked_shader *
link_intrastage_shaders(void *mem_ctx,
                        struct gl_context *ctx,
                        struct gl_shader_program *prog,
                        struct gl_shader **shader_list,
                        unsigned num_shaders,
                        bool allow_missing_main)
{
   struct gl_uniform_block *ubo_blocks = NULL;
   struct gl_uniform_block *ssbo_blocks = NULL;

   /* 检查在多个着色器中定义的全局变量是否一致。*/
  cross_validate_globals(ctx, prog, shader_list[i]->ir, &variables,
                             false);

   /* 检查在多个着色器中定义的接口块是否一致。*/
   validate_intrastage_interface_blocks(prog, (const gl_shader **)shader_list,
                                        num_shaders);

   /* 检查每个函数签名在所有着色器中是否只有一个定义。 */
   ir_function_signature *other_sig =
      other->exact_matching_signature(NULL, &sig->parameters);

   /* 找到定义main的着色器，并克隆它。
    *
    * 从克隆开始，搜索未定义的引用。如果找到一个，找到定义它的着色器。
    * 克隆引用并将其添加到着色器中。重复，直到没有未定义的引用或引用无法解析。
    */
   gl_shader *main = NULL;
   for (unsigned i = 0; i < num_shaders; i++) {
      if (_mesa_get_main_function_signature(shader_list[i]->symbols)) {
         main = shader_list[i];
         break;
      }
   }

   gl_linked_shader *linked = rzalloc(NULL, struct gl_linked_shader);
   // MESA_SHADER_VERTEX = 0,
   linked->Stage = shader_list[0]->Stage;

   /* 创建程序并将其附加到链接后的着色器 */
   struct gl_program *gl_prog =
      ctx->Driver.NewProgram(ctx,
                             _mesa_shader_stage_to_program(shader_list[0]->Stage),
                             prog->Name, false);
            switch (target) {
            case GL_VERTEX_PROGRAM_ARB: {
                  struct st_vertex_program *prog = rzalloc(NULL,
                                               struct st_vertex_program);
                  return _mesa_init_gl_program(&prog->Base, target, id, is_arb_asm) -> &prog->base
            }

   _mesa_reference_shader_program_data(ctx, &gl_prog->sh.data, prog->data);

   /* 不要使用 _mesa_reference_program()，只接管所有权 */
   // vs,fs,gs.. gl_shader_program
   linked->Program = gl_prog;

   linked->ir = new(linked) exec_list;
   clone_ir_list(mem_ctx, linked->ir, main->ir);

   //检查layout限定符
   link_fs(tcs,tes,gs,cs)_inout_layout_qualifiers(prog, linked, shader_list, num_shaders);

   // 检查xfb

   // 检查bindless
   link_bindless_layout_qualifiers(prog, shader_list, num_shaders);

   populate_symbol_table(linked, shader_list[0]->symbols);
       sh->symbols = new(sh) glsl_symbol_table;
       _mesa_glsl_copy_symbols_from_table(sh->ir, symbols, sh->symbols);


   /* 最终链接后的着色器中主函数的指针（即包含主函数的原始着色器的副本）。 */
   ir_function_signature *const main_sig =
      _mesa_get_main_function_signature(linked->symbols);

   /* 移动除变量声明或函数声明之外的所有指令到main。*/
  exec_node *insertion_point =
         move_non_declarations(linked->ir, (exec_node *) &main_sig->body, false,

    // 链接函数调用
   if (!link_function_calls(prog, linked, shader_list, num_shaders)) ...

   if (linked->Stage != MESA_SHADER_FRAGMENT)
      link_output_variables(linked, shader_list, num_shaders);
         if (var->data.mode == ir_var_shader_out &&
                       !symbols->get_variable(var->name)) {
                    var = var->clone(linked_shader, NULL);
                    symbols->add_variable(var);
                 }



   /* 链接此阶段内定义的统一块。 */
   link_uniform_blocks(mem_ctx, ctx, prog, linked, &ubo_blocks,
                       &num_ubo_blocks, &ssbo_blocks, &num_ssbo_blocks);

   /* 将ubo块复制到链接的着色器列表中 */
   linked->Program->sh.UniformBlocks =
      ralloc_array(linked, gl_uniform_block *, num_ubo_blocks);
   ralloc_steal(linked, ubo_blocks);
   for (unsigned i = 0; i < num_ubo_blocks; i++) {
      linked->Program->sh.UniformBlocks[i] = &ubo_blocks[i];
   }
   linked->Program->info.num_ubos = num_ubo_blocks;

   /* 将ssbo块复制到链接的着色器列表中 */
   linked->Program->sh.ShaderStorageBlocks =...

   /* 设置几何着色器输入数组的大小 */
   if (linked->Stage == MESA_SHADER_GEOMETRY) {
      unsigned num_vertices =
         vertices_per_prim(gl_prog->info.gs.input_primitive);
      array_resize_visitor input_resize_visitor(num_vertices, prog,
                                                MESA_SHADER_GEOMETRY);
      foreach_in_list(ir_instruction, ir, linked->ir) {
         ir->accept(&input_resize_visitor);
      }
   }

   if (ctx->Const.VertexID_is_zero_based)
      lower_vertex_id(linked);

   if (ctx->Const.LowerCsDerivedVariables)
      lower_cs_derived(linked);

   return linked;
}

```

## LINK varying_and_uniforms

```c
/**
 * 链接变量和统一变量
 *
 * @param first 要链接的第一个着色器类型
 * @param last 要链接的最后一个着色器类型
 * @param ctx 上下文
 * @param prog 着色器程序
 * @param mem_ctx 内存上下文
 *
 * @return 链接成功返回true，否则返回false
 */
static bool
link_varyings_and_uniforms(unsigned first, unsigned last,
                           struct gl_context *ctx,
                           struct gl_shader_program *prog, void *mem_ctx)
{
   /* 将所有通用着色器输入和输出标记为未配对。 */
   for (unsigned i = MESA_SHADER_VERTEX; i <= MESA_SHADER_FRAGMENT; i++) {
      if (prog->_LinkedShaders[i] != NULL) {
         link_invalidate_variable_locations(prog->_LinkedShaders[i]->ir);
      }
   }

   unsigned prev = first;
   for (unsigned i = prev + 1; i <= MESA_SHADER_FRAGMENT; i++) {
      if (prog->_LinkedShaders[i] == NULL)
         continue;

      match_explicit_outputs_to_inputs(prog->_LinkedShaders[prev],
                                       prog->_LinkedShaders[i]);

            ir_variable *explicit_locations[MAX_VARYINGS_INCL_PATCH][4] ={ {NULL, NULL} };
   /* Find all shader outputs in the "producer" stage.
    */
   foreach_in_list(ir_instruction, node, producer->ir) {
 
      prev = i;
   }

   /* 分配通用顶点属性或颜色位置 */
   if (!assign_attribute_or_color_locations(mem_ctx, prog, &ctx->Const,
                                            MESA_SHADER_VERTEX, true)) {
      return false;
   }

   /* 分配通用片段着色器输入或颜色位置 */
   if (!assign_attribute_or_color_locations(mem_ctx, prog, &ctx->Const,
                                            MESA_SHADER_FRAGMENT, true)) {
      return false;
   }

   prog->last_vert_prog = prog->_LinkedShaders[i]->Program;

   /* 链接varyings */
   if (!link_varyings(prog, first, last, ctx, mem_ctx))
      return false;

   /* 链接并验证uniforms */
   link_and_validate_uniforms(ctx, prog);

   if (!prog->data->LinkStatus)
      return false;

   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
         continue;

      const struct gl_shader_compiler_options *options =
         &ctx->Const.ShaderCompilerOptions[i];

      /* 降低缓冲区接口块的引用 */
      if (options->LowerBufferInterfaceBlocks)
         lower_ubo_reference(prog->_LinkedShaders[i],
                             options->ClampBlockIndicesToArrayBounds,
                             ctx->Const.UseSTD430AsDefaultPacking);

      /* 降低shared引用 */
      if (i == MESA_SHADER_COMPUTE)
         lower_shared_reference(ctx, prog, prog->_LinkedShaders[i]);

      /* 降低向量引用 */
      lower_vector_derefs(prog->_LinkedShaders[i]);
      /* 执行vec索引到swizzle的转换 */
      do_vec_index_to_swizzle(prog->_LinkedShaders[i]->ir);
   }

   return true;
}
```

1. 标记所有通用着色器输入和输出为未配对。
2. 在不同着色器阶段之间匹配显式输出和输入。
3. 分配通用顶点属性或颜色位置。
4. 分配通用片段着色器输入或颜色位置。
5. 设置`last_vert_prog`为最后一个顶点着色器程序。
6. 链接varyings。
7. 链接并验证uniforms。
8. 降低缓冲区接口块的引用，如果设置了对应的选项。
9. 降低shared引用（仅对计算着色器）。
10. 降低向量引用。
11. 执行vec索引到swizzle的转换。

最后，如果着色器程序的链接状态为false，则返回false。这段代码主要用于整个着色器程序的最终链接过程，确保各个着色器阶段的变量和统一变量正确链接和分配位置。

### 清空变量位置

```
void
link_invalidate_variable_locations(exec_list *ir)
{
   foreach_in_list(ir_instruction, node, ir) {
      ir_variable *const var = node->as_variable();

      if (var == NULL)
         continue;

      /* 仅为缺乏显式位置的变量分配位置。
       * 所有内置变量、通用顶点着色器输入（通过 layout(location=...) 设置），
       * 以及通用片段着色器输出（也通过 layout(location=...) 设置）都有显式位置。
       */
      if (!var->data.explicit_location) {
         var->data.location = -1;
         var->data.location_frac = 0;
      }

      /* ir_variable::is_unmatched_generic_inout 在连接器连接一个阶段的输出
       * 到下一个阶段的输入时使用。
       */
      if (var->data.explicit_location &&
          var->data.location < VARYING_SLOT_VAR0) {
         var->data.is_unmatched_generic_inout = 0;
      } else {
         var->data.is_unmatched_generic_inout = 1;
      }
   }
}
```


### 找到所有匹配的输入输出

```
/**
 * 将输出的显式位置与输入匹配，并取消不匹配标志，以防止将它们优化掉。
 */
static void
match_explicit_outputs_to_inputs(gl_linked_shader *producer,
                                 gl_linked_shader *consumer)
{
   glsl_symbol_table parameters;
   ir_variable *explicit_locations[MAX_VARYINGS_INCL_PATCH][4] =
      { {NULL, NULL} };

   /* 在“producer”阶段查找所有着色器输出。 */
   foreach_in_list(ir_instruction, node, producer->ir) {
      ir_variable *const var = node->as_variable();

      if ((var == NULL) || (var->data.mode != ir_var_shader_out))
         continue;

      if (var->data.explicit_location &&
          var->data.location >= VARYING_SLOT_VAR0) {
          // 这个步骤是tcs,tes,gs中的一个
         const unsigned idx = var->data.location - VARYING_SLOT_VAR0;
         if (explicit_locations[idx][var->data.location_frac] == NULL)
            explicit_locations[idx][var->data.location_frac] = var;

         /* 总是匹配TCS输出。它们被所有控制点共享，并且可以用作共享内存。 */
         if (producer->Stage == MESA_SHADER_TESS_CTRL)
            var->data.is_unmatched_generic_inout = 0;
      }
   }

   /* 匹配输入与输出 */
   foreach_in_list(ir_instruction, node, consumer->ir) {
      ir_variable *const input = node->as_variable();

      if ((input == NULL) || (input->data.mode != ir_var_shader_in))
         continue;

      ir_variable *output = NULL;
      if (input->data.explicit_location
          && input->data.location >= VARYING_SLOT_VAR0) {
         output = explicit_locations[input->data.location - VARYING_SLOT_VAR0]
            [input->data.location_frac];

         if (output != NULL){
            input->data.is_unmatched_generic_inout = 0;
            output->data.is_unmatched_generic_inout = 0;
         }
      }
   }
}

```
## 为顶点着色器输入或片段着色器输出分配位置
 
```

/**
 * 为顶点着色器输入或片段着色器输出分配位置
 *
 * \param mem_ctx       用于链接的临时ralloc上下文
 * \param prog          需要分配位置的着色器程序
 * \param constants     程序的驱动特定常量值
 * \param target_index  用于接收位置分配的程序目标选择器。必须是 \c MESA_SHADER_VERTEX 或 \c MESA_SHADER_FRAGMENT。
 *
 * \return
 * 如果成功分配位置，则返回true。否则，将错误输出到着色器链接日志，并返回false。
 */
static bool
assign_attribute_or_color_locations(void *mem_ctx,
        gl_shader_program *prog,
        struct gl_constants *constants,
        unsigned target_index,
        bool do_assignment)
{
    /* 最大通用位置数。这对应于最大绘制缓冲区数或最大通用属性数。 */
    unsigned max_index = (target_index == MESA_SHADER_VERTEX) ?
        constants->Program[target_index].MaxAttribs :
        MAX2(constants->MaxDrawBuffers, constants->MaxDualSourceDrawBuffers);

    /* 将无效位置标记为已使用。 */
    unsigned used_locations = ~SAFE_MASK_FROM_INDEX(max_index);
    unsigned double_storage_locations = 0;

    gl_linked_shader *const sh = prog->_LinkedShaders[target_index];

    /* 运行共四遍。
     *
     * 1. 使所有顶点着色器输入的位置分配无效。
     *
     * 2. 为通过glBindVertexAttribLocation设置的用户定义位置的输入分配位置，
     *    以及通过glBindFragDataLocation设置的用户定义位置的输出分配位置。
     *
     * 3. 按所需的插槽数量降序对未分配位置的属性进行排序。由于应用程序分配的属性位置可能导致
     *    大属性无法获得足够的连续空间，因此需要排序。
     *
     * 4. 为未分配位置的任何输入分配位置。
     */

    const int generic_base = (target_index == MESA_SHADER_VERTEX)
        ? (int) VERT_ATTRIB_GENERIC0 : (int) FRAG_RESULT_DATA0;

    const enum ir_variable_mode direction =
        (target_index == MESA_SHADER_VERTEX)
        ? ir_var_shader_in : ir_var_shader_out;

    /* 用于需要分配位置的属性的临时存储。 */
    struct temp_attr {
        unsigned slots;
        ir_variable *var;

        /* 在qsort调用中使用。 */
        static int compare(const void *a, const void *b)
        {
            const temp_attr *const l = (const temp_attr *) a;
            const temp_attr *const r = (const temp_attr *) b;

            /* 反向排序，因为我们希望在下面的排序中得到降序。 */
            return r->slots - l->slots;
        }
    } to_assign[32];

    /* 用于已分配位置的属性的临时数组，用于检查（非ES）片段着色器输出的重叠插槽/分量。 */
    ir_variable *assigned[12 * 4]; /* （最大的FS输出数量） * （分量数量） */
    unsigned assigned_attr = 0;

    unsigned num_attr = 0;

    foreach_in_list(ir_instruction, node, sh->ir) {
        ir_variable *const var = node->as_variable();

        if ((var == NULL) || (var->data.mode != (unsigned) direction))
            continue;

        if (var->data.explicit_location) {
            var->data.is_unmatched_generic_inout = 0;
        } else if (target_index == MESA_SHADER_VERTEX) {
            // 未声明location的vs out
            unsigned binding;

            // 通过glBind绑定的位置
            if (prog->AttributeBindings->get(binding, var->name)) {
                assert(binding >= VERT_ATTRIB_GENERIC0);
                var->data.location = binding;
                var->data.is_unmatched_generic_inout = 0;
            }
        } else if (target_index == MESA_SHADER_FRAGMENT) {
            unsigned binding;
            unsigned index;
            const char *name = var->name;
            const glsl_type *type = var->type;

            while (type) {
                /* 检查变量名是否有绑定 */
                if (prog->FragDataBindings->get(binding, name)) {
                    assert(binding >= FRAG_RESULT_DATA0);
                    var->data.location = binding;
                    var->data.is_unmatched_generic_inout = 0;

                    if (prog->FragDataIndexBindings->get(index, name)) {
                        var->data.index = index;
                    }
                    break;
                }

                /* 如果没有，并且它是数组类型，则查找 name[0] */
                if (type->is_array()) {
                    name = ralloc_asprintf(mem_ctx, "%s[0]", name);
                    type = type->fields.array;
                    continue;
                }

                break;
            }
        }

        if (strcmp(var->name, "gl_LastFragData") == 0)
            continue;

        const unsigned slots = var->type->count_attribute_slots(target_index == MESA_SHADER_VERTEX);

        /* 如果变量不是内置的并且在着色器中静态分配了位置
         * （可能是通过布局限定符），请确保它不与其他分配的位置冲突。
         * 否则，将其添加到需要链接器分配位置的变量列表中。
         */
        if (var->data.location != -1) {
            if (var->data.location >= generic_base && var->data.index < 1) {
                /* Mask representing the contiguous slots that will be used by
                  * this attribute.
                  */
                 const unsigned attr = var->data.location - generic_base;
                 const unsigned use_mask = (1 << slots) - 1;
                 const char *const string = (target_index == MESA_SHADER_VERTEX)
                    ? "vertex shader input" : "fragment shader output";
                 
                 used_locations |= (use_mask << attr);
            }

            continue;
        }

        if (num_attr >= max_index) {
            linker_error(prog, "太多的 %s（最大 %u）",
                    target_index == MESA_SHADER_VERTEX ?
                    "顶点着色器输入" : "片段着色器输出",
                    max_index);
            return false;
        }
        to_assign[num_attr].slots = slots;
        to_assign[num_attr].var = var;
        num_attr++;
    }

    if (!do_assignment)
        return true;


    /* 如果所有属性都由应用程序分配位置（或具有固定位置的内建属性），则提前返回。这应该是常见情况。 */
    if (num_attr == 0)
        return true;

    qsort(to_assign, num_attr, sizeof(to_assign[0]), temp_attr::compare);

    if (target_index == MESA_SHADER_VERTEX) {
        /* VERT_ATTRIB_GENERIC0 是 VERT_ATTRIB_POS 的伪别名。
         * 它只能通过 glBindAttribLocation 显式分配。将其标记为保留，以防止
         * 在下面自动分配。
         */
        find_deref_visitor find("gl_Vertex");
        find.run(sh->ir);
        if (find.variable_found())
            used_locations |= (1 << 0);
    }

    for (unsigned i = 0; i < num_attr; i++) {
        /* 表示将被此属性使用的连续槽的掩码。*/
        const unsigned use_mask = (1 << to_assign[i].slots) - 1;

        int location = find_available_slots(used_locations, to_assign[i].slots);

        if (location < 0) {
            const char *const string = (target_index == MESA_SHADER_VERTEX)
                ? "顶点着色器输入" : "片段着色器输出";

            linker_error(prog,
                    "没有足够的连续位置可用于 %s `%s'\n",
                    string, to_assign[i].var->name);
            return false;
        }

        to_assign[i].var->data.location = generic_base + location;
        to_assign[i].var->data.is_unmatched_generic_inout = 0;
        used_locations |= (use_mask << location);

        if (to_assign[i].var->type->without_array()->is_dual_slot())
            double_storage_locations |= (use_mask << location);
    }

    /* 现在我们有了所有的位置，根据 GL 4.5 核心规范，第 11.1.1 节（顶点属性），
     * dvec3、dvec4、dmat2x3、dmat2x4、dmat3、dmat3x4、dmat4x3 和 dmat4 被视为
     * 消耗等效单精度类型两倍的属性。
     */
    if (target_index == MESA_SHADER_VERTEX) {
        unsigned total_attribs_size =
            util_bitcount(used_locations & SAFE_MASK_FROM_INDEX(max_index)) +
            util_bitcount(double_storage_locations);
        if (total_attribs_size > max_index) {
            linker_error(prog,
                    "尝试使用 %d 个顶点属性插槽，但只有 %d 个可用 ",
                    total_attribs_size, max_index);
            return false;
        }
    }

    return true;
}

```

* 函数末尾计算了非显式指定位置变量的位置

## link_varyings

```

bool
link_varyings(struct gl_shader_program *prog, unsigned first, unsigned last,
              struct gl_context *ctx, void *mem_ctx)
{
   bool has_xfb_qualifiers = false;
   unsigned num_tfeedback_decls = 0;
   char **varying_names = NULL;
   tfeedback_decl *tfeedback_decls = NULL;

   // 处理变换反馈
   ....


   if (last <= MESA_SHADER_FRAGMENT) {
      /* 除非是单阶段程序，否则从第一个/最后一个阶段中移除未使用的变量 */
      remove_unused_shader_inputs_and_outputs(prog->SeparateShader,
                                              prog->_LinkedShaders[first],
                                              ir_var_shader_in);
      remove_unused_shader_inputs_and_outputs(prog->SeparateShader,
                                              prog->_LinkedShaders[last],
                                              ir_var_shader_out);

      /* 如果程序仅由单个阶段组成 */
      if (first == last) {
         gl_linked_shader *const sh = prog->_LinkedShaders[last];

         do_dead_builtin_varyings(ctx, NULL, sh, 0, NULL);
         do_dead_builtin_varyings(ctx, sh, NULL, num_tfeedback_decls,
                                  tfeedback_decls);

         if (prog->SeparateShader) {
            const uint64_t reserved_slots =
               reserved_varying_slot(sh, ir_var_shader_in);

            /* 为 SSO 分配输入位置，输出位置已经分配。 */
            if (!assign_varying_locations(ctx, mem_ctx, prog,
                                          NULL /* producer */,
                                          sh /* consumer */,
                                          0 /* num_tfeedback_decls */,
                                          NULL /* tfeedback_decls */,
                                          reserved_slots))
               return false;
         }
      } else {
         /* 反向顺序链接阶段（从片段到顶点）确保在较早阶段写入的着色器间输出
          * 在后续阶段中（传递地）未使用时被消除。
          */
         int next = last;
         for (int i = next - 1; i >= 0; i--) {
            if (prog->_LinkedShaders[i] == NULL && i != 0)
               continue;

            gl_linked_shader *const sh_i = prog->_LinkedShaders[i];
            gl_linked_shader *const sh_next = prog->_LinkedShaders[next];

            const uint64_t reserved_out_slots =
               reserved_varying_slot(sh_i, ir_var_shader_out);
            const uint64_t reserved_in_slots =
               reserved_varying_slot(sh_next, ir_var_shader_in);

            do_dead_builtin_varyings(ctx, sh_i, sh_next,
                      next == MESA_SHADER_FRAGMENT ? num_tfeedback_decls : 0,
                      tfeedback_decls);

            if (!assign_varying_locations(ctx, mem_ctx, prog, sh_i, sh_next,
                      next == MESA_SHADER_FRAGMENT ? num_tfeedback_decls : 0,
                      tfeedback_decls,
                      reserved_out_slots | reserved_in_slots))
               return false;

            /* 必须在所有未使用的变量消除之后执行此操作。*/
            if (sh_i != NULL) {
               unsigned slots_used = util_bitcount64(reserved_out_slots);
               if (!check_against_output_limit(ctx, prog, sh_i, slots_used)) {
                  return false;
               }
            }

            unsigned slots_used = util_bitcount64(reserved_in_slots);
            if (!check_against_input_limit(ctx, prog, sh_next, slots_used))
               return false;

            next = i;
         }
      }
   }

   if (!store_tfeedback_info(ctx, prog, num_tfeedback_decls, tfeedback_decls,
                             has_xfb_qualifiers))
      return false;

   return true;
}
```
## link_and_validate_uniforms


```c
static void
link_and_validate_uniforms(struct gl_context *ctx,
                           struct gl_shader_program *prog)
{
   link_assign_uniform_locations(prog, ctx);
      

   link_assign_atomic_counter_resources(ctx, prog);
   link_calculate_subroutine_compat(prog);
   link_check_atomic_counter_resources(ctx, prog);
}

void
link_assign_uniform_locations(struct gl_shader_program *prog,
                              struct gl_context *ctx)
{
   ralloc_free(prog->data->UniformStorage);
   prog->data->UniformStorage = NULL;
   prog->data->NumUniformStorage = 0;

   if (prog->UniformHash != NULL) {
      prog->UniformHash->clear();
   } else {
      prog->UniformHash = new string_to_uint_map;
   }

   /* First pass: Count the uniform resources used by the user-defined
    * uniforms.  While this happens, each active uniform will have an index
    * assigned to it.
    *
    * Note: this is *NOT* the index that is returned to the application by
    * glGetUniformLocation.
    */
   struct string_to_uint_map *hiddenUniforms = new string_to_uint_map;
   count_uniform_size uniform_size(prog->UniformHash, hiddenUniforms,
                                   ctx->Const.UseSTD430AsDefaultPacking);
   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[i];

      if (sh == NULL)
         continue;

      link_update_uniform_buffer_variables(sh, i);

      /* Reset various per-shader target counts.
       */
      uniform_size.start_shader();

      foreach_in_list(ir_instruction, node, sh->ir) {
         ir_variable *const var = node->as_variable();

         if ((var == NULL) || (var->data.mode != ir_var_uniform &&
                               var->data.mode != ir_var_shader_storage))
            continue;

         uniform_size.process(var);
             this->current_var = var;
             this->is_buffer_block = var->is_in_buffer_block();
             this->is_shader_storage = var->is_in_shader_storage_block();
             if (var->is_interface_instance())
                program_resource_visitor::process(var->get_interface_type(),
                                                  var->get_interface_type()->name,
                                                  use_std430_as_default);
             else
                program_resource_visitor::process(var, use_std430_as_default);
                    const glsl_type *t =
                       var->data.from_named_ifc_block ? var->get_interface_type() : var->type;
                    process(var, t, use_std430_as_default);

                        if(){}
                        else if() {}
                        else {
                            this->set_record_array_count(record_array_count);
                            this->visit_field(t, var->name, row_major, NULL, packing, false);
                               [ count_uniform_size::visit_field(...)]
                               ....
                               this->num_active_uniforms++;


                        }

             }

      // ok现在uniform_size已经包含了unfrom数量等相关信息
      sh->Program->info.num_textures = uniform_size.num_shader_samplers;
      sh->Program->info.num_images = uniform_size.num_shader_images;
      sh->num_uniform_components = uniform_size.num_shader_uniform_components;
      sh->num_combined_uniform_components = sh->num_uniform_components;

      for (unsigned i = 0; i < sh->Program->info.num_ubos; i++) {
         sh->num_combined_uniform_components +=
            sh->Program->sh.UniformBlocks[i]->UniformBufferSize / 4;
      }
   }

   prog->data->NumUniformStorage = uniform_size.num_active_uniforms;
   prog->data->NumHiddenUniforms = uniform_size.num_hidden_uniforms;

   /* assign hidden uniforms a slot id */
   hiddenUniforms->iterate(assign_hidden_uniform_slot_id, &uniform_size);
   delete hiddenUniforms;

   link_assign_uniform_storage(ctx, prog, uniform_size.num_values);
        union gl_constant_value *data;
        if (prog->data->UniformStorage == NULL) {

        } else {
           data = prog->data->UniformDataSlots;
        }

   link_setup_uniform_remap_tables(ctx, prog);

/**
 * 遍历IR并更新ir_variables中对于统一块的引用，使其指向链接的着色器列表
 * （之前，它们指向预链接的着色器之一中的统一块列表）。
 */
static void
link_update_uniform_buffer_variables(struct gl_linked_shader *shader,
                                     unsigned stage)
{
   // 创建IR数组引用计数访问者
   ir_array_refcount_visitor v;
   // 运行引用计数访问者以收集统一块引用信息
   v.run(shader->ir);

   // 遍历IR指令列表
   foreach_in_list(ir_instruction, node, shader->ir) {
      // 获取当前节点对应的变量
      ir_variable *const var = node->as_variable();

      // 如果变量为空或不是在缓冲块中，则继续下一个循环
      if (var == NULL || !var->is_in_buffer_block())
         continue;

      // 确保变量的数据模式为统一变量或着色器存储
      assert(var->data.mode == ir_var_uniform ||
             var->data.mode == ir_var_shader_storage);

      // 获取统一块的数量和对应的统一块数组
      unsigned num_blocks = var->data.mode == ir_var_uniform ?
         shader->Program->info.num_ubos : shader->Program->info.num_ssbos;
      struct gl_uniform_block **blks = var->data.mode == ir_var_uniform ?
         shader->Program->sh.UniformBlocks :
         shader->Program->sh.ShaderStorageBlocks;

      // 如果变量是接口实例
      if (var->is_interface_instance()) {
         // 获取变量的引用计数信息
         const ir_array_refcount_entry *const entry = v.get_variable_entry(var);

         // 如果变量被引用
         if (entry->is_referenced) {
            /* 由于这是一个接口实例，实例类型将与去除数组的变量类型相同。
             * 如果变量类型是数组，那么块的名称将带有[0]到[n-1]的后缀。
             * 与非接口实例不同，这里不会有结构类型，因此我们唯一需要关注的
             * 名称分隔符是[。
             */
            assert(var->type->without_array() == var->get_interface_type());
            const char sentinel = var->type->is_array() ? '[' : '\0';

            const ptrdiff_t len = strlen(var->get_interface_type()->name);
            for (unsigned i = 0; i < num_blocks; i++) {
               const char *const begin = blks[i]->Name;
               const char *const end = strchr(begin, sentinel);

               if (end == NULL)
                  continue;

               if (len != (end - begin))
                  continue;

               /* 即使找到匹配项，也不要在这里“break”。
                * 这可能是实例的数组，数组的所有元素都需要标记为引用。
                */
               if (strncmp(begin, var->get_interface_type()->name, len) == 0 &&
                   (!var->type->is_array() ||
                    entry->is_linearized_index_referenced(blks[i]->linearized_array_index))) {
                  blks[i]->stageref |= 1U << stage;
               }
            }
         }

         // 重置变量的数据位置为0，并继续下一个循环
         var->data.location = 0;
         continue;
      }

      // 初始化标志变量
      bool found = false;
      char sentinel = '\0';

      // 如果变量类型是记录
      if (var->type->is_record()) {
         sentinel = '.';
      }
      // 如果变量类型是数组且数组的字段是数组或者去除数组后的类型是记录
      else if (var->type->is_array() && (var->type->fields.array->is_array()
                 || var->type->without_array()->is_record())) {
         sentinel = '[';
      }

      // 获取变量名的长度
      const unsigned l = strlen(var->name);
      // 遍历统一块数组和对应的统一块中的统一变量
      for (unsigned i = 0; i < num_blocks; i++) {
         for (unsigned j = 0; j < blks[i]->NumUniforms; j++) {
            // 如果存在分隔符
            if (sentinel) {
               const char *begin = blks[i]->Uniforms[j].Name;
               const char *end = strchr(begin, sentinel);

               if (end == NULL)
                  continue;

               if ((ptrdiff_t) l != (end - begin))
                  continue;

               found = strncmp(var->name, begin, l) == 0;
            }
            // 如果不存在分隔符
            else {
               found = strcmp(var->name, blks[i]->Uniforms[j].Name) == 0;
            }

            // 如果找到匹配项
            if (found) {
               var->data.location = j;

               // 如果变量被引用，则标记统一块为在指定阶段被引用
               if (variable_is_referenced(v, var))
                  blks[i]->stageref |= 1U << stage;

               break;
            }
         }

         // 如果找到匹配项，则退出内层循环
         if (found)
            break;
      }
      // 确保找到匹配项
      assert(found);
   }
}

```

* t



```

static void
link_assign_uniform_storage(struct gl_context *ctx,
                            struct gl_shader_program *prog,
                            const unsigned num_data_slots)
{
   /* 如果没有uniforms的情况极少，就退出。*/
   if (prog->data->NumUniformStorage == 0)
      return;

   unsigned int boolean_true = ctx->Const.UniformBooleanTrue;

   union gl_constant_value *data;
   if (prog->data->UniformStorage == NULL) {
      // 如果UniformStorage为NULL，则分配空间
      prog->data->UniformStorage = rzalloc_array(prog->data,
                                                 struct gl_uniform_storage,
                                                 prog->data->NumUniformStorage);
      data = rzalloc_array(prog->data->UniformStorage,
                           union gl_constant_value, num_data_slots);
      // 分配UniformDataDefaults数组的空间
      prog->data->UniformDataDefaults =
         rzalloc_array(prog->data->UniformStorage,
                       union gl_constant_value, num_data_slots);
   } else {
      // 否则，使用已有的UniformDataSlots
      data = prog->data->UniformDataSlots;
   }

#ifndef NDEBUG
   // 用于调试的辅助断言
   union gl_constant_value *data_end = &data[num_data_slots];
#endif

   // 初始化parcel，设置相关的UniformStorage和数据数组
   parcel_out_uniform_storage parcel(prog, prog->UniformHash,
                                     prog->data->UniformStorage, data,
                                     ctx->Const.UseSTD430AsDefaultPacking);

   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *shader = prog->_LinkedShaders[i];

      if (!shader)
         continue;

      parcel.start_shader((gl_shader_stage)i);

      // 遍历IR指令列表
      foreach_in_list(ir_instruction, node, shader->ir) {
         ir_variable *const var = node->as_variable();

         if ((var == NULL) || (var->data.mode != ir_var_uniform &&
                               var->data.mode != ir_var_shader_storage))
            continue;

         // 设置并处理uniform变量
         parcel.set_and_process(var);
      }

      // 设置shader中的一些属性，如samplers_used，shadow_samplers等
      shader->Program->SamplersUsed = parcel.shader_samplers_used;
      shader->shadow_samplers = parcel.shader_shadow_samplers;

      if (parcel.num_bindless_samplers > 0) {
         // 处理bindless samplers
         shader->Program->sh.NumBindlessSamplers = parcel.num_bindless_samplers;
         shader->Program->sh.BindlessSamplers =
            rzalloc_array(shader->Program, gl_bindless_sampler,
                          parcel.num_bindless_samplers);
         for (unsigned j = 0; j < parcel.num_bindless_samplers; j++) {
            shader->Program->sh.BindlessSamplers[j].target =
               parcel.bindless_targets[j];
         }
      }

      if (parcel.num_bindless_images > 0) {
         // 处理bindless images
         shader->Program->sh.NumBindlessImages = parcel.num_bindless_images;
         shader->Program->sh.BindlessImages =
            rzalloc_array(shader->Program, gl_bindless_image,
                          parcel.num_bindless_images);
         for (unsigned j = 0; j < parcel.num_bindless_images; j++) {
            shader->Program->sh.BindlessImages[j].access =
               parcel.bindless_access[j];
         }
      }

      STATIC_ASSERT(ARRAY_SIZE(shader->Program->sh.SamplerTargets) ==
                    ARRAY_SIZE(parcel.targets));
      // 复制parcel中的targets到shader中
      for (unsigned j = 0; j < ARRAY_SIZE(parcel.targets); j++)
         shader->Program->sh.SamplerTargets[j] = parcel.targets[j];
   }

#ifndef NDEBUG
   // 用于调试的辅助断言
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
      assert(prog->data->UniformStorage[i].storage != NULL ||
             prog->data->UniformStorage[i].builtin ||
             prog->data->UniformStorage[i].is_shader_storage ||
             prog->data->UniformStorage[i].block_index != -1);
   }

   assert(parcel.values == data_end);
#endif

   // 设置uniform重映射表
   link_setup_uniform_remap_tables(ctx, prog);

   /* 设置shader cache字段 */
   prog->data->NumUniformDataSlots = num_data_slots;
   prog->data->UniformDataSlots = data;

   // 设置uniform的初始值
   link_set_uniform_initializers(prog, boolean_true);

}


```
### 分配uniform storage/ uniform remap表

#### 表结构

```
/**
 * A GLSL program object.
 * Basically a linked collection of vertex and fragment shaders.
 */
struct gl_shader_program
{
    ...


   /**
    * 从 \c glUniformLocation 返回的GL uniform位置到 UniformStorage 条目的映射。
    * 数组将在 UniformRemapTable 中有多个连续的槽，都指向相同的 UniformStorage 条目。
    */
   unsigned NumUniformRemapTable;
   struct gl_uniform_storage **UniformRemapTable;
   unsigned NumUniformRemapTable;

   struct gl_uniform_storage **UniformRemapTable;
   ...
}
```

```

link_setup_uniform_remap_tables(struct gl_context *ctx,
                                struct gl_shader_program *prog)

   unsigned total_entries = prog->NumExplicitUniformLocations;
   unsigned empty_locs = prog->NumUniformRemapTable - total_entries;


   /* Reserve all the explicit locations of the active uniforms. */
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) 
        unsigned element_loc =
               prog->data->UniformStorage[i].remap_location + j;

      for (unsigned j = 0; j < entries; j++)
         prog->UniformRemapTable[chosen_location + j] =
            &prog->data->UniformStorage[i];

      prog->data->UniformStorage[i].remap_location = chosen_location;
   
   // 处理subroutine 
   TODO

```



####  remap更新与glUniform


```

_mesa_Uniform1f(GLint location, GLfloat v0)
   _mesa_uniform(location, 1, &v0, ctx, ctx->_Shader->ActiveProgram, GLSL_TYPE_FLOAT, 1);
       int size_mul = glsl_base_type_is_64bit(basicType) ? 2 : 1;
       struct gl_uniform_storage *uni;
       uni = validate_uniform(location, count, values, &offset, ctx, shProg,
                                 basicType, src_components);
            struct gl_uniform_storage *const uni =
               validate_uniform_parameters(location, count, array_index=offset,
                                           ctx, shProg, "glUniform");
                    struct gl_uniform_storage *const uni = shProg->UniformRemapTable[location];
                    *array_index = location - uni->remap_location;
                    return uni;

       const unsigned components = uni->type->vector_elements;


       /* Store the data in the "actual type" backing storage for the uniform.
        */
       gl_constant_value *storage;
       if (ctx->Const.PackedDriverUniformStorage &&
           (uni->is_bindless || !uni->type->contains_opaque())) {
         ... 
       } else {
          storage = &uni->storage[size_mul * components * offset];
          copy_uniforms_to_storage(storage, uni, ctx, count, values, size_mul,
                                   offset, components, basicType);
                for (unsigned i = 0; i < elems; i++) {
                   uni[i].i = values[i].i;
                }

          _mesa_propagate_uniforms_to_driver_storage(uni, offset, count);
       }



extern "C" void
_mesa_propagate_uniforms_to_driver_storage(struct gl_uniform_storage *uni,
					   unsigned array_index,
					   unsigned count)
{
   unsigned i;

   const unsigned components = uni->type->vector_elements;
   const unsigned vectors = uni->type->matrix_columns;
   const int dmul = uni->type->is_64bit() ? 2 : 1;

   /* Store the data in the driver's requested type in the driver's storage
    * areas.
    */
   unsigned src_vector_byte_stride = components * 4 * dmul;

   for (i = 0; i < uni->num_driver_storage; i++) {
      struct gl_uniform_driver_storage *const store = &uni->driver_storage[i];
      uint8_t *dst = (uint8_t *) store->data;
      const unsigned extra_stride =
	 store->element_stride - (vectors * store->vector_stride);
      const uint8_t *src =
	 (uint8_t *) (&uni->storage[array_index * (dmul * components * vectors)].i);

#if 0
      printf("%s: %p[%d] components=%u vectors=%u count=%u vector_stride=%u "
	     "extra_stride=%u\n",
	     __func__, dst, array_index, components,
	     vectors, count, store->vector_stride, extra_stride);
#endif

      dst += array_index * store->element_stride;

      switch (store->format) {
      case uniform_native: {
	 unsigned j;
	 unsigned v;

	 if (src_vector_byte_stride == store->vector_stride) {
	    if (extra_stride) {
	       for (j = 0; j < count; j++) {
	          memcpy(dst, src, src_vector_byte_stride * vectors);
	          src += src_vector_byte_stride * vectors;
	          dst += store->vector_stride * vectors;

	          dst += extra_stride;
	       }
	    } else {
	       /* Unigine Heaven benchmark gets here */
	       memcpy(dst, src, src_vector_byte_stride * vectors * count);
	       src += src_vector_byte_stride * vectors * count;
	       dst += store->vector_stride * vectors * count;
	    }
	 } else {
	    for (j = 0; j < count; j++) {
	       for (v = 0; v < vectors; v++) {
	          memcpy(dst, src, src_vector_byte_stride);
	          src += src_vector_byte_stride;
	          dst += store->vector_stride;
	       }

	       dst += extra_stride;
	    }
	 }
	 break;
      }

      case uniform_int_float: {
	 const int *isrc = (const int *) src;
	 unsigned j;
	 unsigned v;
	 unsigned c;

	 for (j = 0; j < count; j++) {
	    for (v = 0; v < vectors; v++) {
	       for (c = 0; c < components; c++) {
		  ((float *) dst)[c] = (float) *isrc;
		  isrc++;
	       }

	       dst += store->vector_stride;
	    }

	    dst += extra_stride;
	 }
	 break;
      }

      default:
	 assert(!"Should not get here.");
	 break;
      }
   }
}
```



















## ending
```
/**
 * \file ir_hv_accept.cpp
 * IR指令的所有层次访问器accept方法的实现。
 */

/**
 * 使用层次访问器处理节点列表。
 *
 * 如果`statement_list`为真（默认情况下），这是一个语句列表，因此
 * `v->base_ir`将在迭代每个语句之前设置为指向该语句，并在迭代完成后还原。
 * 如果`statement_list`为假，则这是一个出现在语句内部的列表（例如，参数列表），
 * 因此`v->base_ir`将保持不变。
 *
 * \warning
 * 如果正在处理的节点从列表中移除，此函数将正常运行。但是，如果在处理节点之后向列表中添加节点，
 * 则可能不会处理其中的一些添加的节点。
 */
ir_visitor_status
visit_list_elements(ir_hierarchical_visitor *v, exec_list *l,
                    bool statement_list)
{
   ir_instruction *prev_base_ir = v->base_ir;

   foreach_in_list_safe(ir_instruction, ir, l) {
      if (statement_list)
         v->base_ir = ir;
      ir_visitor_status s = ir->accept(v);

      if (s != visit_continue)
	 return s;
   }
   if (statement_list)
      v->base_ir = prev_base_ir;

   return visit_continue;
}

```


## uniform 初始化

```
void
link_set_uniform_initializers(struct gl_shader_program *prog,
                              unsigned int boolean_true)
{
   void *mem_ctx = NULL;

   for (unsigned int i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *shader = prog->_LinkedShaders[i];

      if (shader == NULL)
         continue;

      foreach_in_list(ir_instruction, node, shader->ir) {
         ir_variable *const var = node->as_variable();

         if (!var || (var->data.mode != ir_var_uniform &&
             var->data.mode != ir_var_shader_storage))
            continue;

         if (!mem_ctx)
            mem_ctx = ralloc_context(NULL);


         if (var->data.explicit_binding) {

            // 处理显式绑定
            const glsl_type *const type = var->type;

            if (type->without_array()->is_sampler() ||
                type->without_array()->is_image()) {
                // 处理sample，image模糊类型
               int binding = var->data.binding;
               linker::set_opaque_binding(mem_ctx, prog, var, var->type,
                                          var->name, &binding);
                       [jump set_opaque_binding]

            } else if (var->is_in_buffer_block()) {
               /* This case is handled by link_uniform_blocks (at
                * process_block_array_leaf)
                */
            } else if (type->contains_atomic()) {
               /* we don't actually need to do anything. */
            } else {
               assert(!"Explicit binding not on a sampler, UBO or atomic.");
            }
         } else if (var->constant_initializer) {
            linker::set_uniform_initializer(mem_ctx, prog, var->name,
                                            var->type, var->constant_initializer,
                                            boolean_true);
         }
      }
   }

   memcpy(prog->data->UniformDataDefaults, prog->data->UniformDataSlots,
          sizeof(union gl_constant_value) * prog->data->NumUniformDataSlots);
   ralloc_free(mem_ctx);
}



set_opaque_binding(void *mem_ctx, gl_shader_program *prog,
                   const ir_variable *var, const glsl_type *type,
                   const char *name, int *binding)
{

      struct gl_uniform_storage *const storage = get_storage(prog, name);
      if (!storage)
         return;

      const unsigned elements = MAX2(storage->array_elements, 1);

      /* Section 4.4.6 (Opaque-Uniform Layout Qualifiers) of the GLSL 4.50 spec
       * says:
       *
       *     "If the binding identifier is used with an array, the first element
       *     of the array takes the specified unit and each subsequent element
       *     takes the next consecutive unit."
       */
      for (unsigned int i = 0; i < elements; i++) {
         storage->storage[i].i = (*binding)++;
      }

      for (int sh = 0; sh < MESA_SHADER_STAGES; sh++) {
            gl_linked_shader *shader = prog->_LinkedShaders[sh];
   
            if (!shader)
               continue;
            if (!storage->opaque[sh].active)
               continue;
   
            if (storage->type->is_sampler()) {
               for (unsigned i = 0; i < elements; i++) {
                  const unsigned index = storage->opaque[sh].index + i;
   
                  if (var->data.bindless) {
                     shader->Program->sh.BindlessSamplers[index].unit =
                        storage->storage[i].i;
                     shader->Program->sh.BindlessSamplers[index].bound = true;
                     shader->Program->sh.HasBoundBindlessSampler = true;
                  } else {
                     shader->Program->SamplerUnits[index] =
                        storage->storage[i].i;
                  }
               } } else if (storage->type->is_image()) {
               for (unsigned i = 0; i < elements; i++) {
                  const unsigned index = storage->opaque[sh].index + i;
   
                  if (var->data.bindless) {
                     shader->Program->sh.BindlessImages[index].unit =
                        storage->storage[i].i;
                     shader->Program->sh.BindlessImages[index].bound = true;
                     shader->Program->sh.HasBoundBindlessImage = true;
                  } else {
                     shader->Program->sh.ImageUnits[index] =
                        storage->storage[i].i;
                  }
               }
            }
         }

}
```


## use program
```
_mesa_UseProgram_no_error(GLuint program)
   use_program(program, true);
         shProg = _mesa_lookup_shader_program(ctx, program);
         /* Attach shader state to the binding point */
         _mesa_reference_pipeline_object(ctx, &ctx->_Shader, &ctx->Shader);
        _mesa_use_shader_program(ctx, shProg);
           for (int i = 0; i < MESA_SHADER_STAGES; i++) {
              struct gl_program *new_prog = NULL;
              if (shProg && shProg->_LinkedShaders[i])
                 new_prog = shProg->_LinkedShaders[i]->Program;
              _mesa_use_program(ctx, stage=i, shProg, pro=new_prog, shTarget=&ctx->Shader);
                target = &shTarget->CurrentProgram[stage];
                /* Program is current, flush it */
                if (shTarget == ctx->_Shader) {
                    FLUSH_VERTICES(ctx, _NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS);
                }

                _mesa_reference_shader_program(ctx,
                                     &shTarget->ReferencedPrograms[stage],
                                     shProg);
                _mesa_reference_program(ctx, target, prog);

           }
           _mesa_active_program(ctx, shProg, "glUseProgram");
                _mesa_reference_shader_program(ctx, &ctx->Shader.ActiveProgram, shProg);

```


### state_tracker 的链接处理

```
/**
 * Link a shader.
 * Called via ctx->Driver.LinkShader()
 * This actually involves converting GLSL IR into an intermediate TGSI-like IR
 * with code lowering and other optimizations.
 */
GLboolean
st_link_shader(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct pipe_screen *pscreen = ctx->st->pipe->screen;

   enum pipe_shader_ir preferred_ir = (enum pipe_shader_ir)
      pscreen->get_shader_param(pscreen, PIPE_SHADER_VERTEX,
                                PIPE_SHADER_CAP_PREFERRED_IR);

   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
        // 处理lowewr_instructions
   }

   build_program_resource_list(ctx, prog);

   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *shader = prog->_LinkedShaders[i];
      if (shader == NULL)
         continue;

      // GLSL IR -> gl_program glsl_to_tgsi  
      struct gl_program *linked_prog =
         get_mesa_program_tgsi(ctx, prog, shader);
         [jump get_mesa_program_tgsi]
      st_set_prog_affected_state_flags(linked_prog);
           switch (prog->info.stage) {
           case MESA_SHADER_VERTEX:
              states = &((struct st_vertex_program*)prog)->affected_states;
        
              *states = ST_NEW_VS_STATE |
                        ST_NEW_RASTERIZER |
                        ST_NEW_VERTEX_ARRAYS;
              set_affected_state_flags(states, prog,
                                       ST_NEW_VS_CONSTANTS,
                                       ST_NEW_VS_SAMPLER_VIEWS,
                                       ST_NEW_VS_SAMPLERS,
                                       ST_NEW_VS_IMAGES,
                                       ST_NEW_VS_UBOS,
                                       ST_NEW_VS_SSBOS,
                                       ST_NEW_VS_ATOMICS);
              break;
          ...



      if (linked_prog) {
         if (!ctx->Driver.ProgramStringNotify(ctx,
                                              _mesa_shader_stage_to_program(i),
                                              linked_prog)) {
                        [ st_program_string_notify]
                        if (target == GL_FRAGMENT_PROGRAM_ARB) {
                           struct st_fragment_program *stfp = (struct st_fragment_program *) prog;
                     
                           st_release_fp_variants(st, stfp);
                           if (!st_translate_fragment_program(st, stfp))
                                    [jump st_translate_fragment_program]
                              return false;
                     
                           if (st->fp == stfp)
                     	 st->dirty |= stfp->affected_states;
                        } 
                        else if (target == GL_VERTEX_PROGRAM_ARB) {
                           struct st_vertex_program *stvp = (struct st_vertex_program *) prog;
                     
                           st_release_vp_variants(st, stvp);
                           if (!st_translate_vertex_program(st, stvp))
                                    [jump st_translate_vertex_program]
                              return false;
                     
                           if (st->vp == stvp)
                     	 st->dirty |= ST_NEW_VERTEX_PROGRAM(st, stvp);
                        } else if ...
            _mesa_reference_program(ctx, &shader->Program, NULL);
            return GL_FALSE;
         }
      } }
   return GL_TRUE;
}


```


###  hir -> Mesa gl_program  by glsl_to_tgsi_visitor 

```
class add_uniform_to_shader : public program_resource_visitor {


```

```

static struct gl_program *
get_mesa_program_tgsi(struct gl_context *ctx,
                      struct gl_shader_program *shader_program,
                      struct gl_linked_shader *shader)
   glsl_to_tgsi_visitor* v;
   struct gl_program *prog;

   // 设置具体shader程序引用
   prog = shader->Program;

   prog->Parameters = _mesa_new_parameter_list();
   v = new glsl_to_tgsi_visitor();
   v->ctx = ctx;

   // 设置prog应用
   v->prog = prog;
   v->shader_program = shader_program;
   v->shader = shader;
   v->options = options;
   v->native_integers = ctx->Const.NativeIntegers;


   _mesa_generate_parameters_list_for_uniforms(ctx, shader_program, shader,
                                               prog->Parameters);
        add_uniform_to_shader add(ctx, shader_program, params);
        foreach_in_list(ir_instruction, node, sh->ir) {
           ir_variable *var = node->as_variable();
           // for uniform 
           add.process(var);
               this->program_resource_visitor::process(var,
                                  ctx->Const.UseSTD430AsDefaultPacking);
                     [jump add_uniform_to_shader::visit_field]
               var->data.param_index = this->idx;

        }

   /* Emit intermediate IR for main(). */
   visit_exec_list(shader->ir, v);
        foreach_in_list_safe(ir_instruction, node, list) {
           node->accept(visitor);
                (glsl_to_tgsi_visitor*)v->visit(...)
        }



   do_set_program_inouts(shader->ir, prog, shader->Stage);
        [jump do_set_program_inouts]

```

### Mesa gl_program 输出输出标识属性信息生成

```

void
do_set_program_inouts(exec_list *instructions, struct gl_program *prog,
                      gl_shader_stage shader_stage)
{
   ir_set_program_inouts_visitor v(prog, shader_stage);

 // 对以下信息进行写入
   prog->info.inputs_read = 0;
   prog->info.outputs_written = 0;
   prog->SecondaryOutputsWritten = 0;
   prog->info.outputs_read = 0;
   prog->info.patch_inputs_read = 0;
   prog->info.patch_outputs_written = 0;
   prog->info.system_values_read = 0;
   if (shader_stage == MESA_SHADER_FRAGMENT) {
      prog->info.fs.uses_sample_qualifier = false;
      prog->info.fs.uses_discard = false;
   }
   visit_list_elements(&v, instructions);
        
        foreach_in_list_safe(ir_instruction, ir, l) 
            ir_visitor_status s = ir->accept(v);
                [jump ir_set_program_inouts_visitor->visit(...)]


/* Default handler: Mark all the locations in the variable as used. */
ir_visitor_status
ir_set_program_inouts_visitor::visit(ir_dereference_variable *ir)
{
   if (!is_shader_inout(ir->var))
      return visit_continue;

   mark_whole_variable(ir->var);
        mark(this->prog, var, 0, type->count_attribute_slots(is_vertex_input),
            this->shader_stage);
        [jump mark]

   return visit_continue;
}



void mark(struct gl_program *prog, ir_variable *var, int offset=0, int len, gl_shader_stage stage)
{
   /* 从GLSL 1.20开始，varyings只能是floats、浮点向量或矩阵，或者它们的数组。
    * 对于使用inputs_read/outputs_written的Mesa程序，除了矩阵使用每列一个插槽外，其他都使用一个插槽。
    * 假定执行更智能的打包的程序将使用inputs_read/outputs_written之外的其他东西。
    */

   for (int i = 0; i < len; i++) {
      assert(var->data.location != -1);

      int idx = var->data.location + offset + i;
      bool is_patch_generic = var->data.patch &&
                              idx != VARYING_SLOT_TESS_LEVEL_INNER &&
                              idx != VARYING_SLOT_TESS_LEVEL_OUTER &&
                              idx != VARYING_SLOT_BOUNDING_BOX0 &&
                              idx != VARYING_SLOT_BOUNDING_BOX1;
      GLbitfield64 bitfield;

      if (is_patch_generic) {
         assert(idx >= VARYING_SLOT_PATCH0 && idx < VARYING_SLOT_TESS_MAX);
         bitfield = BITFIELD64_BIT(idx - VARYING_SLOT_PATCH0);
      }
      else {
         assert(idx < VARYING_SLOT_MAX);
         bitfield = BITFIELD64_BIT(idx);
      }

      if (var->data.mode == ir_var_shader_in) {
         if (is_patch_generic)
            prog->info.patch_inputs_read |= bitfield;
         else
            prog->info.inputs_read |= bitfield;

         /* double inputs read is only for vertex inputs */
         if (stage == MESA_SHADER_VERTEX &&
             var->type->without_array()->is_dual_slot())
            prog->DualSlotInputs |= bitfield;

         if (stage == MESA_SHADER_FRAGMENT) {
            prog->info.fs.uses_sample_qualifier |= var->data.sample;
         }
      } else if (var->data.mode == ir_var_system_value) {
         prog->info.system_values_read |= bitfield;
      } else {
         assert(var->data.mode == ir_var_shader_out);
         if (is_patch_generic) {
            prog->info.patch_outputs_written |= bitfield;
         } else if (!var->data.read_only) {
            prog->info.outputs_written |= bitfield;
            if (var->data.index > 0)
               prog->SecondaryOutputsWritten |= bitfield;
         }

         if (var->data.fb_fetch_output)
            prog->info.outputs_read |= bitfield;
      }
   }
}


```
###

```
add_uniform_to_shader::visit_field(const glsl_type *type, const char *name,
                                   bool /* row_major */,
                                   const glsl_type * /* record_type */,
                                   const enum glsl_interface_packing,
                                   bool /* last_field */)
  /* opaque types don't use storage in the param list unless they are
    * bindless samplers or images.
    */
   if (type->contains_opaque() && !var->data.bindless)
      return;

   int index = params->NumParameters;

   if (ctx->Const.PackedDriverUniformStorage) {

   } else {
      for (unsigned i = 0; i < num_params; i++) {
         _mesa_add_parameter(params, PROGRAM_UNIFORM, name, 4,
                             type->gl_type, NULL, NULL, true);
                TODO
      }
   }

   /* The first part of the uniform that's processed determines the base
    * location of the whole uniform (for structures).
    */
   if (this->idx < 0)
      this->idx = index;


```

###  MESA gl_program -> TGSI

```c
/**
 * 翻译顶点程序。
 */
bool
st_translate_vertex_program(struct st_context *st,
                            struct st_vertex_program *stvp)
{
   struct ureg_program *ureg;
   enum pipe_error error;
   unsigned num_outputs = 0;
   unsigned attr;
   ubyte output_semantic_name[VARYING_SLOT_MAX] = {0};
   ubyte output_semantic_index[VARYING_SLOT_MAX] = {0};

   stvp->num_inputs = 0;
   memset(stvp->input_to_index, ~0, sizeof(stvp->input_to_index));

   // 如果位置不变，插入MVP代码
   if (stvp->Base.arb.IsPositionInvariant)
      _mesa_insert_mvp_code(st->ctx, &stvp->Base);

   // 确定输入的数量、VERT_ATTRIB_x到TGSI通用输入索引的映射，以及输入属性语义信息。
   for (attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
      if ((stvp->Base.info.inputs_read & BITFIELD64_BIT(attr)) != 0) {
         stvp->input_to_index[attr] = stvp->num_inputs;
         stvp->index_to_input[stvp->num_inputs] = attr;
         stvp->num_inputs++;
         if ((stvp->Base.DualSlotInputs & BITFIELD64_BIT(attr)) != 0) {
            // 添加双属性的第二部分的占位符
            stvp->index_to_input[stvp->num_inputs] = ST_DOUBLE_ATTRIB_PLACEHOLDER;
            stvp->num_inputs++;
         }
      }
   }
   // 有点hack，预先设置可能未使用的edgeflag输入
   stvp->input_to_index[VERT_ATTRIB_EDGEFLAG] = stvp->num_inputs;
   stvp->index_to_input[stvp->num_inputs] = VERT_ATTRIB_EDGEFLAG;

   // 计算顶点程序输出到插槽的映射。
   for (attr = 0; attr < VARYING_SLOT_MAX; attr++) {
      if ((stvp->Base.info.outputs_written & BITFIELD64_BIT(attr)) == 0) {
         stvp->result_to_output[attr] = ~0;
      }
      else {
         unsigned slot = num_outputs++;

         stvp->result_to_output[attr] = slot;

         unsigned semantic_name, semantic_index;
         tgsi_get_gl_varying_semantic(attr, st->needs_texcoord_semantic,
                                      &semantic_name, &semantic_index);
         output_semantic_name[slot] = semantic_name;
         output_semantic_index[slot] = semantic_index;
      }
   }
   // 类似上面的hack，预先设置可能未使用的edgeflag输出
   stvp->result_to_output[VARYING_SLOT_EDGE] = num_outputs;
   output_semantic_name[num_outputs] = TGSI_SEMANTIC_EDGEFLAG;
   output_semantic_index[num_outputs] = 0;


   ureg = ureg_create_with_screen(PIPE_SHADER_VERTEX, st->pipe->screen);
        uint i;
        struct ureg_program *ureg = CALLOC_STRUCT( ureg_program );
        for (i = 0; i < ARRAY_SIZE(ureg->properties); i++)
          ureg->properties[i] = ~0;

   if (ureg == NULL)
      return false;

   if (stvp->Base.info.clip_distance_array_size)
      ureg_property(ureg, TGSI_PROPERTY_NUM_CLIPDIST_ENABLED,
                    stvp->Base.info.clip_distance_array_size);
   if (stvp->Base.info.cull_distance_array_size)
      ureg_property(ureg, TGSI_PROPERTY_NUM_CULLDIST_ENABLED,
                    stvp->Base.info.cull_distance_array_size);

   if (stvp->glsl_to_tgsi) {
       [jump tgsi 生成]
   } 


   stvp->tgsi.tokens = ureg_get_tokens(ureg, &stvp->num_tgsi_tokens);
   ureg_destroy(ureg);

   if (stvp->glsl_to_tgsi) {
      stvp->glsl_to_tgsi = NULL;
      st_store_ir_in_disk_cache(st, &stvp->Base, false);
   }

   return stvp->tgsi.tokens != NULL;
}

```

###  TGSI_SEMANTIC_*的生成

output_semantic_name[slot] = semantic_name;
output_semantic_index[slot] = semantic_index;

```
tgsi_get_gl_varying_semantic(gl_varying_slot attr,
                             bool needs_texcoord_semantic,
                             unsigned *semantic_name,
```

### tgsi 生成

```

 if (stvp->glsl_to_tgsi) {
      error = st_translate_program(st->ctx,
                                   PIPE_SHADER_VERTEX,
                                   ureg,
                                   stvp->glsl_to_tgsi,
                                   &stvp->Base,
                                   // 输入
                                   stvp->num_inputs,
                                   stvp->input_to_index,
                                   NULL, // inputSlotToAttr
                                   NULL, // input semantic name
                                   NULL, // input semantic index
                                   NULL, // interp mode
                                   // 输出
                                   num_outputs,
                                   stvp->result_to_output,
                                   output_semantic_name,
                                   output_semantic_index);

      st_translate_stream_output_info(stvp->glsl_to_tgsi,
                                      stvp->result_to_output,
                                      &stvp->tgsi.stream_output);

      free_glsl_to_tgsi_visitor(stvp->glsl_to_tgsi);
   } 


```
#### Mesa gl_program 翻译/ureg信息填充/ st_translate_program

```

/**
 * 将中间表示(IR)（glsl_to_tgsi_instruction）翻译成TGSI格式。
 * \param program  要翻译的程序
 * \param numInputs  使用的输入寄存器数量
 * \param inputMapping  将Mesa片段程序输入映射到TGSI通用输入索引
 * \param inputSemanticName  每个输入的TGSI_SEMANTIC标志
 * \param inputSemanticIndex  每个输入的语义索引（例如，哪个纹理坐标）
 * \param interpMode  每个输入的TGSI_INTERPOLATE_LINEAR/PERSP模式
 * \param numOutputs  使用的输出寄存器数量
 * \param outputMapping  将Mesa片段程序输出映射到TGSI通用输出
 * \param outputSemanticName  每个输出的TGSI_SEMANTIC标志
 * \param outputSemanticIndex  每个输出的语义索引（例如，哪个纹理坐标）
 *
 * \return  PIPE_OK 或 PIPE_ERROR_OUT_OF_MEMORY
 */
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
   const ubyte outputMapping[]=result_to_output,
   const ubyte outputSemanticName[],
   const ubyte outputSemanticIndex[])
{
   struct pipe_screen *screen = st_context(ctx)->pipe->screen;
   struct st_translate *t;
   unsigned i;
   struct gl_program_constants *frag_const =
      &ctx->Const.Program[MESA_SHADER_FRAGMENT];
   enum pipe_error ret = PIPE_OK;

   t = CALLOC_STRUCT(st_translate);

   t->procType = procType;
   t->need_uarl = !screen->get_param(screen, PIPE_CAP_TGSI_ANY_REG_AS_ADDRESS);
   t->inputMapping = inputMapping;
   t->outputMapping = outputMapping;
   t->ureg = ureg;
   t->num_temp_arrays = program->next_array;
   if (t->num_temp_arrays)
      t->arrays = (struct ureg_dst*)
                  calloc(t->num_temp_arrays, sizeof(t->arrays[0]));

   /*
    * 声明输入属性。
    */
   switch (procType) {
   case PIPE_SHADER_FRAGMENT:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_TESS_EVAL:
   case PIPE_SHADER_TESS_CTRL:
      sort_inout_decls_by_slot(program->inputs, program->num_inputs, inputMapping);

      for (i = 0; i < program->num_inputs; ++i) {
         // program->inputs / decl->mesa_index see 2498 glsl_to_tgsi_visitor::visit(ir_dereference_variable *)
         struct inout_decl *decl = &program->inputs[i];
         unsigned slot = inputMapping[decl->mesa_index];
         struct ureg_src src;
         ubyte tgsi_usage_mask = decl->usage_mask;

         if (glsl_base_type_is_64bit(decl->base_type)) {
            if (tgsi_usage_mask == 1)
               tgsi_usage_mask = TGSI_WRITEMASK_XY;
            else if (tgsi_usage_mask == 2)
               tgsi_usage_mask = TGSI_WRITEMASK_ZW;
            else
               tgsi_usage_mask = TGSI_WRITEMASK_XYZW;
         }

         enum tgsi_interpolate_mode interp_mode = TGSI_INTERPOLATE_CONSTANT;
         enum tgsi_interpolate_loc interp_location = TGSI_INTERPOLATE_LOC_CENTER;
         if (procType == PIPE_SHADER_FRAGMENT) {
            assert(interpMode);
            interp_mode = interpMode[slot] != TGSI_INTERPOLATE_COUNT ?
               (enum tgsi_interpolate_mode) interpMode[slot] :
               st_translate_interp(decl->interp, inputSlotToAttr[slot]);

            interp_location = (enum tgsi_interpolate_loc) decl->interp_loc;
         }

         src = ureg_DECL_fs_input_cyl_centroid_layout(ureg,
                  (enum tgsi_semantic) inputSemanticName[slot],
                  inputSemanticIndex[slot],
                  interp_mode, 0, interp_location, slot, tgsi_usage_mask,
                  decl->array_id, decl->size);

         for (unsigned j = 0; j < decl->size; ++j) {
            if (t->inputs[slot + j].File != TGSI_FILE_INPUT) {
               /* ArrayID 在 dst_register 中设置 */
               t->inputs[slot + j] = src;
               t->inputs[slot + j].ArrayID = 0;
               t->inputs[slot + j].Index += j;
            }
         }
      }
      break;
   case PIPE_SHADER_VERTEX:
      for (i = 0; i < numInputs; i++) {
         t->inputs[i] = ureg_DECL_vs_input(ureg, i);
      }
      break;
   case PIPE_SHADER_COMPUTE:
      break;
   default:
      assert(0);
   }

   /*
    * 声明输出属性。
    */
   switch (procType) {
   case PIPE_SHADER_FRAGMENT:
   case PIPE_SHADER_COMPUTE:
      break;
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_TESS_EVAL:
   case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_VERTEX:
      sort_inout_decls_by_slot(program->outputs, program->num_outputs, outputMapping);

      for (i = 0; i < program->num_outputs; ++i) {
         struct inout_decl *decl = &program->outputs[i];
         unsigned slot = outputMapping[decl->mesa_index];
         struct ureg_dst dst;
         ubyte tgsi_usage_mask = decl->usage_mask;

         if (glsl_base_type_is_64bit(decl->base_type)) {
            if (tgsi_usage_mask == 1)
               tgsi_usage_mask = TGSI_WRITEMASK_XY;
            else if (tgsi_usage_mask == 2)
               tgsi_usage_mask = TGSI_WRITEMASK_ZW;
            else
               tgsi_usage_mask = TGSI_WRITEMASK_XYZW;
         }

         dst = ureg_DECL_output_layout(ureg,
                     (enum tgsi_semantic) outputSemanticName[slot],
                     outputSemanticIndex[slot],
                     decl->gs_out_streams,
                     slot, tgsi_usage_mask, decl->array_id, decl->size, decl->invariant);

         dst.Invariant = decl->invariant;
         for (unsigned j = 0; j < decl->size; ++j) {
            if (t->outputs[slot + j].File != TGSI_FILE_OUTPUT) {
               /* ArrayID 在 dst_register 中设置 */
               t->outputs[slot + j] = dst;
               t->outputs[slot + j].ArrayID = 0;
               t->outputs[slot + j].Index += j;
               t->outputs[slot + j].Invariant = decl->invariant;
            }
         }
      }
      break;
   default:
      assert(0);
   }

   if (procType == PIPE_SHADER_FRAGMENT) {
      if (program->shader->Program->info.fs.early_fragment_tests ||
          program->shader->Program->info.fs.post_depth_coverage) {
         ureg_property(ureg, TGSI_PROPERTY_FS_EARLY_DEPTH_STENCIL, 1);

         if (program->shader->Program->info.fs.post_depth_coverage)
            ureg_property(ureg, TGSI_PROPERTY_FS_POST_DEPTH_COVERAGE, 1);
      }

      if (proginfo->info.inputs_read & VARYING_BIT_POS) {
          /* 必须在设置 t->inputs 之后执行此操作。 */
          emit_wpos(st_context(ctx), t, proginfo, ureg,
                    program->wpos_transform_const);
      }

      if (proginfo->info.inputs_read & VARYING_BIT_FACE)
         emit_face_var(ctx, t);

      for (i = 0; i < numOutputs; i++) {
         switch (outputSemanticName[i]) {
         case TGSI_SEMANTIC_POSITION:
            t->outputs[i] = ureg_DECL_output(ureg,
                                             TGSI_SEMANTIC_POSITION, /* Z/Depth */
                                             outputSemanticIndex[i]);
            t->outputs[i] = ureg_writemask(t->outputs[i], TGSI_WRITEMASK_Z);
            break;
         case TGSI_SEMANTIC_STENCIL:
            t->outputs[i] = ureg_DECL_output(ureg,
                                             TGSI_SEMANTIC_STENCIL, /* Stencil */
                                             outputSemanticIndex[i]);
            t->outputs[i] = ureg_writemask(t->outputs[i], TGSI_WRITEMASK_Y);
            break;
         case TGSI_SEMANTIC_COLOR:
            t->outputs[i] = ureg_DECL_output(ureg,
                                             TGSI_SEMANTIC_COLOR,
                                             outputSemanticIndex[i]);
            break;
         case TGSI_SEMANTIC_SAMPLEMASK:
            t->outputs[i] = ureg_DECL_output(ureg,
                                             TGSI_SEMANTIC_SAMPLEMASK,
                                             outputSemanticIndex[i]);
            /* TODO: 如果我们支持超过32个采样，则必须将其变成数组。 */
            t->outputs[i] = ureg_writemask(t->outputs[i], TGSI_WRITEMASK_X);
            break;
         default:
            assert(!"fragment shader outputs must be POSITION/STENCIL/COLOR");
            ret = PIPE_ERROR_BAD_INPUT;
            goto out;
         }
      }
   }
   else if (procType == PIPE_SHADER_VERTEX) {
      for (i = 0; i < numOutputs; i++) {
         if (outputSemanticName[i] == TGSI_SEMANTIC_FOG) {
            /* 强制寄存器包含雾坐标，形式为 (F, 0, 0, 1)。 */
            ureg_MOV(ureg,
                     ureg_writemask(t->outputs[i], TGSI_WRITEMASK_YZW),
                     ureg_imm4f(ureg, 0.0f, 0.0f, 0.0f, 1.0f));
            t->outputs[i] = ureg_writemask(t->outputs[i], TGSI_WRITEMASK_X);
         }
      }
   }

   if (procType == PIPE_SHADER_COMPUTE) {
      emit_compute_block_size(proginfo, ureg);
   }

   /* 声明地址寄存器。
    */
   if (program->num_address_regs > 0) {
      assert(program->num_address_regs <= 3);
      for (int i = 0; i < program->num_address_regs; i++)
         t->address[i] = ureg_DECL_address(ureg);
   }

   /* 声明其他输入寄存器
    */
   {
      GLbitfield64 sysInputs = proginfo->info.system_values_read;

      for (i = 0; sysInputs; i++) {
         if (sysInputs & (1ull << i)) {
            enum tgsi_semantic semName = _mesa_sysval_to_semantic(i);

            t->systemValues[i] = ureg_DECL_system_value(ureg, semName, 0);

            if (semName == TGSI_SEMANTIC_INSTANCEID ||
                semName == TGSI_SEMANTIC_VERTEXID) {
               /* 从Gallium的角度来看，这些系统值始终是整数，并且需要本地整数支持。
                * 但是，如果在顶点阶段支持本地整数但在像素阶段不支持（例如，i915g + draw），
                * Mesa将生成假设这些系统值为浮点数的IR。为了解决不一致性，我们插入了一个U2F。
                */
               struct st_context *st = st_context(ctx);
               struct pipe_screen *pscreen = st->pipe->screen;
               assert(procType == PIPE_SHADER_VERTEX);
               assert(pscreen->get_shader_param(pscreen, PIPE_SHADER_VERTEX, PIPE_SHADER_CAP_INTEGERS));
               (void) pscreen;
               if (!ctx->Const.NativeIntegers) {
                  struct ureg_dst temp = ureg_DECL_local_temporary(t->ureg);
                  ureg_U2F(t->ureg, ureg_writemask(temp, TGSI_WRITEMASK_X),
                           t->systemValues[i]);
                  t->systemValues[i] = ureg_scalar(ureg_src(temp), 0);
               }
            }

            if (procType == PIPE_SHADER_FRAGMENT &&
                semName == TGSI_SEMANTIC_POSITION)
               emit_wpos(st_context(ctx), t, proginfo, ureg,
                         program->wpos_transform_const);

            if (procType == PIPE_SHADER_FRAGMENT &&
                semName == TGSI_SEMANTIC_SAMPLEPOS)
               emit_samplepos_adjustment(t, program->wpos_transform_const

```
### gl_program 定义


```

/**
 * 用于任何类型程序对象的基类
 */
struct gl_program
{
   /** FIXME: 在我们将shader_info从nir_shader中分离之前，这必须是第一个 */
   struct shader_info info;

   GLuint Id;
   GLint RefCount;
   GLubyte *String;  /**< 以null结尾的程序文本 */

   /** GL_VERTEX/FRAGMENT_PROGRAM_ARB, GL_GEOMETRY_PROGRAM_NV */
   GLenum16 Target;
   GLenum16 Format;    /**< 字符串编码格式 */

   GLboolean _Used;        /**< 曾用于绘制吗？用于调试 */

   struct nir_shader *nir;

   /* 保存并与元数据一起恢复。使用ralloc释放。 */
   void *driver_cache_blob;
   size_t driver_cache_blob_size;

   bool is_arb_asm; /** 是否为ARB汇编风格的程序 */

   /** 是否将此程序写入磁盘着色器缓存 */
   bool program_written_to_cache;

   /** 一个位字段，指示哪些顶点着色器输入占用两个插槽
    *
    * 这用于从GL API中的单插槽输入位置映射到着色器中的双插槽双输入位置。此字段在链接时设置一次，之后不再更新，以确保映射保持一致。
    *
    * 注意：原始着色器源中可能存在由于在DualSlotInputs计算之前由编译器消除而不出现在此位字段中的双插槽变量。在DualSlotInputs计算之后由于编译器优化消除此类变量，此位字段中设置的位可能存在，但着色器从不读取它们。
    */
   GLbitfield64 DualSlotInputs;
   /** 用非零索引写入的OutputsWritten输出的子集。 */
   GLbitfield64 SecondaryOutputsWritten;
   /** TEXTURE_x_BIT掩码位 */
   GLbitfield16 TexturesUsed[MAX_COMBINED_TEXTURE_IMAGE_UNITS];
   /** 使用的采样器的位字段 */
   GLbitfield SamplersUsed;
   /** 用于阴影采样的纹理单元。 */
   GLbitfield ShadowSamplers;
   /** 用于samplerExternalOES的纹理单元。 */
   GLbitfield ExternalSamplersUsed;

   /* 仅片段着色器字段 */
   GLboolean OriginUpperLeft;
   GLboolean PixelCenterInteger;

   /** 程序文本中的命名参数、常量等 */
   struct gl_program_parameter_list *Parameters;

   /** 从sampler单元到纹理单元的映射（由glUniform1i()设置） */
   GLubyte SamplerUnits[MAX_SAMPLERS];

   /* FIXME: 我们应该能够使这个结构体成为一个联合体。然而，一些驱动程序（i915/fragment_programs，swrast/prog_execute）混合使用这些字段，我们应该修复这个问题。*/
   struct {
      /** GLSL程序使用的字段 */
      struct {
         /** 由gl_program和gl_shader_program共享的数据 */
         struct gl_shader_program_data *data;

         struct gl_active_atomic_buffer **AtomicBuffers;

         /** 链接后的变换反馈信息。 */
         struct gl_transform_feedback_info *LinkedTransformFeedback;

         /**
          * 用于子程序统一变量的类型数。
          */
         GLuint NumSubroutineUniformTypes;

         /**
          * 子程序统一变量重映表
          * 基于程序级统一变量重映表。
          */
         GLuint NumSubroutineUniforms; /* 非稀疏总数 */
         GLuint NumSubroutineUniformRemapTable;
         struct gl_uniform_storage **SubroutineUniformRemapTable;

         /**
          * 此阶段的子程序函数数和存储它们的空间。
          */
         GLuint NumSubroutineFunctions;
         GLuint MaxSubroutineFunctionIndex;
         struct gl_subroutine_function *SubroutineFunctions;

         /**
          * 从图像统一索引到图像单元的映射（由glUniform1i()设置）
          *
          * 链接器为每个图像统一变量分配一个图像统一索引。每个统一变量关联的图像索引存储在gl_uniform_storage :: image字段中。
          */
         GLubyte ImageUnits[MAX_IMAGE_UNIFORMS];

         /**
          * 每个图像统一索引为着色器中的每个图像统一索引指定的访问限定符。可选择为GL_READ_ONLY、GL_WRITE_ONLY、GL_READ_WRITE或GL_NONE，以指示只读、只写或读写。
          *
          * 它可能不同，尽管比相应图像单元的gl_image_unit :: Access值更严格。
          */
         GLenum16 ImageAccess[MAX_IMAGE_UNIFORMS];

         struct gl_uniform_block **UniformBlocks;
         struct gl_uniform_block **ShaderStorageBlocks;

         /** 正在采样的纹理目标（TEXTURE_1D/2D/3D/etc_INDEX） */
         GLubyte SamplerTargets[MAX_SAMPLERS];

         /**
          * 使用bindless_sampler布局限定符声明的采样器的数量
          * 由ARB_bindless_texture指定。
          */
         GLuint NumBindlessSamplers;
         GLboolean HasBoundBindlessSampler;
         struct gl_bindless_sampler *BindlessSamplers;

         /**
          * 使用bindless_image布局限定符声明的图像的数量
          * 由ARB_bindless_texture指定。
          */
         GLuint NumBindlessImages;
         GLboolean HasBoundBindlessImage;
         struct gl_bindless_image *BindlessImages;

         union {
            struct {
               /**
                * gl_advanced_blend_mode值的位掩码
                */
               GLbitfield BlendSupport;
            } fs;
         };
      } sh;

      /** ARB汇编风格程序字段 */
      struct {
         struct prog_instruction *Instructions;

         /**
          * 程序中使用的本地参数。
          *
          * 它是动态分配的，因为它很少被使用（仅用于汇编风格程序），一旦分配，就是MAX_PROGRAM_LOCAL_PARAMS条目。
          */
         GLfloat (*LocalParams)[4];

         /** 具有间接寻址的寄存器文件读/写的位掩码。 (1 << PROGRAM_x) 位的掩码。 */
         GLbitfield IndirectRegisterFiles;

         /** 逻辑计数 */
         /*@{*/
         GLuint NumInstructions;
         GLuint NumTemporaries;
         GLuint NumParameters;
         GLuint NumAttributes;
         GLuint NumAddressRegs;
         GLuint NumAluInstructions;
         GLuint NumTexInstructions;
         GLuint NumTexIndirections;
         /*@}*/
         /** 本地、实际h/w计数 */
         /*@{*/
         GLuint NumNativeInstructions;
         GLuint NumNativeTemporaries;
         GLuint NumNativeParameters;
         GLuint NumNativeAttributes;
         GLuint NumNativeAddressRegs;
         GLuint NumNativeAluInstructions;
         GLuint NumNativeTexInstructions;
         GLuint NumNativeTexIndirections;
         /*@}*/

         /** 仅在ARB汇编风格程序中可以为true。仅对顶点程序有效。 */
         GLboolean IsPositionInvariant;
      } arb;
   };
}

```
#### vs的程序的输入翻译成ureg_src

```
for (i = 0; i < numInputs; i++) {
    t->inputs[i] = ureg_DECL_vs_input(ureg, i);
        ureg->vs_inputs[index/32] |= 1 << (index % 32);
        return ureg_src_register( TGSI_FILE_INPUT, index=i);
            return ureg_src_array_register(file=TGSI_FILE_INPUT, index, 0);
                struct ureg_src src;
                src.File = file;
                src.xxx=...;
                return src;
}
```

#### fs,cs,gs, tes,tcs, ureg_src

### 

## ir_call 生成


## IR visitor类的关系

### ir_hierarchical_visitor基类

```
/**
 * IR指令树的层次访问器的基类
 *
 * 层次访问器与传统访问器在几个重要方面有所不同。与在复合中的每个子类中有一个`visit`方法不同，
 * 层次访问器有三种访问方法。叶子节点类具有传统的`visit`方法。内部节点类具有`visit_enter`方法，
 * 在处理子节点之前调用，以及`visit_leave`方法，在处理子节点之后调用。
 *
 * 此外，每个访问方法和复合中的`accept`方法都有一个返回值，用于引导导航。任何访问方法都可以选择继续
 * 正常访问树（通过返回`visit_continue`），立即终止访问任何进一步的节点（通过返回`visit_stop`），
 * 或停止访问同级节点（通过返回`visit_continue_with_parent`）。
 *
 * 这两个更改结合在一起，允许将子节点的导航实现在复合的`accept`方法中。叶子节点类的`accept`方法将简单
 * 地调用`visit`方法，如常规情况下，并传递其返回值。内部节点类的`accept`方法将调用`visit_enter`方法，
 * 调用每个子节点的`accept`方法，最后调用`visit_leave`方法。如果其中任何一个返回值不是`visit_continue`，
 * 则必须采取正确的操作。
 *
 * 最终的好处是层次访问器基类不需要是抽象的。可以提供每个`visit`，`visit_enter`和`visit_leave`方法的默认实现。
 * 默认情况下，这些方法中的每一个都只是简单地返回`visit_continue`。这允许显著减少派生类代码。
 *
 * 有关层次访问器的更多信息，请参见：
 *
 *    http://c2.com/cgi/wiki?HierarchicalVisitorPattern
 *    http://c2.com/cgi/wiki?HierarchicalVisitorDiscussion
 */

class ir_hierarchical_visitor {
public:
   ir_hierarchical_visitor();

   /**
    * \name 叶子节点类的访问方法
    */
   /*@{*/
   virtual ir_visitor_status visit(class ir_rvalue *);
   virtual ir_visitor_status visit(class ir_variable *);
   virtual ir_visitor_status visit(class ir_constant *);
   virtual ir_visitor_status visit(class ir_loop_jump *);
   virtual ir_visitor_status visit(class ir_barrier *);

   /**
    * ir_dereference_variable在技术上不是叶子节点，但在这里作为叶子节点处理有几个原因。
    * 通过不自动访问ir_dereference_variable中的一个子ir_variable节点，可以始终将ir_variable节点
    * 处理为变量声明。使用非层次访问器的代码必须设置一个“在解引用中”标志来确定如何处理ir_variable。
    * 通过强制访问者在ir_dereference_variable访问者内处理ir_variable，可以避免此修补。
    *
    * 另外，我无法想象将进入和离开方法分开的用途。在进入和离开方法中可以完成的任何事情都可以在访问方法中完成。
    */
   virtual ir_visitor_status visit(class ir_dereference_variable *);
   /*@}*/

   /**
    * \name 内部节点类的访问方法
    */
   /*@{*/
   virtual ir_visitor_status visit_enter(class ir_loop *);
   virtual ir_visitor_status visit_leave(class ir_loop *);
   virtual ir_visitor_status visit_enter(class ir_function_signature *);
   virtual ir_visitor_status visit_leave(class ir_function_signature *);
   virtual ir_visitor_status visit_enter(class ir_function *);
   virtual ir_visitor_status visit_leave(class ir_function *);
   virtual ir_visitor_status visit_enter(class ir_expression *);
   virtual ir_visitor_status visit_leave(class ir_expression *);
   virtual ir_visitor_status visit_enter(class ir_texture *);
   virtual ir_visitor_status visit_leave(class ir_texture *);
   virtual ir_visitor_status visit_enter(class ir_swizzle *);
   virtual ir_visitor_status visit_leave(class ir_swizzle *);
   virtual ir_visitor_status visit_enter(class ir_dereference_array *);
   virtual ir_visitor_status visit_leave(class ir_dereference_array *);
   virtual ir_visitor_status visit_enter(class ir_dereference_record *);
   virtual ir_visitor_status visit_leave(class ir_dereference_record *);
   virtual ir_visitor_status visit_enter(class ir_assignment *);
   virtual ir_visitor_status visit_leave(class ir_assignment *);
   virtual ir_visitor_status visit_enter(class ir_call *);
   virtual ir_visitor_status visit_leave(class ir_call *);
   virtual ir_visitor_status visit_enter(class ir_return *);
   virtual ir_visitor_status visit_leave(class ir_return *);
   virtual ir_visitor_status visit_enter(class ir_discard *);
   virtual ir_visitor_status visit_leave(class ir_discard *);
   virtual ir_visitor_status visit_enter(class ir_if *);
   virtual ir_visitor_status visit_leave(class ir_if *);
   virtual ir_visitor_status visit_enter(class ir_emit_vertex *);
   virtual ir_visitor_status visit_leave(class ir_emit_vertex *);
   virtual ir_visitor_status visit_enter(class ir_end_primitive *);
   virtual ir_visitor_status visit_leave(class ir_end_primitive *);
   /*@}*/


   /**
    * 使用访问者处理指令的链接列表的实用函数
    */
   void run(struct exec_list *instructions);

   /* 一些访问者可能需要为子树的部分插入新的变量声明和赋值，
    * 这意味着它们需要指向流中当前指令的指针，而不仅仅是它们
    * 在该指令根的树中的节点。
    *
    * 这是通过visit_list_elements实现的——如果访问者未由它调用，将不会发生什么好事。
    */
   class ir_instruction *base_ir;

   /**
    * 在访问者的赋值语句的左侧吗？
    *
    * 这由ir_assignment::accept方法设置和清除。
    */
   bool in_assignee;
};


```
## 子类

class ir_array_refcount_visitor : public ir_hierarchical_visitor {


struct glsl_to_tgsi_visitor : public ir_visitor {
## program_resource_visitor 基类

用于处理与程序资源对应的变量的所有叶子字段的类。

叶子字段是应用程序可以使用 \c glGetProgramResourceIndex 查询的变量的所有部分
（或者可以通过 \c glGetProgramResourceName 返回的部分）。

派生类可以从这个类派生以实现特定功能。
该类仅提供迭代叶子的机制。派生类必须实现 \c ::visit_field，可以重写 \c ::process。


### count_uniform_size派生类

用于帮助计算一组uniform的存储需求的类

随着uniform被添加到活动集合中，活动uniform的数量和这些uniform的存储需求会累积。
活动uniform将被添加到构造函数中提供的哈希表中。

如果同一个uniform被多次添加（即，为每个着色器目标添加一次），它只会被计算一次。



### 


class count_uniform_size : public program_resource_visitor {

## GLSL 限定符的定义

```
 
/**
 * GLSL类型限定符结构体
 */
struct ast_type_qualifier {

     /**
    * 标志位的联合体，通过名称访问。
    */
   union flags {
      struct {
         unsigned invariant:1;
         unsigned precise:1;
         unsigned constant:1;
         unsigned attribute:1;
         unsigned varying:1;
         unsigned in:1;
         unsigned out:1;
         unsigned centroid:1;
         unsigned sample:1;
         unsigned patch:1;
         unsigned uniform:1;
         unsigned buffer:1;
         unsigned shared_storage:1;
         unsigned smooth:1;
         unsigned flat:1;
         unsigned noperspective:1;

         /** GL_ARB_fragment_coord_conventions的布局限定符 */
         /*@{*/
         unsigned origin_upper_left:1;
         unsigned pixel_center_integer:1;
         /*@}*/

         /**
          * 如果使用GL_ARB_enhanced_layouts的"align"布局限定符，则设置标志位。
          */
         unsigned explicit_align:1;

         /**
          * 如果使用GL_ARB_explicit_attrib_location的"location"布局限定符，则设置标志位。
          */
         unsigned explicit_location:1;

         /**
          * 如果使用GL_ARB_explicit_attrib_location的"index"布局限定符，则设置标志位。
          */
         unsigned explicit_index:1;

         /**
          * 如果使用GL_ARB_enhanced_layouts的"component"布局限定符，则设置标志位。
          */
         unsigned explicit_component:1;

         /**
          * 如果使用GL_ARB_shading_language_420pack的"binding"布局限定符，则设置标志位。
          */
         unsigned explicit_binding:1;

         /**
          * 如果使用GL_ARB_shader_atomic counter的"offset"布局限定符，则设置标志位。
          */
         unsigned explicit_offset:1;

         /** GL_AMD_conservative_depth的布局限定符 */
         /** \{ */
         unsigned depth_type:1;
         /** \} */

         /** GL_ARB_uniform_buffer_object的布局限定符 */
         /** \{ */
         unsigned std140:1;
         unsigned std430:1;
         unsigned shared:1;
         unsigned packed:1;
         unsigned column_major:1;
         unsigned row_major:1;
         /** \} */

         /** GLSL 1.50几何着色器的布局限定符 */
         /** \{ */
         unsigned prim_type:1;
         unsigned max_vertices:1;
         /** \} */

         /**
          * 计算着色器的local_size_{x,y,z}标志位。第0位表示local_size_x，依此类推。
          */
         unsigned local_size:3;

         /** ARB_compute_variable_group_size的布局限定符 */
         /** \{ */
         unsigned local_size_variable:1;
         /** \} */

         /** ARB_shader_image_load_store的布局和内存限定符 */
         /** \{ */
         unsigned early_fragment_tests:1;
         unsigned explicit_image_format:1;
         unsigned coherent:1;
         unsigned _volatile:1;
         unsigned restrict_flag:1;
         unsigned read_only:1; /**< "readonly"限定符。 */
         unsigned write_only:1; /**< "writeonly"限定符。 */
         /** \} */

         /** GL_ARB_gpu_shader5的布局限定符 */
         /** \{ */
         unsigned invocations:1;
         unsigned stream:1; /**< 是否分配了stream值 */
         unsigned explicit_stream:1; /**< 是否由着色器代码显式分配了stream值 */
         /** \} */

         /** GL_ARB_enhanced_layouts的布局限定符 */
         /** \{ */
         unsigned explicit_xfb_offset:1; /**< 是否由着色器代码显式分配了xfb_offset值 */
         unsigned xfb_buffer:1; /**< 是否分配了xfb_buffer值 */
         unsigned explicit_xfb_buffer:1; /**< 是否由着色器代码显式分配了xfb_buffer值 */
         unsigned xfb_stride:1; /**< xfb_stride值是否尚未与全局值合并 */
         unsigned explicit_xfb_stride:1; /**< 是否由着色器代码显式分配了xfb_stride值 */
         /** \} */

         /** GL_ARB_tessellation_shader的布局限定符 */
         /** \{ */
         /* tess eval input layout */
         /* gs prim_type reused for primitive mode */
         unsigned vertex_spacing:1;
         unsigned ordering:1;
         unsigned point_mode:1;
         /* tess control output layout */
         unsigned vertices:1;
         /** \} */

         /** GL_ARB_shader_subroutine的限定符 */
         /** \{ */
         unsigned subroutine:1;  /**< 是否标记为'subroutine' */
         /** \} */

         /** GL_KHR_blend_equation_advanced的限定符 */
         /** \{ */
         unsigned blend_support:1; /**< 是否存在任何blend_support_限定符 */
         /** \} */

         /**
          * 如果使用GL_ARB_post_depth_coverage布局限定符，则设置标志位。
          */
         unsigned post_depth_coverage:1;

         /**
          * 由ARB_fragment_shader_interlock添加的布局限定符的标志位。
          */
         unsigned pixel_interlock_ordered:1;
         unsigned pixel_interlock_unordered:1;
         unsigned sample_interlock_ordered:1;
         unsigned sample_interlock_unordered:1;

         /**
          * 如果使用GL_INTEL_conservartive_rasterization布局限定符，则设置标志位。
          */
         unsigned inner_coverage:1;

         /** GL_ARB_bindless_texture的布局限定符 */
         /** \{ */
         unsigned bindless_sampler:1;
         unsigned bindless_image:1;
         unsigned bound_sampler:1;
         unsigned bound_image:1;
         /** \} */

         /** GL_EXT_shader_framebuffer_fetch_non_coherent的布局限定符 */
         /** \{ */
         unsigned non_coherent:1;
         /** \} */
      }
      /** \brief 标志位集合，通过位掩码访问。 */
      q;

      /** \brief 标志位集合，通过位集合访问。 */
      bitset_t i;
   } flags;

   /** 类型的精度（highp/medium/lowp）。 */
   unsigned precision:2;

   /** GL_AMD_conservative_depth的布局限定符类型。 */
   unsigned depth_type:3;

   /**
    * 通过GL_ARB_enhanced_layouts的"align"布局限定符指定的对齐。
    */
   ast_expression *align;

   /** GL_ARB_gpu_shader5中的几何着色器调用次数。 */
   ast_layout_expression *invocations;

   /**
    * 通过GL_ARB_explicit_attrib_location布局限定符指定的位置。
    *
    * \note
    * 仅在设置了\c explicit_location时，此字段才有效。
    */
   ast_expression *location;

   /**
    * 通过GL_ARB_explicit_attrib_location布局限定符指定的索引。
    *
    * \note
    * 仅在设置了\c explicit_index时，此字段才有效。
    */
   ast_expression *index;

   /**
    * 通过GL_ARB_enhanced_layouts指定的组件。
    *
    * \note
    * 仅在设置了\c explicit_component时，此字段才有效。
    */
   ast_expression *component;

   /** GLSL 1.50几何着色器中的最大输出顶点数。 */
   ast_layout_expression *max_vertices;

   /** GLSL 1.50几何着色器中的流。 */
   ast_expression *stream;

   /** 通过GL_ARB_enhanced_layouts关键字指定的xfb_buffer。 */
   ast_expression *xfb_buffer;

   /** 通过GL_ARB_enhanced_layouts关键字指定的xfb_stride。 */
   ast_expression *xfb_stride;

   /** 每个缓冲区的全局xfb_stride值。 */
   ast_layout_expression *out_xfb_stride[MAX_FEEDBACK_BUFFERS];

   /**
    * GLSL 1.50几何着色器和镶嵌着色器中的输入或输出原语类型。
    */
   GLenum prim_type;

   /**
    * 通过GL_ARB_shading_language_420pack的"binding"关键字指定的绑定。
    *
    * \note
    * 仅在设置了\c explicit_binding时，此字段才有效。
    */
   ast_expression *binding;

   /**
    * 通过GL_ARB_shader_atomic_counter的"offset"关键字、
    * 或者通过GL_ARB_enhanced_layouts的"offset"关键字，
    * 或者通过GL_ARB_enhanced_layouts的"xfb_offset"关键字指定的偏移。
    *
    * \note
    * 仅在设置了\c explicit_offset时，此字段才有效。
    */
   ast_expression *offset;

   /**
    * 通过GL_ARB_compute_shader的"local_size_{x,y,z}"布局限定符指定的本地大小。
    * 如果flags.q.local_size & (1 << i)被设置，那么该数组的第i个元素是有效的。
    */
   ast_layout_expression *local_size[3];

   /** 镶嵌着色器：顶点间距（equal, fractional even/odd） */
   enum gl_tess_spacing vertex_spacing;

   /** 镶嵌着色器：顶点顺序（CW或CCW） */
   GLenum ordering;

   /** 镶嵌着色器：点模式 */
   bool point_mode;

   /** 镶嵌着色器控制着色器：输出顶点数 */
   ast_layout_expression *vertices;

   /**
    * 通过ARB_shader_image_load_store布局限定符指定的图像格式。
    *
    * \note
    * 仅在设置了\c explicit_image_format时，此字段才有效。
    */
   GLenum image_format;

   /**
    * 从或写入此图像的数据的基本类型。只允许以下枚举值：GLSL_TYPE_UINT，GLSL_TYPE_INT，GLSL_TYPE_FLOAT。
    *
    * \note
    * 仅在设置了\c explicit_image_format时，此字段才有效。
    */
   glsl_base_type image_base_type;

}
```
## ir.data 的定义

class ir_variable : public ir_instruction : public exec_node

```c
struct ir_variable_data {

    /**
     * 变量是否为只读？
     *
     * 对于声明为const、着色器输入和uniform的变量，此标志位被设置。
     */
    unsigned read_only:1;
    unsigned centroid:1;
    unsigned sample:1;
    unsigned patch:1;

    /**
     * 着色器中是否明确设置了'invariant'限定符？
     *
     * 用于交叉验证限定符。
     */
    unsigned explicit_invariant:1;

    /**
     * 变量是否是不变的？
     *
     * 可能是因为在着色器中明确设置了'invariant'限定符，或者被用于其他不变变量的计算。
     */
    unsigned invariant:1;
    unsigned precise:1;

    /**
     * 此变量是否已被用于读取或写入？
     *
     * 几个GLSL语义检查需要知道变量是否已经被使用。例如，重新声明已被使用的不变变量是一个错误。
     */
    unsigned used:1;

    /**
     * 此变量是否已被静态分配？
     *
     * 这回答了在ast_to_hir期间着色器的任何路径中变量是否被分配。这并不回答在死代码删除后它是否仍然被写入，也不在Mesa的固定功能或ARB程序路径中维护。
     */
    unsigned assigned:1;

    /**
     * 启用单独的着色器程序时，只有多阶段单独程序之间的输入/输出可以安全地从着色器接口中删除。其他输入/输出必须保持活动状态。
     */
    unsigned always_active_io:1;

    /**
     * 枚举指示变量的声明方式。参见ir_var_declaration_type。
     *
     * 用于检测某些非法的变量重声明。
     */
    unsigned how_declared:2;

    /**
     * 变量的存储类。
     *
     * 请参阅ir_variable_mode。
     */
    unsigned mode:4;

    /**
     * 着色器输入/输出的插值模式
     *
     * 请参阅glsl_interp_mode。
     */
    unsigned interpolation:2;

    /**
     * ARB_fragment_coord_conventions
     */
    unsigned origin_upper_left:1;
    unsigned pixel_center_integer:1;

    /**
     * 变量在着色器中是否明确设置了位置？
     *
     * 如果位置在着色器中明确设置，则它<b>不能</b>被链接器或API更改（例如，调用glBindAttribLocation无效）。
     */
    unsigned explicit_location:1;
    unsigned explicit_index:1;

    /**
     * 变量在着色器中是否明确设置了初始绑定？
     *
     * 如果是这样，constant_value包含表示初始绑定点的整数ir_constant。
     */
    unsigned explicit_binding:1;

    /**
     * 变量是否在着色器中明确设置了初始组件？
     */
    unsigned explicit_component:1;

    /**
     * 此变量是否具有初始化器？
     *
     * 链接器使用此信息交叉验证全局变量的初始化器。
     */
    unsigned has_initializer:1;

    /**
     * 此变量是否是尚未与管道中另一个阶段的变量匹配的通用输出或输入？
     *
     * 链接器在为通用输入和输出分配位置时使用此标志作为临时存储。
     */
    unsigned is_unmatched_generic_inout:1;

    /**
     * 此变量是否仅由变换反馈使用？
     *
     * 链接器使用此标志决定是否安全地打包可变量。
     */
    unsigned is_xfb_only:1;

    /**
     * 着色器中是否设置了转换反馈缓冲区？
     */
    unsigned explicit_xfb_buffer:1;

    /**
     * 着色器中是否设置了转换反馈偏移量？
     */
    unsigned explicit_xfb_offset:1;

    /**
     * 着色器中是否设置了转换反馈跨距？
     */
    unsigned explicit_xfb_stride:1;

    /**
     * 如果非零，则此变量可以与其他变量一起打包到单个可变槽中，因此在访问组件时应用此偏移量。
     * 例如，偏移量为1表示该变量的x组件实际上存储在由location指定的位置的y组件中。
     */
    unsigned location_frac:2;

    /**
     * 矩阵的布局。使用glsl_matrix_layout值。
     */
    unsigned matrix_layout:2;

    /**
     * 非零表示此变量是通过降低命名接口块而创建的。
     */
    unsigned from_named_ifc_block:1;

    /**
     * 如果变量必须是着色器输入，则为非零。这对于函数参数的约束是有用的。
     */
    unsigned must_be_shader_input:1;

    /**
     * 双源混合的输出索引。
     *
     * \note
     * GLSL规范仅允许双源混合的索引为0或1。
     */
    unsigned index:1;

    /**
     * 精度限定符。
     *
     * 在桌面GLSL中，我们根本不关心精度限定符，实际上，规范说精度限定符会被忽略。
     *
     * 为了简化，我们始终使得桌面着色器上此字段始终为GLSL_PRECISION_NONE。这样所有变量都有相同的精度值，
     * 并且我们在编译器中为此字段添加的检查永远不会破坏桌面着色器的编译。
     */
    unsigned precision:2;

    /**
     * \brief gl_FragDepth的布局限定符。
     *
     * 仅当此变量为gl_FragDepth并且指定了布局限定符时，此值不等于ir_depth_layout_none。
     */
    ir_depth_layout depth_layout:3;

    /**
     * 内存限定符。
     */
    unsigned memory_read_only:1; /**< "readonly" 限定符。 */
    unsigned memory_write_only:1; /**< "writeonly" 限定符。 */
    unsigned memory_coherent:1;
    unsigned memory_volatile:1;
    unsigned memory_restrict:1;

    /**
     * ARB_shader_storage_buffer_object
     */
    unsigned from_ssbo_unsized_array:1; /**< 无尺寸数组缓冲变量。 */

    unsigned implicit_sized_array:1;

    /**
     * 是否是前一次渲染目标位置对应的帧缓冲中的内容隐式初始化的片段着色器输出。
     */
    unsigned fb_fetch_output:1;

    /**
     * 如果按照ARB_bindless_texture定义，此变量被视为无绑定。
     */
    unsigned bindless:1;

    /**
     * 如果按照ARB_bindless_texture定义，此变量被视为已绑定。
     */
    unsigned bound:1;

    /**
     * 如果访问此变量将发出警告，则发出警告。
     */
private:
    uint8_t warn_extension_index;

public:
    /** 显式指定的图像内部格式，否则为GL_NONE。 */
    uint16_t image_format;

private:
    /**
     * 使用的状态槽数
     *
     * \note
     * 如果需要，这可以存储为少至7位。如果使其更小，请在ir_variable::allocate_state_slots中添加一个断言以确保安全。
     */
    uint16_t _num_state_slots;

public:
    /**
     * 采样器、原子或UBO的初始绑定点。
     *
     * 对于数组类型，这表示第一个元素的绑定点。
     */
    int16_t binding;

    /**
     * 此变量的基本存储位置
     *
     * 此字段的确切含义取决于变量的性质。
     *
     *   - 顶点着色器输入：来自gl_vert_attrib的值之一。
     *   - 顶点着色器输出：来自gl_varying_slot的值之一。
     *   - 几何着色器输入：来自gl_varying_slot的值之一。
     *   - 几何着色器输出：来自gl_varying_slot的值之一。
     *   - 片段着色器输入：来自gl_varying_slot的值之一。
     *   - 片段着色器输出：来自gl_frag_result的值之一。
     *   - Uniforms：默认uniform块的每阶段uniform槽号。
     *   - Uniforms：对于UBO成员，是UBO定义中的索引。
     *   - 非UBO Uniforms：直到链接，都是显式位置，然后重用为存储uniform槽号。
     *   - 其他：此字段当前未使用。
     *
     * 如果变量是uniform、着色器输入或着色器输出，并且尚未分配槽，则值将为-1。
     */
    int location;

    /**
     * 对于glsl->tgsi/mesa IR，我们需要存储参数的索引用于uniform，最初代码过载了location，
     * 但这会导致间接采样器和AoA的问题。这在_mes_generate_parameters_list_for_uniforms中分配。
     */
    int param_index;

    /**
     * 顶点流输出标识符。
     *
     * 对于打包输出，设置位31，位[2*i+1,2*i]表示第i个分量的流。
     */
    unsigned stream;

    /**
     * 原子、变换反馈或块成员偏移。
     */
    unsigned offset;

    /**
     * 用常量表达式数组索引访问的最高元素。
     *
     * 不用于非数组变量。-1永远不会被访问。
     */
    int max_array_access;

    /**
     * 变换反馈缓冲。
     */
    unsigned xfb_buffer;

    /**
     * 变换反馈跨距。
     */
    unsigned xfb_stride;

    /**
     * 仅允许ir_variable直接访问私有成员。
     */
    friend class ir_variable;
};

```


## 创建shader
```
_mesa_CreateShader_no_error(GLenum type)
   return create_shader(ctx, type);
        struct gl_shader *sh;
        name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ShaderObjects, 1);
        sh = _mesa_new_shader(name, _mesa_shader_enum_to_shader_stage(type));
                 struct gl_shader *shader;
                 shader = rzalloc(NULL, struct gl_shader);
                _mesa_init_shader(shader);
                    shader->RefCount = 1;
                    shader->info.Geom.VerticesOut = -1;
                    shader->info.Geom.InputType = GL_TRIANGLES;
                    shader->info.Geom.OutputType = GL_TRIANGLE_STRIP;
        return name
    

```
## 加载源

```
_mesa_ShaderSource_no_error(GLuint shaderObj, GLsizei count,
                            const GLchar *const *string, const GLint *length)
   shader_source(ctx, shaderObj, count, string, length, true);
      sh = _mesa_lookup_shader(ctx, shaderObj);
      set_shader_source(sh, source);
        sh->Source = source;


```

## 编译shader

```
_mesa_CompileShader(GLuint shaderObj)
   _mesa_compile_shader(ctx, _mesa_lookup_shader_err(ctx, shaderObj,
                                                     "glCompileShader"));
      _mesa_glsl_compile_shader(ctx, sh, false, false, false);



_mesa_glsl_compile_shader(struct gl_context *ctx, struct gl_shader *shader,
                          bool dump_ast, bool dump_hir, bool force_recompile =false)

   const char *source = shader->Source;
   struct _mesa_glsl_parse_state *state =
      new(shader) _mesa_glsl_parse_state(ctx, shader->Stage, shader);

   state->error = glcpp_preprocess(state=realloc, shader=&source, info_log=&state->info_log,
                                   extensions=add_builtin_defines, state, gl_ctx=ctx);
	        *shader = parser->output->buf;

   _mesa_glsl_parse(state);
      state->symbols = new(ralloc_parent(state)) glsl_symbol_table;
      state->symbols->add_default_precision_qualifier("sampler2D", ast_precision_low);
      _mesa_glsl_initialize_types(state);
      if ((yyvsp[0].node) != NULL)
         state->translation_unit.push_tail(& (yyvsp[0].node)->link);
      state->symbols->add_type((yyvsp[-1].identifier), glsl_type::get_subroutine_instance((yyvsp[-1].identifier)));
      state->symbols->add_function(new(state) ir_function((yyvsp[-1].identifier)));
      void *ctx = state->linalloc;
      (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[0].identifier), ir_var_auto));

    // 抽象语法树构建完成
   if (dump_ast) {
      foreach_list_typed(ast_node, ast, link, &state->translation_unit) {
         ast->print();
      }
      printf("\n\n");
   }

   shader->ir = new(shader) exec_list;
   if (!state->error && !state->translation_unit.is_empty())
      _mesa_ast_to_hir(shader->ir, state);

   
   if (dump_hir) 
     _mesa_print_ir(stdout, shader->ir, state);

   shader->symbols = new(shader->ir) glsl_symbol_table;

   if (!state->error && !shader->ir->is_empty()) {
      assign_subroutine_indexes(state);
      lower_subroutine(shader->ir, state);

      if (!ctx->Cache || force_recompile)
         opt_shader_and_create_symbol_table(ctx, state->symbols, shader);
            // 优化IR
            ...

            /* 销毁符号表。创建一个新的符号表，其中仅包含仍然存在于IR中的变量和函数。
             * 该符号表将在链接过程中使用。
             *
             * 在符号表中不能有任何被释放的对象仍然被引用。这可能导致链接器引用已释放的内存。
             *
             * 我们不必在这里担心类型或接口类型，因为它们是由glsl_type查找的飞行权重。
             */
             
           _mesa_glsl_copy_symbols_from_table(shader->ir, source_symbols,
                                      shader->symbols);
                foreach_in_list (ir_instruction, ir, shader_ir) {
                    case ir_type_function:
                    case ir_type_variable: {
                }
                src->get_interface("gl_PerVertex", ir_var_shader_in);
                dest->add_interface(iface->name, iface, ir_var_shader_in);
   if (!state->error)
      set_shader_inout_layout(shader, state);
            // 处理shaderinfo.xx.
           switch (shader->Stage) {
           case MESA_SHADER_TESS_CTRL:
              shader->info.TessCtrl.VerticesOut = 0;
              if (state->tcs_
```


### glsl预处理

glcpp

```c

glcpp_lex = custom_target(
  'glcpp-lex.c',
  input : 'glcpp-lex.l',
  output : 'glcpp-lex.c',
  command : [prog_flex, '-o', '@OUTPUT@', '@INPUT@'],
)

glcpp_parse = custom_target(
  'glcpp-parse.[ch]',
  input : 'glcpp-parse.y',
  output : ['glcpp-parse.c', 'glcpp-parse.h'],
  command : [
    prog_bison, '-o', '@OUTPUT0@', '-p', 'glcpp_parser_',
    '--defines=@OUTPUT1@', '@INPUT@',
  ],
)
```

#### glcpp 解析器的定义


### 预处理
```c

int
glcpp_preprocess(void *ralloc_ctx, const char **shader, char **info_log,
                 glcpp_extension_iterator extensions, void *state,
                 struct gl_context *gl_ctx)
{
	int errors;
	glcpp_parser_t *parser =
		glcpp_parser_create(&gl_ctx->Extensions, extensions, state, gl_ctx->API);

	if (! gl_ctx->Const.DisableGLSLLineContinuations)
		*shader = remove_line_continuations(parser, *shader);

	glcpp_lex_set_source_string (parser, *shader);

	glcpp_parser_parse (parser);

	if (parser->skip_stack)
		glcpp_error (&parser->skip_stack->loc, parser, "Unterminated #if\n");

	glcpp_parser_resolve_implicit_version(parser);

	ralloc_strcat(info_log, parser->info_log->buf);

	/* Crimp the buffer first, to conserve memory */
	_mesa_string_buffer_crimp_to_fit(parser->output);

	ralloc_steal(ralloc_ctx, parser->output->buf);
	*shader = parser->output->buf;

	errors = parser->error;
	glcpp_parser_destroy (parser);
	return errors;
}



```


### temp 


