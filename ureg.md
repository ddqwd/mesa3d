


```

_mesa_C


## 创建ureg程序

```
struct ureg_program *
ureg_create(enum pipe_shader_type processor);

```

## 添加TGSI属性
```c
ureg_property(struct ureg_program *ureg, unsigned name, unsigned value)



```

# ureg_program

## ureg_program的定义

```c
struct ureg_program
{
   enum pipe_shader_type processor;  // 当前shader类型
   bool supports_any_inout_decl_range;  // 声明的范围
   int next_shader_processor;  // 下一个shader处理类型

   struct {
      enum tgsi_semantic semantic_name;
      unsigned semantic_index;
      enum tgsi_interpolate_mode interp;  // 插值模式
      unsigned char cylindrical_wrap;
      unsigned char usage_mask;
      enum tgsi_interpolate_loc interp_location;  // 插值位置
      unsigned first;   // 范围起始
      unsigned last;  //  范围最终位置
      unsigned array_id; //数组的>>>>
   } input[UREG_MAX_INPUT];
   unsigned nr_inputs, nr_input_regs; // 输出存放

   unsigned vs_inputs[PIPE_MAX_ATTRIBS/32];  // vs的输出属性

   struct {
      enum tgsi_semantic semantic_name;
      unsigned semantic_index;
   } system_value[UREG_MAX_SYSTEM_VALUE];  // 系统变量存放
   unsigned nr_system_values;

   struct {
      enum tgsi_semantic semantic_name;
      unsigned semantic_index;
      unsigned streams;
      unsigned usage_mask; /* = TGSI_WRITEMASK_* */
      unsigned first;
      unsigned last;
      unsigned array_id;
      boolean invariant;
   } output[UREG_MAX_OUTPUT]; 
   unsigned nr_outputs, nr_output_regs;  //输出存放

   struct {
      union {
         float f[4];
         unsigned u[4];
         int i[4];
      } value;
      unsigned nr;
      unsigned type;   // value的类型
   } immediate[UREG_MAX_IMMEDIATE]; // 立即数存放
   unsigned nr_immediates;

   struct ureg_src sampler[PIPE_MAX_SAMPLERS];
   unsigned nr_samplers;

   struct {
      unsigned index;
      enum tgsi_texture_type target;
      enum tgsi_return_type return_type_x;
      enum tgsi_return_type return_type_y;
      enum tgsi_return_type return_type_z;
      enum tgsi_return_type return_type_w;
   } sampler_view[PIPE_MAX_SHADER_SAMPLER_VIEWS];
   unsigned nr_sampler_views;   // 纹理存放 sampler

   struct {
      unsigned index;
      enum tgsi_texture_type target;
      enum pipe_format format;
      boolean wr;
      boolean raw;
   } image[PIPE_MAX_SHADER_IMAGES];
   unsigned nr_images;   // 图像存放 image2

   struct {
      unsigned index;
      bool atomic;
   } buffer[PIPE_MAX_SHADER_BUFFERS];
   unsigned nr_buffers;   // 缓冲存放

   struct util_bitmask *free_temps;    // 缓冲池
   struct util_bitmask *local_temps; // 本地变量缓冲池
   struct util_bitmask *decl_temps; // 声明临时变量池
   unsigned nr_temps;

   unsigned array_temps[UREG_MAX_ARRAY_TEMPS]; // 输出池
   unsigned nr_array_temps;

   struct const_decl const_decls[PIPE_MAX_CONSTANT_BUFFERS];  // 声明const的

   struct hw_atomic_decl hw_atomic_decls[PIPE_MAX_HW_ATOMIC_BUFFERS];

   unsigned properties[TGSI_PROPERTY_COUNT];

   unsigned nr_addrs;
   unsigned nr_instructions;

   struct ureg_tokens domain[2];  // 生成tgsi_token的最终形式

   bool use_memory[TGSI_MEMORY_TYPE_COUNT];
};



```
## 声明系统变量
```c
struct ureg_src
ureg_DECL_system_value(struct ureg_program *ureg,
                       enum tgsi_semantic semantic_name,
                       unsigned semantic_index)
```
## 声明一个临时变量

```c
struct ureg_dst ureg_DECL_temporary( struct ureg_program *ureg )
{
   return alloc_temporary(ureg, FALSE);
}
```
## 声明一个立即数


```
enum tgsi_imm_type {
   TGSI_IMM_FLOAT32,
   TGSI_IMM_UINT32,
   TGSI_IMM_INT32,
   TGSI_IMM_FLOAT64,
   TGSI_IMM_UINT64,
   TGSI_IMM_INT64,
};
```
立即数的形式有1,2,3,4维，浮点,无符号数形式,整数形式,分别调用不同的接口最终调用的都是decl_immediate
```c

