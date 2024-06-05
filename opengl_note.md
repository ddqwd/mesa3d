
# gl_FragDepth

[gl_FragDepth ](https://www.khronos.org/opengl/wiki/Fragment_Shader/Defined_Outputs)

# sample mask

样本掩k
在图形编程中，PS（Pixel Shader，像素着色器）的 sample mask（样本掩码）是一个掩码值，用于确定哪些多重采样样本点将在像素着色器中执行，以及哪些将被忽略。

多重采样是一种用于抗锯齿的技术，它在每个像素上使用多个采样点来确定最终的像素颜色值。sample mask 用于控制哪些采样点将在像素着色器中运行。sample mask 是一个位掩码，其中每个位对应于一个采样点，如果位被设置为1，则表示对应的采样点将在像素着色器中执行，如果位被设置为0，则表示该采样点将被忽略。

通常，sample mask 的设置和使用如下：

1. 设置 sample mask 值：在图形编程中，可以使用图形API（如OpenGL或DirectX）的函数来设置 sample mask 的值。这通常是在渲染管线的配置阶段完成的。

2. 在像素着色器中使用 sample mask：在像素着色器中，可以使用 sample mask 的值来确定哪些采样点将执行像素着色器代码。通常，像素着色器的代码会被执行多次，每次都使用 sample mask 决定执行的采样点。

3. 组合结果：像素着色器的多次执行的结果将根据 sample mask 中的位值进行组合，以生成最终的像素颜色。

使用 sample mask 可以实现一些高级的抗锯齿和渲染效果，以及控制哪些采样点对最终的像素颜色有影响。这对于优化渲染性能和图像质量非常有用。不同的图形编程框架和硬件可能具有不同的方式来设置和使用 sample mask。

 
在OpenGL中，你可以使用以下API来设置 sample mask：

1. **`glSampleMaski`**：这是OpenGL 3.3及更高版本中的函数。它允许你设置特定的样本掩码值，其中`i`表示样本掩码的索引。你可以通过调用`glSampleMaski`函数来设置每个索引处的样本掩码，以控制像素着色器的执行。

2. **`glEnable` 和 `glDisable`**：你可以使用`glEnable(GL_SAMPLE_MASK)`和`glDisable(GL_SAMPLE_MASK)`来启用或禁用样本掩码。当启用时，样本掩码会生效，影响像素着色器的执行。当禁用时，所有样本都将参与像素着色器的计算。

3. **OpenGL扩展**：某些OpenGL扩展提供了更高级的样本掩码控制功能。例如，`GL_ARB_sample_shading` 扩展允许你设置每个像素的样本覆盖率，以控制不同样本对最终颜色的影响程度。

以下是一个使用`glSampleMaski`函数来设置样本掩码的示例代码：

```c
// 设置样本掩码，控制哪些样本参与像素着色器的计算
GLushort sampleMaskValue = 0b1100; // 设置第0和第1个样本参与计算
GLuint sampleMaskIndex = 0; // 样本掩码的索引
glSampleMaski(sampleMaskIndex, sampleMaskValue);

// 启用样本掩码
glEnable(GL_SAMPLE_MASK);

// 在像素着色器中使用样本掩码
```

请注意，具体的使用方式可能会因OpenGL版本和扩展的支持而有所不同。要了解更多关于样本掩码的详细信息和使用方法，建议查阅OpenGL的官方文档和相关扩展文档。

# ARB
##  ARB_framebuffer_no_attachment


是的，`ARB_framebuffer_no_attachment` 这个扩展的功能已经被集成到了现代OpenGL标准中。具体地说，OpenGL 4.3引入了这个扩展的功能，并将其包括在核心OpenGL规范中。这意味着从OpenGL 4.3开始，你可以在不附加任何附件的情况下创建FBO，而不需要显式启用这个扩展。

所以，现代OpenGL版本（4.3及更高版本）允许你创建没有附件的FBO，而不再需要依赖 `ARB_framebuffer_no_attachment` 扩展。这一改变提供了更大的灵活性，允许开发者更精细地控制渲染管线，以满足各种渲染需求，而不需要强制附加渲染缓冲或纹理附件。

# ARB assembly-style program fields

`ARB assembly-style program fields` 是一种 OpenGL 着色器程序的特定字段和参数的命名约定，它通常用于与着色器程序相关的注释和文档中。

在 OpenGL 中，着色器程序可以以多种不同的方式编写，包括高级语言（如GLSL）和汇编语言（如ARB assembly-style）。`ARB assembly-style` 指的是一种使用类似汇编语言的方式编写着色器程序的方法。这种方式更底层，直接操作着色器指令和寄存器。

`ARB assembly-style program fields` 指的是在使用这种汇编风格编写的着色器程序中，特定字段和参数的命名约定。这些字段和参数包括着色器指令、寄存器、输入和输出变量等。这个约定用于帮助开发人员理解和编写底层的着色器程序。

通常，这种汇编风格的着色器编写方式不太常见，因为它更复杂和低级，而现代 OpenGL 更倾向于使用高级语言（如GLSL）来编写着色器程序。然而，在某些情况下，开发人员可能会使用这种汇编风格的着色器来实现特定的优化或调试需求。


以下是一个使用OpenGL的ARB assembly-style程序的简单示例，该程序绘制一个着色的三角形。请注意，这是一个非常底层的示例，通常不建议在实际应用中使用ARB assembly-style程序，因为现代OpenGL倾向于使用更高级的GLSL语言。

```assembly
!!ARBvp1.0
PARAM mvp[4] = { state.matrix.mvp };

ATTRIB vertex_position = fragment.position;

DP4 result.position.x, mvp[0], vertex_position;
DP4 result.position.y, mvp[1], vertex_position;
DP4 result.position.z, mvp[2], vertex_position;
DP4 result.position.w, mvp[3], vertex_position;

MOV result.color, vertex.position;

END
```

上述代码是一个ARB assembly-style的顶点着色器程序示例，用于执行基本的变换操作和传递颜色信息到片段着色器。

- `!!ARBvp1.0`：这是ARB assembly-style程序的版本指令，指定使用版本1.0的顶点程序。

- `PARAM mvp[4]`：定义了一个参数`mvp`，它是一个4x4的矩阵，用于进行模型-视图-投影变换。

- `ATTRIB vertex_position`：指定了一个输入属性`vertex_position`，表示顶点的位置。

- `DP4 result.position.x, mvp[0], vertex_position;`：进行矩阵-向量乘法，将顶点位置进行变换，并将结果存储在`result.position`中。

- `MOV result.color, vertex.position;`：将顶点的位置信息传递给片段着色器作为颜色信息。

这只是一个简单的示例，展示了如何使用ARB assembly-style程序编写一个顶点着色器。在实际应用中，通常使用更高级的GLSL语言来编写着色器，因为它更易读和维护。此示例仅用于说明ARB assembly-style程序的基本工作原理。


# WQM

WQM 是 "Wavefront Quality Mode" 的缩写，是在AMD的GPU架构中的一个术语。WQM 是与GPU执行模型和片段着色器中的像素处理相关的一个重要概念。

在GPU中，WQM 表示一种执行模式，其中 GPU 在处理多个片段或像素时会采取一定的处理方式，以确保像素的颜色值和深度等信息的一致性。具体来说：

1. **Wavefronts（波前）**：GPU通常以一组称为"wavefront"（波前）的线程组为单位来执行着色器程序。这些线程在执行过程中会一起执行相同的指令，但处理不同的数据片段。

2. **WQM 模式**：在 WQM 模式下，GPU会确保在同一个波前中的所有线程产生相同的结果，即它们的输出颜色、深度等信息都一致。这是为了避免在渲染过程中出现不一致的结果，因为不同线程可能处理不同片段，但最终结果必须在同一个波前中保持一致。

3. **像素着色器中的应用**：WQM 特别重要的地方在于像素着色器（片段着色器）。在像素着色器中，不同的像素可能需要执行不同的代码路径，例如，某些像素可能被丢弃（由于深度测试或 alpha 测试），而其他像素可能需要不同的纹理采样。WQM 确保在同一个波前中的所有像素都执行相同的代码路径，以保持一致性。

4. **性能优化**：WQM 可能会涉及额外的开销，因为需要确保一致性，但它有助于确保在渲染过程中不会出现不一致的渲染结果。在某些情况下，GPU 可能会通过 WQM 模式下的延迟加载数据来提高性能，这意味着只有在实际需要时才会加载数据。

总之，WQM 是一种确保 GPU 在处理像素时保持一致性的执行模式，尤其在像素着色器中，以避免不一致的渲染结果。这对于实现正确的图形渲染非常重要。


# Derivatives

"Derivatives" 指的是在着色器中计算变量的导数（或称为梯度）。在图形编程中，导数通常用于计算曲线或曲面上某一点的斜率或梯度。在着色器编程中，导数通常与纹理采样、法线映射、阴影计算等图形效果相关。

在着色器中，有两种主要类型的导数：

1. **常规导数（Regular Derivatives）**：这些导数通常用于计算变量的局部变化率。在片段着色器中，常规导数通常用于计算纹理坐标的变化率，以进行纹理采样。常规导数可以通过内置函数如 `dFdx` 和 `dFdy` 来获取。

2. **精确导数（Exact Derivatives）**：这些导数通常更准确，适用于需要高精度的情况。例如，在阴影计算中，需要非常精确的导数来确定阴影的边界。精确导数通常通过使用特殊的着色器扩展或标志来启用，以获得更高的计算精度。

导数在许多图形效果中都非常有用。例如，通过计算法线映射，可以使曲面看起来更加真实；通过计算梯度，可以实现光滑的渐变效果。然而，导数的计算可能会涉及到额外的计算成本，因此在性能要求较高的情况下需要谨慎使用。

总之，"derivatives"（导数）是着色器编程中用于计算变量的变化率或梯度的重要概念，用于许多图形效果的计算。它们通常用于纹理采样、法线映射、阴影计算等场景中。

# KILL

在OpenGL的片段着色器（Fragment Shader）中，KILL 是一种特殊的片段着色器指令，用于丢弃当前正在处理的片段（或称为像素），使其不参与后续的渲染操作。这个指令的作用是在片段着色器中进行条件性的片段丢弃，以基于一些条件来决定是否绘制当前片段。

KILL 指令通常与片段着色器中的条件语句一起使用，以根据某些条件来决定是否绘制当前像素。例如，你可以在片段着色器中执行一些计算，然后根据计算结果决定是否丢弃该像素。如果计算结果满足特定条件，可以使用 KILL 指令来终止该片段的处理，从而不将其写入帧缓冲，这将影响最终渲染结果。

以下是一个简单的示例，演示了如何在片段着色器中使用 KILL 指令：

```glsl
#version 330 core
in vec4 fragColor;
out vec4 finalColor;

void main() {
    // 执行一些条件判断，例如，基于某些条件丢弃片段
    if (some_condition) {
        discard; // 使用 KILL 指令来丢弃当前片段
    }

    // 如果未丢弃，将颜色写入帧缓冲
    finalColor = fragColor;
}
```

在上面的示例中，如果 `some_condition` 满足，那么使用 `discard` 指令将当前片段丢弃，否则将颜色写入帧缓冲。

KILL 指令在某些情况下非常有用，例如在进行深度测试、alpha 测试或实现特殊的片段丢弃逻辑时。然而，需要谨慎使用，因为它可能会影响性能，并且不适用于所有情况。通常情况下，尽量避免在片段着色器中频繁使用 KILL 指令，以保持良好的性能。


# FMA 

FMA 是 "Fused Multiply-Add"（融合乘法-加法）的缩写，它是一种在计算机中执行浮点数运算的指令或操作。FMA 操作同时执行乘法和加法，并将结果合并成一个步骤，这可以提高数值计算的性能和精度。

标准的乘法和加法操作可以分成两个步骤进行：

1. 乘法：执行两个数的乘法，生成一个中间结果。
2. 加法：将中间结果与另一个数相加，得到最终的结果。

而 FMA 操作将这两个步骤合并成一个单独的操作，如下所示：

```
result = (a * b) + c
```

在这个操作中，`a` 和 `b` 是要相乘的两个数，`c` 是要添加到乘积的数，而 `result` 是最终的结果。FMA 操作能够在单个时钟周期内执行这两个操作，这通常比分开执行乘法和加法操作更快。

FMA 操作在科学计算、图形渲染、信号处理等领域中非常有用，因为它可以提高计算速度，同时降低了舍入误差的累积。由于 FMA 操作的高性能和精度，许多现代处理器和图形处理单元（GPU）都支持它，并且在数值计算密集型应用中广泛使用。



# std430

GLSL（OpenGL着色器语言）中的数据布局规范用于控制 Uniform Buffer Objects (UBO) 和 Shader Storage Buffer Objects (SSBO) 数据的排列方式。以下是GLSL中常见的数据布局规范：

1. `std430`：`std430` 布局规范要求数据以一致的方式排列，不允许填充字节，确保着色器程序在不同硬件平台上的一致性。这个规范通常用于确保数据在不同系统上的兼容性。

2. `std140`：`std140` 布局规范类似于 `std430`，但允许一些额外的填充字节，以确保数据按照规定的对齐方式排列。这个规范通常用于权衡性能和内存使用。

3. `packed`：`packed` 布局规范通常用于最小化内存使用，允许数据以紧凑的方式排列，但可能会导致性能损失，因为数据的访问可能不够高效。

4. `shared`：`shared` 布局规范用于共享内存，通常在计算着色器中使用，以便多个线程可以访问共享内存。数据排列方式需要特别注意。

5. `row_major` 和 `column_major`：这两个布局规范用于指定矩阵的排列方式。`row_major` 表示矩阵以行主序排列，而 `column_major` 表示矩阵以列主序排列。

这些布局规范允许开发人员根据需要选择合适的数据排列方式，以满足性能和内存使用的需求。选择合适的规范通常取决于应用程序的需求、硬件支持以及需要与其他系统或平台兼容。不同的规范具有不同的优势和权衡，开发人员需要根据具体情况做出选择。


# subroutine 

OpenGL GLSL（OpenGL着色器语言）中的子例程（Subroutine）是一种编程功能，允许着色器程序在运行时根据需要切换不同的函数实现。这允许着色器在不改变整个程序的情况下替换特定的函数实现，从而实现动态的功能切换。

具体来说，子例程在GLSL中通常用于以下场景：

1. 多个着色器函数实现：在某些情况下，着色器程序可能需要根据特定的条件或需求使用不同的函数实现。使用子例程，您可以定义多个函数实现，并在运行时根据需要选择其中一个进行执行。

2. 简化着色器代码：使用子例程，您可以将一些常见的功能模块分解为独立的函数，并在多个着色器中共享这些函数，从而减少代码的重复性。

3. 灵活的着色器组合：通过在着色器中使用子例程，您可以创建具有不同功能的模块，并在需要时以不同的组合方式组合这些模块，以实现复杂的着色器效果。

子例程的使用通常包括以下步骤：

1. 定义子例程：在GLSL中，您可以使用 `subroutine` 关键字定义子例程，然后为子例程定义多个具体的函数实现。例如：

```c
subroutine void LightingModel(vec3 lightDir, vec3 normal, out vec4 color);
```

2. 为变量指定子例程类型：在着色器中，您可以使用 `subroutine()` 类型来声明变量，并将变量与特定的子例程类型关联。例如：

```c
subroutine(LightingModel) vec4 lightModel;
```

3. 在运行时选择子例程：在着色器程序的执行期间，您可以使用 `subroutine uniform` 类型的变量来选择要执行的子例程实现。通过更改这些 uniform 变量的值，您可以动态切换子例程的执行。

子例程允许在着色器中实现更灵活的逻辑和功能组合，以满足不同的渲染需求，同时保持着色器代码的可维护性和可扩展性。

[subroutine](https://www.khronos.org/opengl/wiki/Shader_Subroutine)

# dsa


OpenGL DSA（Direct State Access）是OpenGL的一个扩展，旨在简化OpenGL状态更改和对象操作的方式。DSA允许您以更直接的方式操作OpenGL对象，而无需切换OpenGL上下文或绑定对象。这提高了OpenGL的可用性和易用性，并减少了开发过程中的错误和复杂性。

在传统的OpenGL中，对象状态通常通过绑定（Binding）和激活（Enable/Disable）来管理。这意味着您需要在操作对象之前将其绑定到OpenGL上下文，并在操作完毕后解绑。这种状态切换和绑定操作可能会导致性能开销，并且容易引入错误，因为您需要确保正确绑定和解绑对象。

DSA引入了一种更直接的方法，它允许您在不绑定对象的情况下直接修改对象状态。例如，您可以使用DSA直接更改纹理参数，分配缓冲区数据，或者创建Framebuffer对象，而无需显式绑定对象。这使得代码更清晰，更易于维护，并且通常更高效。

以下是一些OpenGL DSA的典型用法示例：

1. **纹理参数设置（Texture Parameter Setting）：** 使用DSA，您可以直接设置纹理对象的参数，而无需先绑定纹理对象。

   ```cpp
   glTextureStorage2D(texture, 1, GL_RGBA8, width, height);
   glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   ```

2. **Framebuffer对象创建（Framebuffer Object Creation）：** 使用DSA，您可以创建Framebuffer对象而无需显式绑定。

   ```cpp
   GLuint fbo;
   glCreateFramebuffers(1, &fbo);
   ```

3. **缓冲区分配（Buffer Allocation）：** 使用DSA，您可以直接分配缓冲区数据，而无需绑定缓冲区。

   ```cpp
   glNamedBufferData(buffer, size, data, GL_STATIC_DRAW);
   ```

请注意，DSA功能通常需要OpenGL 4.5或更高版本的支持。如果您的OpenGL实现支持DSA，那么可以考虑在项目中使用它，以改善代码的可读性和性能。但请注意，使用DSA需要谨慎，因为某些情况下可能会导致不必要的复杂性。


# prim_mask

在OpenGL中，`prim_mask` 通常指的是 Primitive Mask（原语掩码），它是一个用于控制渲染管线中哪些原语（通常是三角形）应该被绘制的一种机制。

原语掩码是OpenGL的一种高级功能，通常与GPU的多核心或多线程渲染相关。它可以用来控制哪些绘制命令会影响帧缓冲区，以及如何并行处理多个绘制命令，以提高渲染性能。

在OpenGL中，渲染命令通常是单线程执行的，这意味着在绘制大量原语（例如三角形）时，会消耗大量的CPU时间。为了提高性能，现代OpenGL实现引入了原语掩码的概念，允许开发者指定哪些原语应该被渲染，从而将工作分配给多个核心或线程。

以下是一些关于 `prim_mask` 的常见用法和示例：

1. **多线程渲染：** 原语掩码可以用于将渲染任务分配给多个线程或核心。每个线程可以使用不同的原语掩码来渲染不同的部分，从而提高渲染的并行性。

2. **剪裁：** 原语掩码还可用于剪裁不可见的原语。如果某个原语的掩码被设置为不绘制，则它将被跳过，不会对帧缓冲区产生任何影响。

3. **条件渲染：** 使用原语掩码，您可以实现条件渲染，即只在满足特定条件时才渲染某些原语。这对于实现一些高级的渲染技术非常有用。

请注意，原语掩码通常是OpenGL实现的高级功能，不是所有OpenGL版本和驱动程序都支持。要使用原语掩码，您需要查看OpenGL实现的文档，并确保您的硬件和驱动程序支持相关功能。具体的原语掩码使用方式可能因OpenGL版本和硬件而异。


# st_validate_state

通过MESA_VERBOSE=state可以得出哪些状态设置.

测试
```

Mesa: _mesa_update_state: (0xffffffff) ctx->ModelView, ctx->Projection, ctx->TextureMatrix, ctx->Color, ctx->Depth, ctx->Eval/EvalMap, ctx->Fog, ctx->Hint, ctx->Light, ctx->Line, ctx->Pixel, ctx->Point, ctx->Polygon, ctx->PolygonStipple, ctx->Scissor, ctx->Stencil, ctx->Texture(Object), ctx->Transform, ctx->Viewport, ctx->Texture(State), ctx->Array, ctx->RenderMode, ctx->Visual, ctx->DrawBuffer,, 

```
根据st->dirty 来判断哪些状态需要更新以及调用在update_function中的函数。



# CLIP_PANE
OpenGL中的`glClipPlane`函数用于定义裁剪平面，它允许你定义一个平面，以此平面来裁剪或剔除渲染的几何体。这个函数通常与裁剪测试一起使用，以确定哪些部分的几何体应该被渲染，哪些部分应该被剔除。

以下是`glClipPlane`函数的基本用法：

```c
void glClipPlane(GLenum plane, const GLdouble *equation);
```

- `plane`参数指定要定义的裁剪平面，可以是`GL_CLIP_PLANE0`到`GL_CLIP_PLANE5`中的一个，这意味着你最多可以定义六个裁剪平面。
- `equation`参数是一个指向包含裁剪平面方程系数的数组的指针。裁剪平面方程通常表示为Ax + By + Cz + D = 0，`equation`数组应包含A、B、C和D的值。裁剪平面方程的法线（A、B、C）通常应该是单位长度的，以确保正确的裁剪。

例如，如果要定义一个平面将所有Z坐标小于0的几何体剔除掉，可以这样做：

```c
GLfloat clipPlaneEquation[] = {0.0f, 0.0f, 1.0f, 0.0f}; // 裁剪平面方程，表示Z=0平面
glClipPlane(GL_CLIP_PLANE0, clipPlaneEquation);
```

然后，在渲染时，你可以启用裁剪平面并进行渲染：

```c
glEnable(GL_CLIP_PLANE0);
// 渲染几何体
glDisable(GL_CLIP_PLANE0); // 在渲染完后禁用裁剪平面
```

在上述代码中，`glEnable(GL_CLIP_PLANE0)`启用了裁剪平面0，从而使所有Z坐标小于0的几何体在渲染时被剔除掉。

请注意，裁剪平面的定义会影响接下来的渲染操作，因此要确保在需要使用裁剪平面的地方启用它，并在不需要时禁用它，以免影响其他渲染操作。


以下是一个简单的OpenGL实际例子，演示如何使用`glClipPlane`定义一个裁剪平面来剔除一个正方体的底部：

```c
#include <GL/glut.h>

// 定义裁剪平面的方程
GLfloat clipPlaneEquation[] = {0.0f, -1.0f, 0.0f, 0.0f}; // 表示Y=0平面

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 启用裁剪平面0
    glEnable(GL_CLIP_PLANE0);
    glClipPlane(GL_CLIP_PLANE0, clipPlaneEquation);

    // 绘制一个正方体
    glColor3f(1.0f, 0.0f, 0.0f); // 红色
    glutSolidCube(2.0);

    // 禁用裁剪平面0
    glDisable(GL_CLIP_PLANE0);

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(400, 400);
    glutCreateWindow("OpenGL Clip Plane Example");

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);

    glutMainLoop();
    return 0;
}
```

在这个示例中，我们定义了一个裁剪平面方程`clipPlaneEquation`，表示Y=0平面。在`display`函数中，我们启用了裁剪平面0并定义了它，然后绘制了一个红色的正方体。由于裁剪平面将Y坐标小于0的部分剔除掉，所以正方体的底部将被裁剪掉，只有上半部分可见。

你需要确保你的OpenGL环境正确设置，包括OpenGL库的链接和GLUT库的链接，以使该示例工作。此示例仅用于演示`glClipPlane`的基本用法，实际应用中，你可以根据需要定义不同的裁剪平面来实现更复杂的裁剪效果。

# KHR_blend_equation_advanced.


`KHR_blend_equation_advanced` 是一个OpenGL扩展，旨在扩展混合（blending）操作的能力，以允许更复杂的颜色混合和混合方程。这个扩展引入了一些新的混合方程，以及一些新的混合操作模式，允许开发者更精细地控制渲染中的颜色混合。

主要特性和功能包括：

1. **新的混合方程**：该扩展引入了一些新的混合方程，例如`GL_MULTIPLY_KHR`、`GL_SCREEN_KHR`、`GL_OVERLAY_KHR`等。这些混合方程可以用于不同的颜色混合效果，例如模拟光照、颜色叠加等。

2. **混合操作模式**：除了新的混合方程，扩展还引入了新的混合操作模式，如`GL_DARKEN_KHR`、`GL_LIGHTEN_KHR`、`GL_COLORDODGE_KHR`等。这些模式可以用于控制混合的方式，以实现不同的图形效果。

3. **自定义混合方程**：扩展允许开发者定义自定义的混合方程，从而实现更灵活的颜色混合效果。

这个扩展提供了更高级的混合功能，可以用于实现更复杂的图形效果，如光照、透明度、阴影等。要使用这个扩展，你需要检查OpenGL支持情况，并在渲染中使用相应的混合方程和混合操作模式。通常，你可以使用OpenGL的扩展机制来查询和启用这个扩展。

需要注意的是，扩展的具体细节和支持程度可能因OpenGL实现和硬件而异，因此在使用时需要查阅相关文档和检查OpenGL上下文的扩展字符串来确保扩展可用。

在现代OpenGL中，使用 `KHR_blend_equation_advanced` 扩展需要以下步骤：

1. 检查扩展支持：

   在你的OpenGL应用程序中，首先要检查是否支持 `KHR_blend_equation_advanced` 扩展。你可以使用现代OpenGL扩展加载库（如GLEW、GLAD等）来检查扩展的可用性。以下是一个示例代码片段：

   ```c
   if (GLEW_KHR_blend_equation_advanced) {
       // 扩展可用，可以使用高级混合功能
   } else {
       // 扩展不可用，无法使用高级混合功能
   }
   ```

   确保你在创建OpenGL上下文之后进行这个检查。

2. 启用高级混合模式：

   如果 `KHR_blend_equation_advanced` 扩展可用，你可以使用新的混合方程和混合操作模式来实现高级混合效果。以下是一个示例代码片段，展示如何设置高级混合模式：

   ```c
   // 设置混合方程
   glBlendEquationSeparatei(0, GL_FUNC_ADD, GL_FUNC_ADD); // 通常是GL_FUNC_ADD，但你可以选择其他方程

   // 设置混合操作模式
   glBlendFuncSeparatei(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE); // 通常是标准的混合模式，但你可以选择其他模式
   ```

   这里使用了 `glBlendEquationSeparatei` 和 `glBlendFuncSeparatei` 函数，它们允许你为每个颜色附件（attachment）指定不同的混合方程和混合操作模式。第一个参数是颜色附件的索引，通常为0。

3. 渲染：

   设置了高级混合模式后，你可以开始渲染场景。OpenGL将根据你设置的混合模式来执行混合操作，以实现所需的效果。

请注意，混合方程和混合操作模式将取决于你的渲染需求，你可以根据不同的场景和效果来选择适当的混合设置。要了解更多关于 `KHR_blend_equation_advanced` 扩展的信息，包括可用的混合方程和混合操作模式，你可以参考OpenGL的扩展文档或相关资源。此外，确保你的OpenGL上下文的版本支持该扩展，因为较旧的OpenGL版本可能不支持这些高级混合功能。



# EXT_shader_framebuffer_fetch

`EXT_shader_framebuffer_fetch` 和 `EXT_shader_framebuffer_fetch_non_coherent` 是OpenGL扩展，用于扩展着色器对帧缓冲器读取的能力。
这个在mesa默认是不启用的, 如果启用设置如下环境变量
```
export MESA_EXTENSION_OVERRIDE="+GL_EXT_shader_framebuffer_fetch +GL_EXT_shader_framebuffer_fetch_non_coherent"
```

1. **EXT_shader_framebuffer_fetch**：

   - `EXT_shader_framebuffer_fetch` 扩展引入了一个新的着色器内建函数 `texelFetch()`，允许着色器直接读取帧缓冲器的像素值，而无需使用纹理采样器。这对于一些高级渲染技术，如后期处理、逐像素光照等非常有用。

2. **EXT_shader_framebuffer_fetch_non_coherent**：

   - `EXT_shader_framebuffer_fetch_non_coherent` 是 `EXT_shader_framebuffer_fetch` 扩展的一个变种，它引入了一种非一致（non-coherent）的读取模式。在非一致模式下，读取的像素值可能不会考虑到之前的写入操作，因此用于某些需要更低级别控制的情况。

这两个扩展都扩展了着色器的功能，允许它们在渲染过程中更灵活地访问帧缓冲器的数据。这对于实现复杂的图形效果和渲染技术非常有用，但需要谨慎使用，因为直接的帧缓冲器读取操作可能会导致性能问题和非一致的结果。它们通常在需要特殊的图形效果和渲染需求时才会被使用。


## staging texture

暂存纹理 虽然 GraphicsDevice.UpdateTexture 始终可用于更新纹理对象，但首先将数据上传到中间“暂存纹理”，然后安排从该资源的副本到您想要渲染的纹理中会更有效。 “暂存纹理”只是使用TextureUsage.Staging 使用标志创建的纹理。 暂存纹理可以“映射”到 CPU 可见的数据范围。 这允许您直接写入由 GPU 驱动程序专门准备的区域来传输纹理数据。 数据映射和取消映射后（请参阅 GraphicsDevice.Map 和 GraphicsDevice.Unmap），您应该使用 CommandList.CopyTexture 安排副本。


## tile mode

"Tile mode"（瓦片模式）是一种纹理或帧缓冲存储的方式。在图形渲染中，通常有两种主要的存储模式：线性模式和瓦片模式。

1. **线性模式：** 数据被按照线性地址排列，也就是说，所有的像素按照一定的顺序依次存储。这种模式的优点是对顺序读取和写入操作来说比较高效，但对于随机读取或写入来说效率较低。

2. **瓦片模式：** 数据被分割成小块（瓦片），每个瓦片按照自己的地址空间存储。这种模式的优点是可以更好地利用缓存，尤其是对于图形渲染中常见的大量相邻像素操作，如纹理采样和渲染目标的读写。由于瓦片之间的距离相对较小，它更适合并行操作。

在某些硬件架构中，特别是在一些图形处理单元（GPU）中，瓦片模式通常被用于提高内存访问效率。然而，这可能会引入一些复杂性，因为需要处理纹理或帧缓冲的布局和瓦片之间的传输。在特定的图形API和硬件上，可能需要开发者手动管理瓦片模式，以便优化图形渲染性能。


https://fgiesen.wordpress.com/2011/01/17/texture-tiling-and-swizzling/


### bin


https://patents.google.com/patent/US20140292756A1/en



### eqaa

https://www.geeks3d.com/20110915/amd-eqaa-modes-enhanced-quality-anti-aliasing-for-radeon-hd-6900-series/

EQAA 表示 Enhanced Quality Anti-Aliasing（增强质量抗锯齿）。抗锯齿（Anti-Aliasing）是一种图形技术，用于减轻计算机图形中锯齿状边缘（jagged edges）的现象。EQAA 是对传统抗锯齿技术的一种改进，旨在提供更高质量的图形呈现。

在 EQAA 中，除了常规的多重采样抗锯齿（MSAA）采样点之外，额外的样本点被引入，这些点用于提高图形的质量。通常，EQAA 通过在图形渲染中引入更多的样本点来减少锯齿状边缘，从而提供更平滑、更真实的图形效果。

EQAA 的具体实现方式可能因图形硬件和驱动程序而异，但其目标是改进抗锯齿效果，使得图形在视觉上更加清晰和真实。EQAA 可能是图形卡驱动程序中的一个设置选项，允许用户在图形质量和性能之间进行权衡选择。


# SIMD/CPU


https://tvm.d2l.ai/chapter_cpu_schedules/arch.html


# 透视插值


https://www.comp.nus.edu.sg/~lowkl/publications/lowk_persp_interp_techrep.pdf

