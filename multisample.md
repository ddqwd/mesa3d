

# OpenGL 4.5 core

## 14.3.1 多重采样

多重采样是一种抗锯齿所有 GL 原语（点、线和多边形）的机制。该技术是在每个像素处多次采样所有原语。颜色采样值在每次更新像素时被解析为单一的、可显示的颜色，因此抗锯齿在应用程序级别上似乎是自动的。
因为每个样本都包括颜色、深度和模板信息，所以颜色（包括纹理操作）、深度和模板函数在多重采样模式下的表现与单样本模式相当。
**在帧缓冲中添加了一个名为多重采样缓冲的附加缓冲区。** 像素样本值，包括颜色、深度和模板值，被存储在这个缓冲区中。样本包含每个片段颜色的单独颜色值。**当帧缓冲包括一个多重采样缓冲时，即使多重采样缓冲不存储深度或模板值，帧缓冲也不包括深度或模板缓冲。然而，颜色缓冲与多重采样缓冲共存。**
对于渲染多边形，多重采样抗锯齿是最有价值的，因为它对于隐藏表面消除不需要排序，并且能够正确处理相邻多边形、对象轮廓甚至相交多边形。如果只渲染线条，则基本 GL 提供的“平滑”抗锯齿机制可能会导致更高质量的图像。该机制旨在允许在单个场景的渲染过程中交替使用多重采样和平滑抗锯齿技术。
如果 SAMPLE_BUFFERS 的值为 1（参见第 9.2.3.1 节），则所有原语的光栅化都会改变，称为多重采样光栅化。否则，原语光栅化称为单样本光栅化。
在多重采样渲染期间，像素片段的内容会以两种方式发生变化。首先，每个片段包括一个覆盖值，其值为 SAMPLES 位（参见第 9.2.3.1 节）。
为了查询给定样本的阴影采样位置（阴影采样位置），使用以下命令： 

 
 ```c
void GetMultisamplefv(enum pname, uint index, float *val);
```

函数 `GetMultisamplefv` 用于获取多重采样的信息，其中参数 `pname` 必须为 `SAMPLE_POSITION`，而 `index` 对应于要返回其位置的采样点。采样位置以两个浮点值的形式返回，分别存储在 `val[0]` 和 `val[1]` 中，它们的取值范围在 0 到 1 之间，对应于在 GL 像素空间中该采样点的 x 和 y 位置。因此，(0.5, 0.5) 对应于像素中心。如果多重采样模式没有固定的采样位置，返回的值可能只反映某些像素内部的样本位置。

错误处理：
- 如果 `pname` 不是 `SAMPLE_POSITION`，则会生成 `INVALID_ENUM` 错误。
- 如果 `index` 大于或等于 `SAMPLES` 的值，则会生成 `INVALID_VALUE` 错误。

**其次，每个片段包含 `SAMPLES` 个深度值和相关数据集，**而不是在单样本渲染模式下维护的单个深度值和相关数据集。实现可以选择为多个采样点分配相同的相关数据。用于评估这种相关数据的位置可以位于像素内的任何位置，包括片段中心或任何采样点位置。不同的相关数据值不必在相同的位置进行评估。因此，每个像素片段由整数 x 和 y 网格坐标、`SAMPLES` 个深度值和相关数据集以及最多 `SAMPLES` 位的覆盖值组成。

通过调用 `Enable` 或 `Disable` 并将目标设置为 `MULTISAMPLE`，可以启用或禁用多重采样光栅化。如果禁用了 `MULTISAMPLE`，则所有原语的多重采样光栅化等效于单样本（片段中心）光栅化，只是片段的覆盖值被设置为完全覆盖。颜色和深度值以及纹理坐标的集合都可以设置为单样本光栅化时将被分配的值，或者可以按照下面描述的方式为多重采样光栅化分配。