static struct ureg_src
decl_immediate( struct ureg_program *ureg,
                const unsigned *v,
                unsigned nr,
                unsigned type )
{
   unsigned i, j;
   unsigned swizzle = 0;

   /* Could do a first pass where we examine all existing immediates
    * without expanding.
    */

   for (i = 0; i < ureg->nr_immediates; i++) {
      if (ureg->immediate[i].type != type) {
         continue;
      }
      // 32 64扩展匹配
      if (match_or_expand_immediate(v,
                                    type,
                                    nr,
                                    ureg->immediate[i].value.u,
                                    &ureg->immediate[i].nr,
                                    &swizzle)) {
           -----------------------------------------------
                   unsigned nr2 = *pnr2;
                   unsigned i, j;
                
                    // 处理
                   if (type == TGSI_IMM_FLOAT64 ||
                       type == TGSI_IMM_UINT64 ||
                       type == TGSI_IMM_INT64)
                      return match_or_expand_immediate64(v, nr, v2, pnr2, swizzle);
                
                   *swizzle = 0;
                
                   for (i = 0; i < nr; i++) {
                      boolean found = FALSE;
                
                      for (j = 0; j < nr2 && !found; j++) {
                         if (v[i] == v2[j]) {
                            *swizzle |= j << (i * 2);
                            found = TRUE;
                         }
                      }
                
                      if (!found) {
                         if (nr2 >= 4) {
                            return FALSE;
                         }
                
                         v2[nr2] = v[i];
                         *swizzle |= nr2 << (i * 2);
                         nr2++;
                      }
                   }
                
                   /* Actually expand immediate only when fully succeeded.
                    */
                   *pnr2 = nr2;
                   return TRUE;

         goto out;
      }
   }

   if (ureg->nr_immediates < UREG_MAX_IMMEDIATE) {
      i = ureg->nr_immediates++;
      ureg->immediate[i].type = type;
      if (match_or_expand_immediate(v,
                                    type,
                                    nr,
                                    ureg->immediate[i].value.u,
                                    &ureg->immediate[i].nr,
                                    &swizzle)) {
         goto out;
      }
   }

   set_bad(ureg);

out:
   /* Make sure that all referenced elements are from this immediate.
    * Has the effect of making size-one immediates into scalars.
    */
   if (type == TGSI_IMM_FLOAT64 ||
       type == TGSI_IMM_UINT64 ||
       type == TGSI_IMM_INT64) {
      for (j = nr; j < 4; j+=2) {
         swizzle |= (swizzle & 0xf) << (j * 2);
      }
   } else {
      for (j = nr; j < 4; j++) {
         swizzle |= (swizzle & 0x3) << (j * 2);
      }
   }
   // swizzle擦在哦
   return ureg_swizzle(ureg_src_register(TGSI_FILE_IMMEDIATE, i),
                       (swizzle >> 0) & 0x3,
                       (swizzle >> 2) & 0x3,
                       (swizzle >> 4) & 0x3,
                       (swizzle >> 6) & 0x3);
}

```

## 确定寄存器swizzle 

```c
static inline struct ureg_src 
ureg_swizzle( struct ureg_src reg, 
              int x, int y, int z, int w )
{
   unsigned swz = ( (reg.SwizzleX << 0) |
                    (reg.SwizzleY << 2) |
                    (reg.SwizzleZ << 4) |
                    (reg.SwizzleW << 6));

   assert(reg.File != TGSI_FILE_NULL);
   assert(x < 4);
   assert(y < 4);
   assert(z < 4);
   assert(w < 4);

   reg.SwizzleX = (swz >> (x*2)) & 0x3;
   reg.SwizzleY = (swz >> (y*2)) & 0x3;
   reg.SwizzleZ = (swz >> (z*2)) & 0x3;
   reg.SwizzleW = (swz >> (w*2)) & 0x3;
   return reg;
}
```

## 确定目标写入分量

```c

static inline struct ureg_dst 
ureg_writemask( struct ureg_dst reg,
                unsigned writemask )
{
   assert(reg.File != TGSI_FILE_NULL);
   reg.WriteMask &= writemask;
   return reg;
}

```
## 分配一个新的缓冲

nr标识使用的ureg->buffer中的索引，如果没有这个则重新分配一个buffer
```c
struct ureg_src ureg_DECL_buffer(struct ureg_program *ureg, unsigned nr,
                                 bool atomic)
```

## 各种tgsi指令操作的添加

根据操作指令的不同定义了不同的宏处理，宏其实定义了不同的函数,函数内部将信息写入到ureg->domain[1]中的tokens中
在函数内部分为如下几步:
1. 写入指令信息 

ureg_emit_insn

2. 发射src, dst 信息

通过ureg_emit_dst, ureg_emit_src来完成
```c
void
ureg_emit_dst( struct ureg_program *ureg,
               struct ureg_dst dst )
{
   unsigned size = 1 + (dst.Indirect ? 1 : 0) +
                   (dst.Dimension ? (dst.DimIndirect ? 2 : 1) : 0);

   union tgsi_any_token *out = get_tokens( ureg, DOMAIN_INSN, size );
   unsigned n = 0;

   assert(dst.File != TGSI_FILE_NULL);
   assert(dst.File != TGSI_FILE_SAMPLER);
   assert(dst.File != TGSI_FILE_SAMPLER_VIEW);
   assert(dst.File != TGSI_FILE_IMMEDIATE);
   assert(dst.File < TGSI_FILE_COUNT);

   out[n].value = 0;
   out[n].dst.File = dst.File;
   out[n].dst.WriteMask = dst.WriteMask;
   out[n].dst.Indirect = dst.Indirect;
   out[n].dst.Index = dst.Index;
   n++;

   if (dst.Indirect) {
      out[n].value = 0;
      out[n].ind.File = dst.IndirectFile;
      out[n].ind.Swizzle = dst.IndirectSwizzle;
      out[n].ind.Index = dst.IndirectIndex;
      if (!ureg->supports_any_inout_decl_range &&
          (dst.File == TGSI_FILE_INPUT || dst.File == TGSI_FILE_OUTPUT))
         out[n].ind.ArrayID = 0;
      else
         out[n].ind.ArrayID = dst.ArrayID;
      n++;
   }

   if (dst.Dimension) {
      out[0].dst.Dimension = 1;
      out[n].dim.Dimension = 0;
      out[n].dim.Padding = 0;
      if (dst.DimIndirect) {
         out[n].dim.Indirect = 1;
         out[n].dim.Index = dst.DimensionIndex;
         n++;
         out[n].value = 0;
         out[n].ind.File = dst.DimIndFile;
         out[n].ind.Swizzle = dst.DimIndSwizzle;
         out[n].ind.Index = dst.DimIndIndex;
         if (!ureg->supports_any_inout_decl_range &&
             (dst.File == TGSI_FILE_INPUT || dst.File == TGSI_FILE_OUTPUT))
            out[n].ind.ArrayID = 0;
         else
            out[n].ind.ArrayID = dst.ArrayID;
      } else {
         out[n].dim.Indirect = 0;
         out[n].dim.Index = dst.DimensionIndex;
      }
      n++;
   }

   assert(n == size);
}
```


```c
void
ureg_emit_src( struct ureg_program *ureg,
               struct ureg_src src )
{
   unsigned size = 1 + (src.Indirect ? 1 : 0) +
                   (src.Dimension ? (src.DimIndirect ? 2 : 1) : 0);

   union tgsi_any_token *out = get_tokens( ureg, DOMAIN_INSN, size );
   unsigned n = 0;

   assert(src.File != TGSI_FILE_NULL);
   assert(src.File < TGSI_FILE_COUNT);

   out[n].value = 0;
   out[n].src.File = src.File;
   out[n].src.SwizzleX = src.SwizzleX;
   out[n].src.SwizzleY = src.SwizzleY;
   out[n].src.SwizzleZ = src.SwizzleZ;
   out[n].src.SwizzleW = src.SwizzleW;
   out[n].src.Index = src.Index;
   out[n].src.Negate = src.Negate;
   out[0].src.Absolute = src.Absolute;
   n++;

   if (src.Indirect) {
      out[0].src.Indirect = 1;
      out[n].value = 0;
      out[n].ind.File = src.IndirectFile;
      out[n].ind.Swizzle = src.IndirectSwizzle;
      out[n].ind.Index = src.IndirectIndex;
      if (!ureg->supports_any_inout_decl_range &&
          (src.File == TGSI_FILE_INPUT || src.File == TGSI_FILE_OUTPUT))
         out[n].ind.ArrayID = 0;
      else
         out[n].ind.ArrayID = src.ArrayID;
      n++;
   }

   if (src.Dimension) {
      out[0].src.Dimension = 1;
      out[n].dim.Dimension = 0;
      out[n].dim.Padding = 0;
      if (src.DimIndirect) {
         out[n].dim.Indirect = 1;
         out[n].dim.Index = src.DimensionIndex;
         n++;
         out[n].value = 0;
         out[n].ind.File = src.DimIndFile;
         out[n].ind.Swizzle = src.DimIndSwizzle;
         out[n].ind.Index = src.DimIndIndex;
         if (!ureg->supports_any_inout_decl_range &&
             (src.File == TGSI_FILE_INPUT || src.File == TGSI_FILE_OUTPUT))
            out[n].ind.ArrayID = 0;
         else
            out[n].ind.ArrayID = src.ArrayID;
      } else {
         out[n].dim.Indirect = 0;
         out[n].dim.Index = src.DimensionIndex;
      }
      n++;
   }

   assert(n == size);
}

```





宏内部处理完成后将tokens的信息写入ureg->domain[1].token中
之后ureg_finalize通过copy_instructions 将domain[1]的指令拷贝到domain[0]中， 最后生成完整的tgsi_tokens



通过ureg_Opcode的形式

```
#define OP00( op )                                              \
static inline void ureg_##op( struct ureg_program *ureg )       \
{                                                               \
   enum tgsi_opcode opcode = TGSI_OPCODE_##op;                  \
   struct ureg_emit_insn_result insn;                           \
   insn = ureg_emit_insn(ureg,                                  \
                         opcode,                                \
                         FALSE,                                 \
                         0,                                     \
                         0,                                     \
                         0);                                    \
   ureg_fixup_insn_size( ureg, insn.insn_token );               \
}

#define OP01( op )                                              \
static inline void ureg_##op( struct ureg_program *ureg,        \
                              struct ureg_src src )             \
{                                                               \
   enum tgsi_opcode opcode = TGSI_OPCODE_##op;                  \
   struct ureg_emit_insn_result insn;                           \
   insn = ureg_emit_insn(ureg,                                  \
                         opcode,                                \
                         FALSE,                                 \
                         0,                                     \
                         0,                                     \
                         1);                                    \
   ureg_emit_src( ureg, src );                                  \
   ureg_fixup_insn_size( ureg, insn.insn_token );               \
}

#define OP00_LBL( op )                                          \
static inline void ureg_##op( struct ureg_program *ureg,        \
                              unsigned *label_token )           \
{                                                               \
   enum tgsi_opcode opcode = TGSI_OPCODE_##op;                  \
   struct ureg_emit_insn_result insn;                           \
   insn = ureg_emit_insn(ureg,                                  \
                         opcode,                                \
                         FALSE,                                 \
                         0,                                     \
                         0,                                     \
                         0);                                    \
   ureg_emit_label( ureg, insn.extended_token, label_token );   \
   ureg_fixup_insn_size( ureg, insn.insn_token );               \
}

#define OP01_LBL( op )                                          \
static inline void ureg_##op( struct ureg_program *ureg,        \
                              struct ureg_src src,              \
                              unsigned *label_token )          \
{                                                               \
   enum tgsi_opcode opcode = TGSI_OPCODE_##op;                  \
   struct ureg_emit_insn_result insn;                           \
   insn = ureg_emit_insn(ureg,                                  \
                         opcode,                                \
                         FALSE,                                 \
                         0,                                     \
                         0,                                     \
                         1);                                    \
   ureg_emit_label( ureg, insn.extended_token, label_token );   \
   ureg_emit_src( ureg, src );                                  \
   ureg_fixup_insn_size( ureg, insn.insn_token );               \
}

#define OP10( op )                                                      \
static inline void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst )                     \
{                                                                       \
   enum tgsi_opcode opcode = TGSI_OPCODE_##op;                          \
   struct ureg_emit_insn_result insn;                                   \
   if (ureg_dst_is_empty(dst))                                          \
      return;                                                           \
   insn = ureg_emit_insn(ureg,                                          \
                         opcode,                                        \
                         dst.Saturate,                                  \
                         0,                                             \
                         1,                                             \
                         0);                                            \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_fixup_insn_size( ureg, insn.insn_token );                       \
}


#define OP11( op )                                                      \
static inline void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst,                      \
                              struct ureg_src src )                     \
{                                                                       \
   enum tgsi_opcode opcode = TGSI_OPCODE_##op;                          \
   struct ureg_emit_insn_result insn;                                   \
   if (ureg_dst_is_empty(dst))                                          \
      return;                                                           \
   insn = ureg_emit_insn(ureg,                                          \
                         opcode,                                        \
                         dst.Saturate,                                  \
                         0,                                             \
                         1,                                             \
                         1);                                            \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_emit_src( ureg, src );                                          \
   ureg_fixup_insn_size( ureg, insn.insn_token );                       \
}