如果启用了 `MULTISAMPLE`，则所有原语的多重采样光栅化与单样本光栅化有很大的区别。理解到帧缓冲中的每个像素都与之关联的采样点。这些位置是精确位置，而不是区域或区域，并且每个位置都称为采样点。这些采样点不一定对应于由 `GetMultisamplefv` 返回的着色采样位置。它们的位置无法查询，并且可能位于视为限制像素的单位正方形内或外。这些样本的数量可能与 `SAMPLES` 的值不同。此外，采样点的相对位置可能对于帧缓冲中的每个像素都相同，也可能不同。



# gl

```c
/**
 * Multisample attribute group (GL_MULTISAMPLE_BIT).
 */
struct gl_multisample_attrib
{
    // Enable 控制
   GLboolean Enabled;

   GLboolean SampleAlphaToCoverage;

   GLboolean SampleAlphaToOne;

   GLboolean SampleCoverage;
   GLboolean SampleCoverageInvert;
   GLboolean SampleShading;

   /* ARB_texture_multisample / GL3.2 additions */
   GLboolean SampleMask;

   GLfloat SampleCoverageValue;  /**< In range [0, 1] */
   GLfloat MinSampleShadingValue;  /**< In range [0, 1] */

   /** The GL spec defines this as an array but >32x MSAA is madness */
   GLbitfield SampleMaskValue;

   /* NV_alpha_to_coverage_dither_control */
   GLenum SampleAlphaToCoverageDitherControl;
};

```


# state_tracker


```c
void
st_update_rasterizer(struct st_context *st)
{
   struct gl_context *ctx = st->ctx;
   struct pipe_rasterizer_state *raster = &st->state.rasterizer;
   const struct gl_program *fragProg = ctx->FragmentProgram._Current;

   memset(raster, 0, sizeof(*raster));

   /* _NEW_MULTISAMPLE */
   bool multisample = _mesa_is_multisample_enabled(ctx);
   raster->multisample = multisample;

    cso_set_rasterizer(st->cso_context, raster);
}
```

```
bool _mesa_is_multisample_enabled(const struct gl_context *ctx)
{
   /* 多重采样样本数可能未被驱动程序验证，但当它被设置时，
    * 我们知道它在有效范围内，驱动程序不应该验证多重采样
    * 帧缓冲为非多重采样，反之亦然。
    */
   return ctx->Multisample.Enabled &&
          ctx->DrawBuffer &&
          _mesa_geometric_nonvalidated_samples(ctx->DrawBuffer) >= 1;
}
```



# GetMultisamplefv

```
void GLAPIENTRY
_mesa_GetMultisamplefv(GLenum pname, GLuint index, GLfloat * val)
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->NewState & _NEW_BUFFERS) {
      _mesa_update_state(ctx);
   }

   switch (pname) {
   case GL_SAMPLE_POSITION: {
      if (index >= ctx->DrawBuffer->Visual.samples) {
         _mesa_error( ctx, GL_INVALID_VALUE, "glGetMultisamplefv(index)" );
         return;
      }

      get_sample_position(ctx, ctx->DrawBuffer, index, val);

      /* FBOs can be upside down (winsys always are)*/
      if (ctx->DrawBuffer->FlipY)
         val[1] = 1.0f - val[1];

      return;
   }

    // 
   case GL_PROGRAMMABLE_SAMPLE_LOCATION_ARB:
      if (!ctx->Extensions.ARB_sample_locations) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetMultisamplefv(pname)" );
         return;
      }

      if (index >= MAX_SAMPLE_LOCATION_TABLE_SIZE * 2) {
         _mesa_error( ctx, GL_INVALID_VALUE, "glGetMultisamplefv(index)" );
         return;
      }

      if (ctx->DrawBuffer->SampleLocationTable)
         *val = ctx->DrawBuffer->SampleLocationTable[index];
      else
         *val = 0.5f;

      return;

   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetMultisamplefv(pname)" );
      return;
   }
}


```


# llvmpipe 


```c

const float lp_sample_pos_4x[4][2] = { { 0.375, 0.125 },
                                       { 0.875, 0.375 },
                                       { 0.125, 0.625 },
                                       { 0.625, 0.875 } };


   lp->pipe.get_sample_position = llvmpipe_get_sample_position;

```