#define OP12( op )                                                      \
static inline void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst,                      \
                              struct ureg_src src0,                     \
                              struct ureg_src src1 )                    \
{                                                                       \
   enum tgsi_opcode opcode = TGSI_OPCODE_##op;                          \
   struct ureg_emit_insn_result insn;                                   \
   if (ureg_dst_is_empty(dst))                                          \
      return;                                                           \
   insn = ureg_emit_insn(ureg,                                          \
                         opcode,                                        \
                         dst.Saturate,                                  \
                         0,                                             \
                         1,                                             \
                         2);                                            \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_emit_src( ureg, src0 );                                         \
   ureg_emit_src( ureg, src1 );                                         \
   ureg_fixup_insn_size( ureg, insn.insn_token );                       \
}

#define OP12_TEX( op )                                                  \
static inline void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst,                      \
                              enum tgsi_texture_type target,            \
                              struct ureg_src src0,                     \
                              struct ureg_src src1 )                    \
{                                                                       \
   enum tgsi_opcode opcode = TGSI_OPCODE_##op;                          \
   enum tgsi_return_type return_type = TGSI_RETURN_TYPE_UNKNOWN;        \
   struct ureg_emit_insn_result insn;                                   \
   if (ureg_dst_is_empty(dst))                                          \
      return;                                                           \
   insn = ureg_emit_insn(ureg,                                          \
                         opcode,                                        \
                         dst.Saturate,                                  \
                         0,                                             \
                         1,                                             \
                         2);                                            \
   ureg_emit_texture( ureg, insn.extended_token, target,                \
                      return_type, 0 );                                 \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_emit_src( ureg, src0 );                                         \
   ureg_emit_src( ureg, src1 );                                         \
   ureg_fixup_insn_size( ureg, insn.insn_token );                       \
}

#define OP13( op )                                                      \
static inline void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst,                      \
                              struct ureg_src src0,                     \
                              struct ureg_src src1,                     \
                              struct ureg_src src2 )                    \
{                                                                       \
   enum tgsi_opcode opcode = TGSI_OPCODE_##op;                          \
   struct ureg_emit_insn_result insn;                                   \
   if (ureg_dst_is_empty(dst))                                          \
      return;                                                           \
   insn = ureg_emit_insn(ureg,                                          \
                         opcode,                                        \
                         dst.Saturate,                                  \
                         0,                                             \
                         1,                                             \
                         3);                                            \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_emit_src( ureg, src0 );                                         \
   ureg_emit_src( ureg, src1 );                                         \
   ureg_emit_src( ureg, src2 );                                         \
   ureg_fixup_insn_size( ureg, insn.insn_token );                       \
}

#define OP14_TEX( op )                                                  \
static inline void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst,                      \
                              enum tgsi_texture_type target,            \
                              struct ureg_src src0,                     \
                              struct ureg_src src1,                     \
                              struct ureg_src src2,                     \
                              struct ureg_src src3 )                    \
{                                                                       \
   enum tgsi_opcode opcode = TGSI_OPCODE_##op;                          \
   enum tgsi_return_type return_type = TGSI_RETURN_TYPE_UNKNOWN;        \
   struct ureg_emit_insn_result insn;                                   \
   if (ureg_dst_is_empty(dst))                                          \
      return;                                                           \
   insn = ureg_emit_insn(ureg,                                          \
                         opcode,                                        \
                         dst.Saturate,                                  \
                         0,                                             \
                         1,                                             \
                         4);                                            \
   ureg_emit_texture( ureg, insn.extended_token, target,                \
                      return_type, 0 );                                 \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_emit_src( ureg, src0 );                                         \
   ureg_emit_src( ureg, src1 );                                         \
   ureg_emit_src( ureg, src2 );                                         \
   ureg_emit_src( ureg, src3 );                                         \
   ureg_fixup_insn_size( ureg, insn.insn_token );                       \
}

```







```c
/**
 * Plug in the program and shader-related device driver functions.
 */
void
st_init_program_functions(struct dd_function_table *functions)
{
   functions->NewProgram = st_new_program;
   functions->DeleteProgram = st_delete_program;
   functions->ProgramStringNotify = st_program_string_notify;
   functions->NewATIfs = st_new_ati_fs;
   
   functions->LinkShader = st_link_shader;
}

```
st_vertex_program.Base-> gl_program

## tgsi_tokens的生成

```c
const struct tgsi_token *ureg_finalize( struct ureg_program *ureg )
{
   const struct tgsi_token *tokens;

   switch (ureg->processor) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_TESS_EVAL:
      ureg_property(ureg, TGSI_PROPERTY_NEXT_SHADER,
                    ureg->next_shader_processor == -1 ?
                       PIPE_SHADER_FRAGMENT :
                       ureg->next_shader_processor);
      break;
   default:
      ; /* nothing */
   }

   emit_header( ureg );
   emit_decls( ureg );
   copy_instructions( ureg );
        memcpy(out,
               ureg->domain[DOMAIN_INSN].tokens,
               nr_tokens * sizeof out[0] );
   fixup_header_size( ureg );

   if (ureg->domain[0].tokens == error_tokens ||
       ureg->domain[1].tokens == error_tokens) {
      debug_printf("%s: error in generated shader\n", __FUNCTION__);
      assert(0);
      return NULL;
   }

   tokens = &ureg->domain[DOMAIN_DECL].tokens[0].token;

   if (0) {
      debug_printf("%s: emitted shader %d tokens:\n", __FUNCTION__,
                   ureg->domain[DOMAIN_DECL].count);
      tgsi_dump( tokens, 0 );
   }

#if DEBUG
   /* tgsi_sanity doesn't seem to return if there are too many constants. */
   bool too_many_constants = false;
   for (unsigned i = 0; i < ARRAY_SIZE(ureg->const_decls); i++) {
      for (unsigned j = 0; j < ureg->const_decls[i].nr_constant_ranges; j++) {
         if (ureg->const_decls[i].constant_range[j].last > 4096) {
            too_many_constants = true;
            break;
         }
      }
   }

   if (tokens && !too_many_constants && !tgsi_sanity_check(tokens)) {
      debug_printf("tgsi_ureg.c, sanity check failed on generated tokens:\n");
      tgsi_dump(tokens, 0);
      assert(0);
   }
#endif


   return tokens;
}

```
* 通过copy_instructions 将操作指令也就是main函数主体的tokens从 domain[DOMAIN_INSN]拷贝到domain[DOMAIN_DECL]中

## 转tgsi tokens对 声明变量的处理

该接口遍历ureg_program的声明描述，保存到ureg->domain中的ureg_tokens中
根据声明的类型不同分为 输入输出,纹理图像,缓冲，原子，临时变量，立即数

```c
static void emit_decls( struct ureg_program *ureg )
{
   unsigned i,j;

   for (i = 0; i < ARRAY_SIZE(ureg->properties); i++)
      if (ureg->properties[i] != ~0u)
         emit_property(ureg, i, ureg->properties[i]);

   if (ureg->processor == PIPE_SHADER_VERTEX) {
      for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
         if (ureg->vs_inputs[i/32] & (1u << (i%32))) {
            emit_decl_range( ureg, TGSI_FILE_INPUT, i, 1 );
         }
      }
   } else if (ureg->processor == PIPE_SHADER_FRAGMENT) {
      if (ureg->supports_any_inout_decl_range) {
         for (i = 0; i < ureg->nr_inputs; i++) {
            emit_decl_fs(ureg,
                         TGSI_FILE_INPUT,
                         ureg->input[i].first,
                         ureg->input[i].last,
                         ureg->input[i].semantic_name,
                         ureg->input[i].semantic_index,
                         ureg->input[i].interp,
                         ureg->input[i].cylindrical_wrap,
                         ureg->input[i].interp_location,
                         ureg->input[i].array_id,
                         ureg->input[i].usage_mask);
         }
      }
      else {
         for (i = 0; i < ureg->nr_inputs; i++) {
            for (j = ureg->input[i].first; j <= ureg->input[i].last; j++) {
               emit_decl_fs(ureg,
                            TGSI_FILE_INPUT,
                            j, j,
                            ureg->input[i].semantic_name,
                            ureg->input[i].semantic_index +
                            (j - ureg->input[i].first),
                            ureg->input[i].interp,
                            ureg->input[i].cylindrical_wrap,
                            ureg->input[i].interp_location, 0,
                            ureg->input[i].usage_mask);
            }
         }
      }
   } else {
      if (ureg->supports_any_inout_decl_range) {
         for (i = 0; i < ureg->nr_inputs; i++) {
            emit_decl_semantic(ureg,
                               TGSI_FILE_INPUT,
                               ureg->input[i].first,
                               ureg->input[i].last,
                               ureg->input[i].semantic_name,
                               ureg->input[i].semantic_index,
                               0,
                               TGSI_WRITEMASK_XYZW,
                               ureg->input[i].array_id,
                               FALSE);
         }
      }
      else {
         for (i = 0; i < ureg->nr_inputs; i++) {
            for (j = ureg->input[i].first; j <= ureg->input[i].last; j++) {
               emit_decl_semantic(ureg,
                                  TGSI_FILE_INPUT,
                                  j, j,
                                  ureg->input[i].semantic_name,
                                  ureg->input[i].semantic_index +
                                  (j - ureg->input[i].first),
                                  0,
                                  TGSI_WRITEMASK_XYZW, 0, FALSE);
            }
         }
      }
   }

   for (i = 0; i < ureg->nr_system_values; i++) {
      emit_decl_semantic(ureg,
                         TGSI_FILE_SYSTEM_VALUE,
                         i,
                         i,
                         ureg->system_value[i].semantic_name,
                         ureg->system_value[i].semantic_index,
                         0,
                         TGSI_WRITEMASK_XYZW, 0, FALSE);
   }

   if (ureg->supports_any_inout_decl_range) {
      for (i = 0; i < ureg->nr_outputs; i++) {
         emit_decl_semantic(ureg,
                            TGSI_FILE_OUTPUT,
                            ureg->output[i].first,
                            ureg->output[i].last,
                            ureg->output[i].semantic_name,
                            ureg->output[i].semantic_index,
                            ureg->output[i].streams,
                            ureg->output[i].usage_mask,
                            ureg->output[i].array_id,
                            ureg->output[i].invariant);
      }
   }
   else {
      for (i = 0; i < ureg->nr_outputs; i++) {
         for (j = ureg->output[i].first; j <= ureg->output[i].last; j++) {
            emit_decl_semantic(ureg,
                               TGSI_FILE_OUTPUT,
                               j, j,
                               ureg->output[i].semantic_name,
                               ureg->output[i].semantic_index +
                               (j - ureg->output[i].first),
                               ureg->output[i].streams,
                               ureg->output[i].usage_mask,
                               0,
                               ureg->output[i].invariant);
         }
      }
   }

   for (i = 0; i < ureg->nr_samplers; i++) {
      emit_decl_range( ureg,
                       TGSI_FILE_SAMPLER,
                       ureg->sampler[i].Index, 1 );
   }

   for (i = 0; i < ureg->nr_sampler_views; i++) {
      emit_decl_sampler_view(ureg,
                             ureg->sampler_view[i].index,
                             ureg->sampler_view[i].target,
                             ureg->sampler_view[i].return_type_x,
                             ureg->sampler_view[i].return_type_y,
                             ureg->sampler_view[i].return_type_z,
                             ureg->sampler_view[i].return_type_w);
   }

   for (i = 0; i < ureg->nr_images; i++) {
      emit_decl_image(ureg,
                      ureg->image[i].index,
                      ureg->image[i].target,
                      ureg->image[i].format,
                      ureg->image[i].wr,
                      ureg->image[i].raw);
   }

   for (i = 0; i < ureg->nr_buffers; i++) {
      emit_decl_buffer(ureg, ureg->buffer[i].index, ureg->buffer[i].atomic);
   }

   for (i = 0; i < TGSI_MEMORY_TYPE_COUNT; i++) {
      if (ureg->use_memory[i])
         emit_decl_memory(ureg, i);
   }

   for (i = 0; i < PIPE_MAX_CONSTANT_BUFFERS; i++) {
      struct const_decl *decl = &ureg->const_decls[i];

      if (decl->nr_constant_ranges) {
         uint j;

         for (j = 0; j < decl->nr_constant_ranges; j++) {
            emit_decl_range2D(ureg,
                              TGSI_FILE_CONSTANT,
                              decl->constant_range[j].first,
                              decl->constant_range[j].last,
                              i);
         }
      }
   }

   for (i = 0; i < PIPE_MAX_HW_ATOMIC_BUFFERS; i++) {
      struct hw_atomic_decl *decl = &ureg->hw_atomic_decls[i];

      if (decl->nr_hw_atomic_ranges) {
         uint j;

         for (j = 0; j < decl->nr_hw_atomic_ranges; j++) {
            emit_decl_atomic_2d(ureg,
                                decl->hw_atomic_range[j].first,
                                decl->hw_atomic_range[j].last,
                                i,
                                decl->hw_atomic_range[j].array_id);
         }
      }
   }

   if (ureg->nr_temps) {
      unsigned array = 0;
      for (i = 0; i < ureg->nr_temps;) {
         boolean local = util_bitmask_get(ureg->local_temps, i);
         unsigned first = i;
         i = util_bitmask_get_next_index(ureg->decl_temps, i + 1);
         if (i == UTIL_BITMASK_INVALID_INDEX)
            i = ureg->nr_temps;

         if (array < ureg->nr_array_temps && ureg->array_temps[array] == first)
            emit_decl_temps( ureg, first, i - 1, local, ++array );
         else
            emit_decl_temps( ureg, first, i - 1, local, 0 );
      }
   }

   if (ureg->nr_addrs) {
      emit_decl_range( ureg,
                       TGSI_FILE_ADDRESS,
                       0, ureg->nr_addrs );
   }

   for (i = 0; i < ureg->nr_immediates; i++) {
      emit_immediate( ureg,
                      ureg->immediate[i].value.u,
                      ureg->immediate[i].type );
   }
}

```

### 处理输入

#### vs 特殊处理

```
            emit_decl_range( ureg, TGSI_FILE_INPUT, i, 1 );

```

#### fs 特殊处理

```c
static void
emit_decl_fs(struct ureg_program *ureg,
             unsigned file,
             unsigned first,
             unsigned last,
             enum tgsi_semantic semantic_name,
             unsigned semantic_index,
             enum tgsi_interpolate_mode interpolate,
             unsigned cylindrical_wrap,
             enum tgsi_interpolate_loc interpolate_location,
             unsigned array_id,
             unsigned usage_mask)
{
   union tgsi_any_token *out = get_tokens(ureg, DOMAIN_DECL,
                                          array_id ? 5 : 4);

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 4;
   out[0].decl.File = file;
   out[0].decl.UsageMask = usage_mask;
   out[0].decl.Interpolate = 1;
   out[0].decl.Semantic = 1;
   out[0].decl.Array = array_id != 0;

   out[1].value = 0;
   out[1].decl_range.First = first;
   out[1].decl_range.Last = last;

   out[2].value = 0;
   out[2].decl_interp.Interpolate = interpolate;
   out[2].decl_interp.CylindricalWrap = cylindrical_wrap;
   out[2].decl_interp.Location = interpolate_location;

   out[3].value = 0;
   out[3].decl_semantic.Name = semantic_name;
   out[3].decl_semantic.Index = semantic_index;

   if (array_id) {
      out[4].value = 0;
      out[4].array.ArrayID = array_id;
   }
}
```

#### 其他shader的处理

```c
static void
emit_decl_semantic(struct ureg_program *ureg,
                   unsigned file,
                   unsigned first,
                   unsigned last,
                   enum tgsi_semantic semantic_name,
                   unsigned semantic_index,
                   unsigned streams,
                   unsigned usage_mask,
                   unsigned array_id,
                   boolean invariant)
{
   union tgsi_any_token *out = get_tokens(ureg, DOMAIN_DECL, array_id ? 4 : 3);

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 3;
   out[0].decl.File = file;
   out[0].decl.UsageMask = usage_mask;
   out[0].decl.Semantic = 1;
   out[0].decl.Array = array_id != 0;
   out[0].decl.Invariant = invariant;

   out[1].value = 0;
   out[1].decl_range.First = first;
   out[1].decl_range.Last = last;

   out[2].value = 0;
   out[2].decl_semantic.Name = semantic_name;
   out[2].decl_semantic.Index = semantic_index;
   out[2].decl_semantic.StreamX = streams & 3;
   out[2].decl_semantic.StreamY = (streams >> 2) & 3;
   out[2].decl_semantic.StreamZ = (streams >> 4) & 3;
   out[2].decl_semantic.StreamW = (streams >> 6) & 3;

   if (array_id) {
      out[3].value = 0;
      out[3].array.ArrayID = array_id;
   }
}



```
### 处理系统变量/内建变量

```c
static void
emit_decl_semantic(struct ureg_program *ureg,
                   unsigned file,
                   unsigned first,
                   unsigned last,
                   enum tgsi_semantic semantic_name,
                   unsigned semantic_index,
                   unsigned streams,
                   unsigned usage_mask,
                   unsigned array_id,
                   boolean invariant)
{
   union tgsi_any_token *out = get_tokens(ureg, DOMAIN_DECL, array_id ? 4 : 3);

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 3;
   out[0].decl.File = file;
   out[0].decl.UsageMask = usage_mask;
   out[0].decl.Semantic = 1;
   out[0].decl.Array = array_id != 0;
   out[0].decl.Invariant = invariant;

   out[1].value = 0;
   out[1].decl_range.First = first;
   out[1].decl_range.Last = last;

   out[2].value = 0;
   out[2].decl_semantic.Name = semantic_name;
   out[2].decl_semantic.Index = semantic_index;
   out[2].decl_semantic.StreamX = streams & 3;
   out[2].decl_semantic.StreamY = (streams >> 2) & 3;
   out[2].decl_semantic.StreamZ = (streams >> 4) & 3;
   out[2].decl_semantic.StreamW = (streams >> 6) & 3;

   if (array_id) {
      out[3].value = 0;
      out[3].array.ArrayID = array_id;
   }
}
```

### 处理输出

emit_decl_semantic(ureg,

### 处理buffer

```c
static void
emit_decl_buffer(struct ureg_program *ureg,
                 unsigned index,
                 bool atomic)
{
   union tgsi_any_token *out = get_tokens(ureg, DOMAIN_DECL, 2);

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 2;
   out[0].decl.File = TGSI_FILE_BUFFER;
   out[0].decl.UsageMask = TGSI_WRITEMASK_XYZW;
   out[0].decl.Atomic = atomic;

   out[1].value = 0;
   out[1].decl_range.First = index;
   out[1].decl_range.Last = index;
}

```


### 处理图像

```c
static void
emit_decl_image(struct ureg_program *ureg,
                unsigned index,
                enum tgsi_texture_type target,
                enum pipe_format format,
                boolean wr,
                boolean raw)
{
   union tgsi_any_token *out = get_tokens(ureg, DOMAIN_DECL, 3);

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 3;
   out[0].decl.File = TGSI_FILE_IMAGE;
   out[0].decl.UsageMask = TGSI_WRITEMASK_XYZW;

   out[1].value = 0;
   out[1].decl_range.First = index;
   out[1].decl_range.Last = index;

   out[2].value = 0;
   out[2].decl_image.Resource = target;
   out[2].decl_image.Writable = wr;
   out[2].decl_image.Raw      = raw;
   out[2].decl_image.Format   = format;
}



```

### 处理SAMPLE

```c

      emit_decl_range( ureg,
                       TGSI_FILE_SAMPLER,
                       ureg->sampler[i].Index, 1 );


```
### 处理纹理 SAMPLER_VIEW

```c
static void
emit_decl_sampler_view(struct ureg_program *ureg,
                       unsigned index,
                       enum tgsi_texture_type target,
                       enum tgsi_return_type return_type_x,
                       enum tgsi_return_type return_type_y,
                       enum tgsi_return_type return_type_z,
                       enum tgsi_return_type return_type_w )
{
   union tgsi_any_token *out = get_tokens(ureg, DOMAIN_DECL, 3);

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 3;
   out[0].decl.File = TGSI_FILE_SAMPLER_VIEW;
   out[0].decl.UsageMask = TGSI_WRITEMASK_XYZW;

   out[1].value = 0;
   out[1].decl_range.First = index;
   out[1].decl_range.Last = index;

   out[2].value = 0;
   out[2].decl_sampler_view.Resource    = target;
   out[2].decl_sampler_view.ReturnTypeX = return_type_x;
   out[2].decl_sampler_view.ReturnTypeY = return_type_y;
   out[2].decl_sampler_view.ReturnTypeZ = return_type_z;
   out[2].decl_sampler_view.ReturnTypeW = return_type_w;
}

```
### 处理常量


```c

static void
emit_decl_range2D(struct ureg_program *ureg,
                  unsigned file,
                  unsigned first,
                  unsigned last,
                  unsigned index2D)
{
   union tgsi_any_token *out = get_tokens(ureg, DOMAIN_DECL, 3);

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 3;
   out[0].decl.File = file;
   out[0].decl.UsageMask = TGSI_WRITEMASK_XYZW;
   out[0].decl.Dimension = 1;

   out[1].value = 0;
   out[1].decl_range.First = first;
   out[1].decl_range.Last = last;

   out[2].value = 0;
   out[2].decl_dim.Index2D = index2D;
}

```
### 处理原子


```c
static void
emit_decl_atomic_2d(struct ureg_program *ureg,
                    unsigned first,
                    unsigned last,
                    unsigned index2D,
                    unsigned array_id)
{
   union tgsi_any_token *out = get_tokens(ureg, DOMAIN_DECL, array_id ? 4 : 3);

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 3;
   out[0].decl.File = TGSI_FILE_HW_ATOMIC;
   out[0].decl.UsageMask = TGSI_WRITEMASK_XYZW;
   out[0].decl.Dimension = 1;
   out[0].decl.Array = array_id != 0;

   out[1].value = 0;
   out[1].decl_range.First = first;
   out[1].decl_range.Last = last;

   out[2].value = 0;
   out[2].decl_dim.Index2D = index2D;

   if (array_id) {
      out[3].value = 0;
      out[3].array.ArrayID = array_id;
   }
}
```


### 处理临时变量

```c

static void
emit_decl_temps( struct ureg_program *ureg,
                 unsigned first, unsigned last,
                 boolean local,
                 unsigned arrayid )
{
   union tgsi_any_token *out = get_tokens( ureg, DOMAIN_DECL,
                                           arrayid ? 3 : 2 );

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 2;
   out[0].decl.File = TGSI_FILE_TEMPORARY;
   out[0].decl.UsageMask = TGSI_WRITEMASK_XYZW;
   out[0].decl.Local = local;

   out[1].value = 0;
   out[1].decl_range.First = first;
   out[1].decl_range.Last = last;

   if (arrayid) {
      out[0].decl.Array = 1;
      out[2].value = 0;
      out[2].array.ArrayID = arrayid;
   }
}




```

### 处理 TGSI_FILE_ADDRESS

```c

      emit_decl_range( ureg,

```


### 处理立即数

```c
static void
emit_immediate( struct ureg_program *ureg,
                const unsigned *v,
                unsigned type )
{
   union tgsi_any_token *out = get_tokens( ureg, DOMAIN_DECL, 5 );

   out[0].value = 0;
   out[0].imm.Type = TGSI_TOKEN_TYPE_IMMEDIATE;
   out[0].imm.NrTokens = 5;
   out[0].imm.DataType = type;
   out[0].imm.Padding = 0;

   out[1].imm_data.Uint = v[0];
   out[2].imm_data.Uint = v[1];
   out[3].imm_data.Uint = v[2];
   out[4].imm_data.Uint = v[3];
}

```

### 处理内存TGSI_FILE_MEMORY

```c
static void
emit_decl_memory(struct ureg_program *ureg, unsigned memory_type)
{
   union tgsi_any_token *out = get_tokens(ureg, DOMAIN_DECL, 2);

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 2;
   out[0].decl.File = TGSI_FILE_MEMORY;
   out[0].decl.UsageMask = TGSI_WRITEMASK_XYZW;
   out[0].decl.MemType = memory_type;

   out[1].value = 0;
   out[1].decl_range.First = memory_type;
   out[1].decl_range.Last = memory_type;
}


```
## st_vertex_program 生成

```
/**
 * Called via ctx->Driver.NewProgram() to allocate a new vertex or
 * fragment program.
 */
static struct gl_program *
st_new_program(struct gl_context *ctx, GLenum target, GLuint id,
               bool is_arb_asm)
{
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: {
      struct st_vertex_program *prog = rzalloc(NULL,
                                               struct st_vertex_program);
      return _mesa_init_gl_program(&prog->Base, target, id, is_arb_asm);
   }
 

}
```
## gl_linked_shader.ir 生成

_mesa_ir_link_shader(struct gl_context *ctx, struct gl_shader_program *prog)
{

   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
}

## glsl_to_tgsi_visitor* v;

```
GLboolean
st_link_shader(struct gl_context *ctx, struct gl_shader_program *prog)

   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *shader = prog->_LinkedShaders[i];
      if (shader == NULL)
         continue;

      struct gl_program *linked_prog =
         get_mesa_program_tgsi(ctx, prog, shader);
      st_set_prog_affected_state_flags(linked_prog);

      if (linked_prog) {
         if (!ctx->Driver.ProgramStringNotify(ctx,
                                              _mesa_shader_stage_to_program(i),
                                              linked_prog)) {
            _mesa_reference_program(ctx, &shader->Program, NULL);
            return GL_FALSE;
         }
      }
   }


// 转化glsl ir to gl_program(st_vertex_program,
get_mesa_program_tgsi(struct gl_context *ctx,
                      struct gl_shader_program *shader_program,
                      struct gl_linked_shader *shader)

   // 
   gl_program* prog = shader->Program;
   glsl_to_tgsi_visitor* v;

   v = new glsl_to_tgsi_visitor();

   v->ctx = ctx;
   v->prog = prog;

   _mesa_generate_parameters_list_for_uniforms(ctx, shader_program, shader,
                                               prog->Parameters);
   
   /* Emit intermediate IR for main(). */
   visit_exec_list(shader->ir, v);

void
visit_exec_list(exec_list *list, ir_visitor *visitor)
{
   foreach_in_list_safe(ir_instruction, node, list) {
      node->accept(visitor);
   }
}

```
struct exec_node {
   struct exec_node *next;
   struct exec_node *prev;
}


class ir_instruction : public exec_node {
   virtual void accept(ir_visitor *) = 0;

## 

```

st_program_string_notify( struct gl_context *ctx,
                                           GLenum target,
                                           struct gl_program *prog )

   ...

   else if (target == GL_VERTEX_PROGRAM_ARB) {
      struct st_vertex_program *stvp = (struct st_vertex_program *) prog;

      st_release_vp_variants(st, stvp);
      if (!st_translate_vertex_program(st, stvp))
         return false;

      if (st->vp == stvp)
	 st->dirty |= ST_NEW_VERTEX_PROGRAM(st, stvp);

   }
```

* prog
## glsl_to_tgsi_visitor 生成


st_translate_vertex_program(struct st_context *st,
                            struct st_vertex_program *stvp)

   struct ureg_program *ureg;

    
   /*
    * 确定输入的数量，VERT_ATTRIB_x与TGSI通用输入索引之间的映射，以及输入属性语义信息。
    */
   for (attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
       if ((stvp->Base.info.inputs_read & BITFIELD64_BIT(attr)) != 0) {
          stvp->input_to_index[attr] = stvp->num_inputs;
          stvp->index_to_input[stvp->num_inputs] = attr;
          stvp->num_inputs++;
          if ((stvp->Base.DualSlotInputs & BITFIELD64_BIT(attr)) != 0) {
             /* add placeholder for second part of a double attribute */
             stvp->index_to_input[stvp->num_inputs] = ST_DOUBLE_ATTRIB_PLACEHOLDER;
             stvp->num_inputs++;
          }
       }
   }

## IR-reg TGSI format

/**
 * 将中间表示（glsl_to_tgsi_instruction）翻译成TGSI格式。
 * \param ctx  GL上下文
 * \param procType  着色器程序的类型（顶点，片段等）
 * \param ureg  指向TGSI程序构建器的指针
 * \param program  GLSL到TGSI的访问者
 * \param proginfo  GL程序的信息
 * \param numInputs  使用的输入寄存器数量
 * \param inputMapping  将Mesa片段程序的输入映射到TGSI通用输入索引
 * \param inputSlotToAttr  将输入槽映射到属性
 * \param inputSemanticName  每个输入的TGSI_SEMANTIC标志
 * \param inputSemanticIndex  每个输入的语义索引（例如：哪个纹理坐标）
 * \param interpMode  每个输入的TGSI_INTERPOLATE_LINEAR/PERSP模式
 * \param numOutputs  使用的输出寄存器数量
 * \param outputMapping  将Mesa片段程序的输出映射到TGSI通用输出
 * \param outputSemanticName  每个输出的TGSI_SEMANTIC标志
 * \param outputSemanticIndex  每个输出的语义索引（例如：哪个纹理坐标）
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
   const ubyte outputMapping[],
   const ubyte outputSemanticName[],
   const ubyte outputSemanticIndex[]);

    
    // fs, gs, tes, tcs 声明属性处理
    for (i = 0; i < program->num_inputs; ++i) {


}


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
} 


* ureg->processor
* ureg->supports_any_inout_decl_range

```
ureg_create_with_screen(enum pipe_shader_type processor,
                        struct pipe_screen *screen)
   ureg->processor = processor;
   ureg->supports_any_inout_decl_range =
      screen &&
      screen->get_shader_param(screen, processor,
                               PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE) != 0;
```
