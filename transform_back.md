# Tranform Feedback基础


https://www.khronos.org/opengl/wiki/Transform_Feedback


Transform Feedback（变换反馈）是捕获由顶点处理步骤生成的基元（Primitives），并将这些基元的数据记录到缓冲区对象中的过程。这允许保留对象的变换后渲染状态，并可以多次重新提交这些数据。

注意：将提到处理多个流输出的各种函数。反馈到多个流需要访问OpenGL 4.0或ARB_transform_feedback3和ARB_gpu_shader5扩展。因此，如果这些扩展对您不可用，请忽略任何涉及到这些扩展的讨论。


## 着色器设置

为了捕获基元，包含最终顶点处理着色器阶段的程序必须在链接之前使用一些参数链接。这些参数必须在链接程序之前设置，而不能在之后设置。因此，如果要使用`glCreateShaderProgram`，必须使用OpenGL 4.4或ARB_enhanced_layouts提供的着色器内规范API。

平台问题（NVIDIA）：NV_transform_feedback允许在链接后动态设置这些参数。对于变换反馈而言，唯一重要的程序对象是提供最后顶点处理着色器阶段的程序对象。这些设置可以在另一个程序中的任何其他程序上进行，但只有最后激活的顶点处理着色器阶段上的设置将被使用。

Transform feedback可以以两种捕获模式之一操作。在交错模式下，所有捕获的输出都进入同一缓冲区，彼此交错。在分离模式下，每个捕获的输出都进入单独的缓冲区。这必须在链接时在程序对象上进行选择。

注意：分离的捕获模式并不一定意味着它必须捕获到不同的Buffer Objects。它只是意味着它们被捕获到单独的缓冲绑定点。同一缓冲对象的不同区域可以绑定到不同的绑定点，因此允许你在同一缓冲对象中进行分离捕获。只要绑定的区域不重叠，这将起作用。

要定义程序的捕获设置以及捕获哪些输出变量，可以使用以下函数：

```c
void glTransformFeedbackVaryings(GLuint program​, GLsizei count​, const char **varyings​, GLenum bufferMode​);
```

`bufferMode​`是捕获模式。它必须是`GL_INTERLEAVED_ATTRIBS`或`GL_SEPARATE_ATTRIBS`。

`count​`是`varyings​`数组中字符串的数量。该数组指定要捕获的适当着色器阶段的输出变量的名称。变量的提供顺序定义了它们被捕获的顺序。这些变量的名称符合GLSL变量的命名规则。

有关可捕获的输出数量以及可捕获的总组件数量的限制。在这些限制内，可以捕获任何输出变量，包括结构类型、数组和接口块的成员。

注意：此函数设置程序的所有反馈输出。因此，如果再次调用它（在链接之前），之前的设置将被忘记。




### 捕获的数据格式

变换反馈显式捕获基元。虽然它确实捕获每个顶点的数据，但是在将每个基元拆分为单独的基元之后进行捕获。因此，如果使用GL_TRIANGLE_STRIP进行渲染，并且使用6个顶点进行渲染，则会产生4个三角形。变换反馈将捕获4个三角形的数据。由于每个三角形有3个顶点，TF将捕获12个顶点，而不是从绘图命令中可能期望的6个。

每个基元按照处理的顺序进行编写。在一个基元内，编写的顶点数据的顺序是在基元装配后的顶点顺序。这意味着，例如，使用三角形条进行绘制时，捕获的基元将在每隔一个三角形中交换两个顶点的顺序。这确保了面剔除仍然尊重初始顶点数据数组中提供的顺序。

在每个顶点内部，数据按照`varyings`数组指定的顺序进行编写（在进行交错渲染时）。如果输出变量是聚合（结构/数组），则按顺序写入聚合的每个成员。输出的基本类型的每个组件都按顺序编写。最终，所有数据都紧密打包在一起（尽管捕获双精度浮点数可能会导致填充）。

组件将始终是float/double、有符号整数或无符号整数，使用GLfloat/GLdouble、GLint和GLuint的大小。不执行打包或规范化。变换反馈系统没有更灵活的顶点格式规范系统的自动模拟。

当然，这并不妨碍你在着色器中手动将位打包到无符号整数中。

### 高级交错

**高级交错**
**核心版本：** 4.6
**自版本：** 4.0
**ARB扩展：** ARB_transform_feedback3

上述设置只提供两种模式：要么所有捕获的输出都进入单独的缓冲区，要么它们全部进入同一个缓冲区。在许多情况下，能够将多个组件写入一个缓冲区，同时将其他组件写入其他缓冲区是很有用的。

此外，捕获的交错数据是紧密打包的，每个变量的组件紧跟在前一个组件之后。如果某些数据更改而其他数据不更改，有时跳过某些数据是很有用的。

可以通过在`varyings`数组中使用特殊的“varying”名称来实现这些效果。这些特殊名称不命名实际的输出变量；它们只对后续的写入产生某些特定的影响。

这些名称及其效果如下：

- **gl_NextBuffer：** 这导致所有后续的输出都被路由到下一个缓冲区索引。这些缓冲区从0开始，每次在`varyings`列表中遇到此名称时递增1。
  - 不能有超过可以用于变换反馈的缓冲区的数量。因此，这些的数量必须严格小于GL_MAX_TRANSFORM_FEEDBACK_BUFFERS。

- **gl_SkipComponents#：** 这使系统跳过写入#数量的组件，其中#可以是1到4。跳过的组件覆盖的内存将不被修改。在这种情况下，每个组件的大小都是float的大小。
  - 请注意，以这种方式跳过的组件仍然计入输出组件数量的限制。

几何着色器中的输出变量可以声明流到特定流。这是通过着色器内部的规范来控制的，但有一些限制会影响高级组件交错。

没有两个流到不同流的输出可以被同一个缓冲区捕获。试图这样做将导致链接错误。因此，使用交错写入的多个流需要使用高级交错将属性路由到不同的缓冲区。

请注意，这种能力实际上使分离的捕获模式变得多余。使用这些功能进行交错是分离模式可以做的功能的一个功能超集，因为它可以将每个输出单独捕获到每个缓冲区。


## 双精度浮点数和对齐

**双精度浮点数对齐**  
**核心版本：** 4.6  
**自版本：** 4.0  
**ARB扩展：** ARB_gpu_shader_fp64

单精度浮点数和整数的对齐方式为4字节。然而，双精度值的对齐方式为8字节。这在捕获变换反馈数据时会导致问题。

必须确保组件的对齐。对于浮点数和整数，这很容易确保，但是双精度需要特别注意。用户必须确保所有双精度数据都具有8字节的对齐。具体而言，您必须确保两件事：

1. 每个双精度组件都从8字节边界开始。您可能需要根据需要插入填充，使用上面介绍的跳过功能。
2. 去往包含双精度组件的特定缓冲区的所有顶点数据的总顶点数据大小必须对齐到8字节。这确保第二个顶点将从8字节边界开始。因此，您可能需要在顶点数据的末尾添加填充。

例如，如果您想按照以下定义的顺序捕获：

```glsl
out DataBlock
{
  float var1;
  dvec2 someDoubles;
  float var3;
};
```

以下是您在`varyings`数据中需要的字符串序列，如果您想按定义的顺序捕获它：

```cpp
const char *varyings[] =
{
  "DataBlock.var1",
  "gl_SkipComponents1",     // 填充下一个组件以对齐到8字节。
  "DataBlock.someDoubles",
  "DataBlock.var3",
  "gl_SkipComponents1",     // 将整个顶点结构填充到8字节对齐。
};
```

如果不这样做，就会导致未定义的行为。您可以通过更改捕获它们的顺序来避免填充。您不必更改在着色器中定义它们的顺序。

### 在着色器中的规范

**在着色器中的规范**  
**核心版本：** 4.6  
**自版本：** 4.4  
**ARB扩展：** ARB_enhanced_layouts

着色器可以定义由变换反馈捕获的输出以及它们的精确捕获方式。当着色器定义它们时，查询程序的变换反馈模式将返回交错模式（因为高级交错使得分离模式成为交错模式的一个完整子集）。

布局限定符可用于定义在变换反馈操作中捕获哪些输出变量。当在着色器中设置这些限定符时，它们将完全覆盖通过`glTransformFeedbackVaryings`通过OpenGL设置变换反馈输出的任何尝试。

通过使用`xfb_offset`布局限定符声明的任何输出变量或输出接口块都将成为变换反馈输出的一部分。该限定符必须以整数字节偏移指定。偏移是要写入当前缓冲区的顶点开始处的字节数。

根据以前组件的大小，计算包含的值的偏移（数组、结构或接口块的成员，如果整个块具有偏移量）的方式计算。不允许显式提供的偏移违反对齐限制。因此，如果定义包含双精度（直接或间接）的定义，则偏移必须是8字节对齐的。

可以在它们身上直接指定接口块的成员的偏移，这将覆盖任何计算的偏移。另外，接口块的所有成员都不需要写入输出（尽管如果在块本身上设置`xfb_offset`，将会发生这种情况）。几何着色器的输出流的流分配必须对块的所有成员保持一致，但偏移不是必需的。

被捕获的不同变量分配给缓冲区绑定索引。偏移分配是为分离的缓冲区分开的。如果由同一缓冲区捕获的两个变量有重叠的字节偏移，无论是自动计算还是显式分配，都会导致链接错误。

可以通过在具有`xfb_offset`限定符的声明上使用`xfb_buffer`限定符进行显式缓冲区分配。这需要一个整数，定义捕获的输出与哪个缓冲区绑定索引关联。该整数必须小于`GL_MAX_TRANSFORM_FEEDBACK_BUFFERS`。

对于未显式指定缓冲区的全局变量或接口块的任何偏移，都将使用当前缓冲区。当前缓冲区的设置如下：

```glsl
layout(xfb_buffer = 1) out;
```

所有随后为不显式指定缓冲区的全局变量设置的偏移都将使用1作为它们的缓冲区。着色器的初始当前缓冲区为0。

变量可以具有分配给它们的`xfb_buffer`，而没有`xfb_offset`。这无效，将被忽略。

接口块与缓冲区有特殊的关联。每个接口块都与一个缓冲区关联，而不管其成员是否被捕获。该缓冲区可以是上述定义

的当前缓冲区，也可以是由`xfb_buffer`显式指定的缓冲区。

如前所述，块的所有成员都不必被捕获。但是，如果块的任何成员被捕获，则它们都必须被捕获到相同的缓冲区。具体而言，与该块相关联的缓冲区。如果缓冲区索引与块使用的索引不同，则在块成员上使用`xfb_buffer`是一个错误。

举个例子：

```glsl
layout(xfb_buffer = 2) out; // 默认缓冲区为2。

out OutputBlock1            // 块缓冲区索引隐式为2。
{
  float val1;
  layout(xfb_buffer = 2, xfb_offset = 0) first;  // 提供的索引与块的索引相同。
  layout(xfb_buffer = 1, xfb_offset = 0) other;  // 编译错误，因为更改了块成员的缓冲区索引。
};
```

每个缓冲区都有概念上的步幅。这表示从捕获的一个顶点的开始到下一个顶点的开始的字节数。它是通过取具有最高`xfb_offset`值的输出，将其大小添加到该偏移并将计算值的基本对齐方式对齐来计算的。缓冲区的对齐方式为4，除非它捕获任何双精度值，在这种情况下它为8。这意味着您不需要手动填充结构以进行对齐，就像在外部着色器设置中那样。

缓冲区的步幅也可以使用`xfb_stride`布局限定符显式设置。这允许您在末尾添加额外的空间，也许是为了跳过不会更改的数据。如果指定的步幅：

- 对于该缓冲区捕获的数据的偏移和计算大小来说太小。
- 没有正确对齐。它必须至少是4字节对齐的，并且如果缓冲区捕获任何双精度值，则必须是8字节对齐的。

缓冲区的步幅如下设置：

```glsl
layout(xfb_buffer = 1, xfb_stride = 32) out; // 将缓冲区1的步幅设置为32。同时，将缓冲区1设置为当前。
```

如果在一个缓冲区中的任何捕获的输出在空间中重叠或违反填充，将导致链接错误。例如：

```glsl
layout(xfb_buffer = 0) out Data
{
  layout(xfb_offset = 0) float val1;
  layout(xfb_offset = 4) vec4 val2;
  layout(xfb_offset = 16) float val3;  // 编译错误。val2覆盖范围[4, 20)的字节。
};
```

如果使用几何着色器输出流，并且来自不同流的两个输出被路由到同一个缓冲区，则将导致编译器/链接器错误。

注意：当将ARB_enhanced_layouts作为扩展使用（在旧硬件上），如果没有同时可用的ARB_transform_feedback3，则只能输出到单个缓冲区。您仍然可以使用偏移来放置顶点属性数据之间的空间，但不能将`xbf_buffer`设置为除0以外的任何值。


## 缓冲绑定

一旦你有一个带有正确设置以记录输出的程序，现在你必须设置缓冲对象来捕获这些值。缓冲对象及其存储通常以通常的方式创建。改变的是你在哪里使用它们。 当你希望开始一个变换反馈操作时，你必须将一个或多个缓冲区绑定到索引的`GL_TRANSFORM_FEEDBACK_BUFFER`绑定点。这是通过使用`glBindBufferRange`函数（或等效函数）完成的。你提供的偏移必须是4字节对齐的，除非要捕获到缓冲区中的数据包含双精度值。在这种情况下，它必须是8字节对齐的。 你绑定缓冲区范围的索引是设置不同输出位置的索引，这些位置是在设置输出的位置时使用的索引，按`varyings`提供的顺序从0开始分配的。在交错模式下，输出要么全部记录到一个缓冲区，要么记录到更高级布局规范机制中指定的缓冲区。
## 反馈过程

一旦缓冲区被绑定，就可以开始反馈操作了。通过调用以下函数实现：

```cpp
void glBeginTransformFeedback(GLenum primitiveMode);
```

这激活了变换反馈模式。当变换反馈模式处于活动状态（且没有暂停）时，如果执行绘图命令，则将设置为由最终顶点处理阶段捕获的所有输出记录到绑定的缓冲区中。这些将跟踪最后记录的位置，以便多个绘图命令将数据添加到记录的数据中。

如果你绑定的缓冲区范围不够大以容纳记录的数据，则会导致未定义的行为。

当前程序分配的所有反馈缓冲区绑定索引必须具有有效的绑定。如果没有，则此函数将失败，并显示OpenGL错误。

`primitiveMode`必须是GL_POINTS、GL_LINES或GL_TRIANGLES之一。这些模式定义了系统捕获的原语的类型。它们还对达到最终原语组装阶段的原语类型设置了限制。该渲染过程的原语基本类型定义如下，按给定的顺序：

1. 如果激活了几何着色器，则它是GS输出的原语类型。
2. 如果激活了曲面细分评估着色器，则它是细分过程生成的原语类型。
3. 如果两者都不可用，则它是用于渲染的命令的`mode`参数。

当变换反馈处于活动状态（且未暂停）时，肯定有一些事情是不能做的：

- 更改`GL_TRANSFORM_FEEDBACK_BUFFER`缓冲区绑定。
- 执行任何读取或写入这些缓冲区的任何部分的操作（当然，除了反馈写入）。
- 重新分配这些缓冲区的任何存储空间。这包括使其无效。
- 更改当前程序。所以不能调用`glUseProgram`或`glBindProgramPipeline`。这还包括重新链接适当的程序，以及如果目标管道是绑定管道，则调用`glUseProgramStages`。

要结束反馈模式，调用：

```cpp
void glEndTransformFeedback();
```

例如：

```cpp
glUseProgram(g_program);
glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedback_buffer, buffer_offset, number_of_bytes);
glBeginTransformFeedback(GL_LINES);
glBindVertexArray(vao);
glDrawElements(GL_POINTS, sizeof(pindices)/sizeof(ushort), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
glEndTransformFeedback();
glUseProgram(0);
```

## 变换反馈对象

OpenGL中的变换反馈操作所需的状态集合可以封装到OpenGL对象中。变换反馈对象是容器对象。它们使用`glGenTransformFeedbacks`、`glDeleteTransformFeedbacks`等通常的方式创建和管理。

要绑定变换反馈对象，使用以下函数：

```cpp
void glBindTransformFeedback(GLenum target​, GLuint id​);
```

`target​`必须始终是`GL_TRANSFORM_FEEDBACK`。如果当前的变换反馈对象处于活动状态且没有暂停，就不能绑定变换反馈对象。

这些对象封装的状态包括：

- 通用缓冲绑定目标`GL_TRANSFORM_FEEDBACK_BUFFER`。因此，所有对`glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, ...)`的调用将把缓冲区附加到当前绑定的反馈对象上。
- 所有索引的`GL_TRANSFORM_FEEDBACK_BUFFER`绑定。因此，所有对`glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, ...)`（或任何等效函数）的调用将把缓冲区的给定区域附加到当前绑定的反馈对象上。
- 变换反馈是否处于活动状态和/或暂停状态。
- 如果它处于活动状态，当前反馈操作中记录的原语数等的当前计数。

因此，反馈对象存储了正在被记录的缓冲区绑定。这使得可以轻松地在不同的反馈缓冲区绑定集之间切换，而不必每次都绑定它们。

反馈对象0是默认的变换反馈对象。你可以像任何其他变换反馈对象一样使用它，只是你不能删除它。

**注意：** 上述关于何时可以在绑定到进行变换反馈的缓冲区上做什么的说明仅适用于当前绑定的变换反馈对象。因此，如果取消绑定当前的变换反馈对象，可以再次更改它们。

### 反馈暂停和恢复

变换反馈对象不仅仅适用于轻松交换不同的缓冲区集。你可以暂时暂停变换反馈操作，执行一些不会被捕获的渲染，然后恢复反馈操作。反馈对象将正确跟踪要记录顶点数据的位置。

要暂时暂停反馈操作，调用以下函数：

```cpp
void glPauseTransformFeedback();
```

此时，反馈对象可以通过绑定不同的对象来取消绑定。当前程序可以更改等。反馈操作可以无限期地暂停，并且在缓冲区在暂停的反馈操作中读取是合法的（尽管首先需要取消绑定反馈对象）。

要恢复暂停的反馈操作，必须执行以下操作：

1. 绑定暂停的反馈对象。
2. 绑定调用`glBeginTransformFeedback`时使用的确切程序或管道对象。
3. 调用以下函数：

```cpp
void glResumeTransformFeedback();
```

请注意，在恢复后，原语模式仍将保持不变。

如果在已暂停的反馈对象上调用`glEndTransformFeedback`，它将正确结束该对象的反馈操作。不能在已结束的反馈对象上使用`glResumeTransformFeedback`。

### 反馈渲染

主题文章：[Vertex_Rendering#Transform_feedback_rendering](https://www.khronos.org/opengl/wiki/Vertex_Rendering#Transform_feedback_rendering)

要渲染由变换反馈操作捕获的数据，可以使用查询对象获取捕获的原语数量，将其乘以每个原语的顶点数，并将该顶点计数传递到顶点渲染函数中。但是，这个过程需要CPU从查询对象读取。这可能引发同步问题。虽然同步对象可以解决其中一些问题，但有更好的方法。

反馈对象记录它们在反馈操作中捕获的顶点数。请注意，只有在调用`glEndTransformFeedback`之前，反馈操作才是完整的，因此在尝试使用这些数据之前必须首先调用它。

一旦反馈操作完成，反馈对象就有了一个顶点计数。这可以通过绑定反馈对象并调用以下函数之一来用于渲染目的：

- `glDrawTransformFeedback`
- `glDrawTransformFeedbackInstanced`
- `glDrawTransformFeedbackStream`（使用OpenGL 4.0或ARB_transform_feedback3）
- `glDrawTransformFeedbackStreamInstanced`（使用OpenGL 4.0或ARB_transform_feedback3）

这些函数的工作方式类似于`glDrawArrays`（或`glDrawArraysInstanced`）。请注意，这些函数不执行任何顶点规范设置工作。它们不会自动获取反馈缓冲区并将它们绑定为顶点数据使用。你必须手动完成这些操作。所有这些函数所做的就是从反馈操作获取顶点计数。还请注意，在第一次变换反馈通过时，必须调用一个非变换的`glDraw*`函数将顶点数据写入变换反馈缓冲区，因为变换反馈对象尚未具有顶点计数信息。完成此操作后，`glDrawTransform*`可以在变换反馈和渲染到屏幕期间使用。



# mesa-18 实现分析

## state tracker 变换反馈接口

```c
struct st_transform_feedback_object {
   struct gl_transform_feedback_object base;

   unsigned num_targets;
   struct pipe_stream_output_target *targets[PIPE_MAX_SO_BUFFERS];

   /* 封装了可用作 draw_vbo 源的计数。
    * 它包含了每个流的上一次调用 EndTransformFeedback 的流输出目标。 */
   struct pipe_stream_output_target *draw_count[MAX_VERTEX_STREAMS];
}



void
st_init_xformfb_functions(struct dd_function_table *functions)
{
   functions->NewTransformFeedback = st_new_transform_feedback;
   functions->DeleteTransformFeedback = st_delete_transform_feedback;
   functions->BeginTransformFeedback = st_begin_transform_feedback;
   functions->EndTransformFeedback = st_end_transform_feedback;
   functions->PauseTransformFeedback = st_pause_transform_feedback;
   functions->ResumeTransformFeedback = st_resume_transform_feedback;
}


```
## mesa-18 radeonsi驱动streamout 接口

```c
void si_init_streamout_functions(struct si_context *sctx)
{
	sctx->b.create_stream_output_target = si_create_so_target;
	sctx->b.stream_output_target_destroy = si_so_target_destroy;
	sctx->b.set_stream_output_targets = si_set_streamout_targets;
	sctx->atoms.s.streamout_begin.emit = si_emit_streamout_begin;
	sctx->atoms.s.streamout_enable.emit = si_emit_streamout_enable;
}

```

## 关键接口分析

### 设置捕获变量 glTransformFeedbackVaryings

```c
glTransformFeedbackVaryings
_mesa_TransformFeedbackVaryings_no_error(GLuint program, GLsizei count,
   struct gl_shader_program *shProg = _mesa_lookup_shader_program(ctx, program);
   transform_feedback_varyings(ctx, shProg, count, varyings, bufferMode);
         /* allocate new memory for varying names */
         shProg->TransformFeedback.VaryingNames =
            malloc(count * sizeof(GLchar *));
```

### 驱动状态设置  glEnable(GL_RASTERIZER_DISCARD)

```c
_mesa_Enable( GLenum cap )
    _mesa_set_enable( ctx, cap, GL_TRUE );
       switch (cap) {
       ...
       case GL_RASTERIZER_DISCARD:
          if (!_mesa_is_desktop_gl(ctx) && !_mesa_is_gles3(ctx))
             goto invalid_enum_error;
          CHECK_EXTENSION(EXT_transform_feedback, cap);
          if (ctx->RasterDiscard != state) {
             FLUSH_VERTICES(ctx, 0);
             // 设置ST_NEW_RASTERIZER
             ctx->NewDriverState |= ctx->DriverFlags.NewRasterizerDiscard;
                f->NewRasterizerDiscard = ST_NEW_RASTERIZER;
             ctx->RasterDiscard = state;
          }
          break;
        ...
      }
      if (ctx->Driver.Enable) {
         ctx->Driver.Enable( ctx, cap, state );
             functions->Enable = st_Enable;
                 struct st_context *st = st_context(ctx);
                 switch (cap) {
                 case GL_DEBUG_OUTPUT:
                 case GL_DEBUG_OUTPUT_SYNCHRONOUS:
                    st_update_debug_callback(st);
                    break;
                 default:
                    break;
                 }
      }
```


```
st_update_rasterizer(struct st_context *st)
    ...

   /* ST_NEW_RASTERIZER */
   raster->rasterizer_discard = ctx->RasterDiscard;

   cso_set_rasterizer(st->cso_context, raster);
          ...
          cso->data = ctx->pipe->create_rasterizer_state(ctx->pipe, &cso->state);

          iter = cso_insert_state(ctx->cache, hash_key, CSO_RASTERIZER, cso);
```
* 之后通过bind_rasterizer_state 设置脏位之后发射原子函数下发寄存器控制信息

### 变换反馈对象获取 glBindBuffer* 


```
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      if (ctx->Extensions.EXT_transform_feedback) {
         return &ctx->TransformFeedback.CurrentBuffer;
      }

```

### 激活反馈glBeginTransformFeedback

```c

_mesa_BeginTransformFeedback_no_error(GLenum mode)
   begin_transform_feedback(ctx, mode, true);
   obj = ctx->TransformFeedback.CurrentObject;

   /* Figure out what pipeline stage is the source of data for transform
    * feedback.
    */
   source = get_xfb_source(ctx);
       for (i = MESA_SHADER_GEOMETRY; i >= MESA_SHADER_VERTEX; i--) {
           if (ctx->_Shader->CurrentProgram[i] != NULL)
              return ctx->_Shader->CurrentProgram[i];
       }
  
   info = source->sh.LinkedTransformFeedback;

    ...

    // 这一步对于si无作用
   ctx->NewDriverState |= ctx->DriverFlags.NewTransformFeedback;

    // 设置图元模式
   obj->Active = GL_TRUE;
   ctx->TransformFeedback.Mode = mode;

   ...
 
   ctx->Driver.BeginTransformFeedback(ctx, mode, obj);
        [jump st_begin_transform_feedback]
```

#### st_begin_transform_feedback


```c
/* XXX Do we really need the mode? */
static void
st_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
                            struct gl_transform_feedback_object *obj)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct st_transform_feedback_object *sobj =
         st_transform_feedback_object(obj);
   unsigned i, max_num_targets;
   unsigned offsets[PIPE_MAX_SO_BUFFERS] = {0};

   max_num_targets = MIN2(ARRAY_SIZE(sobj->base.Buffers),
                          ARRAY_SIZE(sobj->targets));

   /* Convert the transform feedback state into the gallium representation. */
   for (i = 0; i < max_num_targets; i++) {
      struct st_buffer_object *bo = st_buffer_object(sobj->base.Buffers[i]);

      if (bo && bo->buffer) {
         unsigned stream = obj->program->sh.LinkedTransformFeedback->
            Buffers[i].Stream;

         /* Check whether we need to recreate the target. */
         if (!sobj->targets[i] ||
             sobj->targets[i] == sobj->draw_count[stream] ||
             sobj->targets[i]->buffer != bo->buffer ||
             sobj->targets[i]->buffer_offset != sobj->base.Offset[i] ||
             sobj->targets[i]->buffer_size != sobj->base.Size[i]) {
            /* Create a new target. */
            struct pipe_stream_output_target *so_target =
                  pipe->create_stream_output_target(pipe, bo->buffer,
                                                    sobj->base.Offset[i],
                                                    sobj->base.Size[i]);
                    [jump si_create_so_target]


            pipe_so_target_reference(&sobj->targets[i], NULL);
            sobj->targets[i] = so_target;
         }

         sobj->num_targets = i+1;
      } else {
         pipe_so_target_reference(&sobj->targets[i], NULL);
      }
   }

   /* Start writing at the beginning of each target. */
   cso_set_stream_outputs(st->cso_context, sobj->num_targets,
                          sobj->targets, offsets);

       /* reference new targets */
       for (i = 0; i < num_targets; i++) {
          pipe_so_target_reference(&ctx->so_targets[i], targets[i]);
       }
       /* unref extra old targets, if any */
       for (; i < ctx->nr_so_targets; i++) {
          pipe_so_target_reference(&ctx->so_targets[i], NULL);
 
       pipe->set_stream_output_targets(pipe, num_targets, targets,
                                       offsets);
            [jump si_set_streamout_targets]
       ctx->nr_so_targets = num_targets;
}

```

* 其中当流输出不存在，变换反馈重新激活，变换反馈的缓冲对象更该都会导致新的流输出目标产生

#### 创建流输出目标 si_create_so_target

```c

static struct pipe_stream_output_target *
si_create_so_target(struct pipe_context *ctx,
		    struct pipe_resource *buffer,
		    unsigned buffer_offset,
		    unsigned buffer_size)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_streamout_target *t;
	struct r600_resource *rbuffer = r600_resource(buffer);

	t = CALLOC_STRUCT(si_streamout_target);

	u_suballocator_alloc(allocator=sctx->allocator_zeroed_memory, size=4,alignment=4,
			     out_offset=&t->buf_filled_size_offset,
			     outbuf=(struct pipe_resource**)&t->buf_filled_size);
    
       /* Make sure we have enough space in the buffer. */
       if (!allocator->buffer ||
           allocator->offset + size > allocator->size) {
          /* Allocate a new buffer. */
          allocator->buffer = screen->resource_create(screen, &templ);
          /* Clear the memory if needed. */
          if (allocator->zero_buffer_memory) {
              // transfer_map处理
              ...
          }
       }
       /* Return the buffer. */
       *out_offset = allocator->offset;
       pipe_resource_reference(outbuf, allocator->buffer);
    
       allocator->offset += size;
       return;

	pipe_resource_reference(&t->b.buffer, buffer);

	return &t->b;
}

```
* 关于 rbuffer->valid_buffer_range

```
/* 已初始化的缓冲区范围（使用写传输、流输出、DMA或作为随机访问目标）。
 * 缓冲区的其余部分被视为无效，并可以无同步映射。
 *
 * 这允许对尚未使用的缓冲区范围进行无同步映射。这是为那些忘记使用
 * 无同步映射标志并期望驱动程序解决的应用程序而设计的。
 */
struct util_range valid_buffer_range;

```
* si_create_so_target内部创建了一个si_streamout_target，用来保存变换反馈绑定的缓冲以及向bo申请的填充大小缓冲等信息


#### 设置流输出目标si_set_streamout_targets

```c
/**
 * 设置流输出目标。
 *
 * @param ctx 上下文对象
 * @param num_targets 目标数量
 * @param targets 流输出目标数组
 * @param offsets 流输出目标偏移数组
 */
static void si_set_streamout_targets(struct pipe_context *ctx,
				     unsigned num_targets,
				     struct pipe_stream_output_target **targets,
				     const unsigned *offsets)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_buffer_resources *buffers = &sctx->rw_buffers;
	struct si_descriptors *descs = &sctx->descriptors[SI_DESCS_RW_BUFFERS];
	unsigned old_num_targets = sctx->streamout.num_targets;
	unsigned i, bufidx;

	/* 我们将取消绑定缓冲区。标记需要刷新的缓存。 */
	if (sctx->streamout.num_targets && sctx->streamout.begin_emitted) {
		/* 由于流输出使用经过 TC L2 的矢量写入，而大多数其他客户端也可以使用 TC L2，
		 * 我们不需要刷新它。
		 *
		 * 需要刷新它的情况是 VGT DMA 索引获取（对于 CIK 或更早的 GPU）和间接绘图数据，
		 * 这些情况很少见。因此，在资源中标记 TC L2 的脏状态，并在绘制调用时处理。 */
		for (i = 0; i < sctx->streamout.num_targets; i++)
			if (sctx->streamout.targets[i])
				r600_resource(sctx->streamout.targets[i]->b.buffer)->TC_L2_dirty = true;

		/* 在流输出缓冲区被用作常量缓冲区时，无效标量缓存。 */
		sctx->flags |= SI_CONTEXT_INV_SMEM_L1 |
				 SI_CONTEXT_INV_VMEM_L1 |
				 SI_CONTEXT_VS_PARTIAL_FLUSH;
	}

	/* 所有对流输出目标的读取必须在我们开始写入目标之前完成。 */
	if (num_targets)
		sctx->flags |= SI_CONTEXT_PS_PARTIAL_FLUSH |
		                 SI_CONTEXT_CS_PARTIAL_FLUSH;

	/* 停止流输出。 */
	if (sctx->streamout.num_targets && sctx->streamout.begin_emitted)
		si_emit_streamout_end(sctx);

	/* 设置新目标。 */
	unsigned enabled_mask = 0, append_bitmask = 0;
	for (i = 0; i < num_targets; i++) {
		si_so_target_reference(&sctx->streamout.targets[i], targets[i]);
		if (!targets[i])
			continue;

		si_context_add_resource_size(sctx, targets[i]->buffer);
		enabled_mask |= 1 << i;

		if (offsets[i] == ((unsigned)-1))
			append_bitmask |= 1 << i;
	}

	for (; i < sctx->streamout.num_targets; i++)
		si_so_target_reference(&sctx->streamout.targets[i], NULL);

	sctx->streamout.enabled_mask = enabled_mask;
	sctx->streamout.num_targets = num_targets;
	sctx->streamout.append_bitmask = append_bitmask;

	/* 更新脏状态位。 */
	if (num_targets) {
		si_streamout_buffers_dirty(sctx);
	        si_mark_atom_dirty(sctx, &sctx->atoms.s.streamout_begin);
            [will jump  si_emit_streamout_begin]

	        si_set_streamout_enable(sctx, true);
            	radeon_set_context_reg_seq(sctx->gfx_cs, R_028B94_VGT_STRMOUT_CONFIG, 2);
            	radeon_emit(sctx->gfx_cs,
            		    S_028B94_STREAMOUT_0_EN(si_get_strmout_en(sctx)) |
            		    S_028B94_RAST_STREAM(0) |
            		    S_028B94_STREAMOUT_1_EN(si_get_strmout_en(sctx)) |
            		    S_028B94_STREAMOUT_2_EN(si_get_strmout_en(sctx)) |
            		    S_028B94_STREAMOUT_3_EN(si_get_strmout_en(sctx)));
            	radeon_emit(sctx->gfx_cs,
            		    sctx->streamout.hw_enabled_mask &
            		    sctx->streamout.enabled_stream_buffers_mask);

	} else {
		si_set_atom_dirty(sctx, &sctx->atoms.s.streamout_begin, false);
		si_set_streamout_enable(sctx, false);
	}

	/* 设置着色器资源。*/
	for (i = 0; i < num_targets; i++) {
		bufidx = SI_VS_STREAMOUT_BUF0 + i;

		if (targets[i]) {
			struct pipe_resource *buffer = targets[i]->buffer;
			uint64_t va = r600_resource(buffer)->gpu_address;

			/* 设置描述符。
			 *
			 * 在 VI 中，格式必须是非 INVALID，否则将认为缓冲区未绑定，
			 * 并且存储指令将无效。*/
			uint32_t *desc = descs->list + bufidx*4;
			desc[0] = va;
			desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32);
			desc[2] = 0xffffffff;
			desc[3] = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) |
				  S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
				  S_008F0C_DST_SEL_Z(V_008F0C_SQ_SEL_Z) |
				  S_008F0C_DST_SEL_W(V_008F0C_SQ_SEL_W) |
				  S_008F0C_DATA_FORMAT(V_008F0C_BUF_DATA_FORMAT_32);

			/* 设置资源。 */
			pipe_resource_reference(&buffers->buffers[bufidx],
						buffer);
			radeon_add_to_gfx_buffer_list_check_mem(sctx,
							    r600_resource(buffer),
							    buffers->shader_usage,
							    RADEON_PRIO_SHADER_RW_BUFFER,
							    true);
			r600_resource(buffer)->bind_history |= PIPE_BIND_STREAM_OUTPUT;

			buffers->enabled_mask |= 1u << bufidx;
		} else {
			/* 清除描述符并取消设置资源。 */
			memset(descs->list + bufidx*4, 0,
			       sizeof(uint32_t) * 4);
			pipe_resource_reference(&buffers->buffers[bufidx],
						NULL);
			buffers->enabled_mask &= ~(1u << bufidx);
		}
	}
	for (; i < old_num_targets; i++) {
		bufidx = SI_VS_STREAMOUT_BUF0 + i;
		/* 清除描述符并取消设置资源。 */
		memset(descs->list + bufidx*4, 0, sizeof(uint32_t) * 4);
		pipe_resource_reference(&buffers->buffers[bufidx], NULL);
		buffers->enabled_mask &= ~(1u << bufidx);
	}

	sctx->descriptors_dirty |= 1u << SI_DESCS_RW_BUFFERS;
}

```
* 该函数将绑定的缓冲以SQ_BUF_RSRC_WORD的形式填入rw buffer中的stream_buf* 槽中，同时对已经处理万的流输出终止处理

### 结束变换反馈EndTransformFeedback

```c
_mesa_EndTransformFeedback_no_error(void)
   end_transform_feedback(ctx, ctx->TransformFeedback.CurrentObject);
   ctx->NewDriverState |= ctx->DriverFlags.NewTransformFeedback;
   ctx->Driver.EndTransformFeedback(ctx, obj);
   ctx->TransformFeedback.CurrentObject->Active = GL_FALSE;
   ctx->TransformFeedback.CurrentObject->Paused = GL_FALSE;
   ctx->TransformFeedback.CurrentObject->EndedAnytime = GL_TRUE;

static void
st_end_transform_feedback(struct gl_context *ctx,
                          struct gl_transform_feedback_object *obj)
{
    struct st_context *st = st_context(ctx);
    struct st_transform_feedback_object *sobj =
        st_transform_feedback_object(obj);
    unsigned i;

    cso_set_stream_outputs(st->cso_context, 0, NULL, NULL);

    /* 下一次调用glDrawTransformFeedbackStream应该使用
     * 上一次调用glEndTransformFeedback的顶点计数。
     * 因此，保存每个流的目标。
     *
     * NULL表示顶点计数为0（初始状态）。
     */
    for (i = 0; i < ARRAY_SIZE(sobj->draw_count); i++)
        pipe_so_target_reference(&sobj->draw_count[i], NULL);

    for (i = 0; i < ARRAY_SIZE(sobj->targets); i++) {
        unsigned stream = obj->program->sh.LinkedTransformFeedback->
            Buffers[i].Stream;

        /* 如果未绑定或已为此流设置？ */
        if (!sobj->targets[i] || sobj->draw_count[stream])
            continue;

        pipe_so_target_reference(&sobj->draw_count[stream], sobj->targets[i]);
    }
}

```
* 通过cso_set_stream_outputs(st->cso_context, 0, NULL, NULL) 重置streamout
* 每次调用都会重置sobj->draw_count的引用数，所以下一次激活变换反馈都会创建新的流输出目标

### 获取反馈数据glGetBufferSubDataARB /glMapBuffer* and so forth

```c
_mesa_GetBufferSubData(GLenum target, GLintptr offset,
                       GLsizeiptr size, GLvoid *data)
   ctx->Driver.GetBufferSubData(ctx, offset, size, data, bufObj);
   [jump st_bufferobj_get_subdata ]

/**
 * Called via glGetBufferSubDataARB().
 */
static void
st_bufferobj_get_subdata(struct gl_context *ctx,
                         GLintptrARB offset,
                         GLsizeiptrARB size,
                         void * data, struct gl_buffer_object *obj)
{
   pipe_buffer_read(st_context(ctx)->pipe, st_obj->buffer,
                    offset, size, data);
       struct pipe_transfer *src_transfer;
       ubyte *map;
       map = (ubyte *) pipe_buffer_map_range(pipe,
                                             buf,
                                             offset, size,
                                             PIPE_TRANSFER_READ,
                                             &src_transfer);
               struct pipe_box box;
               void *map;
               u_box_1d(offset, length, &box);
               map = pipe->transfer_map(pipe, buffer, 0, access, &box, transfer);
               if (!map) {
                  return NULL;
               }
               return map;

       if (!map)
          return;
       memcpy(data, map, size);
       pipe_buffer_unmap(pipe, src_transfer);
}

```

## shader编译对xfb的处理

### ast xfb定义
```c

struct _mesa_glsl_parse_state {

   /**
    * Output layout qualifiers from GLSL 1.50 (geometry shader controls),
    * and GLSL 4.00 (tessellation control shader).
    */
   struct ast_type_qualifier *out_qualifier;
}


struct ast_type_qualifier {
   DECLARE_RALLOC_CXX_OPERATORS(ast_type_qualifier);

   // 注意: 此bitset的位数必须至少与下面'q'结构体的标志位数相同。先前，大小为128而不是96。
   // 但是在GCC 5.4.0中存在的一个错误导致在其他地方生成错误的SSE代码，导致崩溃。96位解决了此问题。
   // 请参阅https://bugs.freedesktop.org/show_bug.cgi?id=105497
   DECLARE_BITSET_T(bitset_t, 96);
   union flags {
      struct {
         ...
        //GL_ARB_enhanced_layouts的布局限定符 */
         unsigned explicit_xfb_offset:1; /**< 由着色器代码显式指定的xfb_offset值 */
         unsigned xfb_buffer:1; /**< 是否分配了xfb_buffer值 */
         unsigned explicit_xfb_buffer:1; /**< 由着色器代码显式指定的xfb_buffer值 */
         unsigned xfb_stride:1; /**< xfb_stride值是否尚未与全局值合并 */
         unsigned explicit_xfb_stride:1; /**< 由着色器代码显式指定的xfb_stride值 */

         ...

      }
      q;

      bitset_t i; // 以位掩码形式访问的标志集合
   } flags;

   ...
   // 通过GL_ARB_enhaced_layouts指定的组件
   // 注意：仅在explicit_component被设置时此字段有效。
   ast_expression *component;

   // 通过GL_ARB_enhanced_layouts关键字指定的xfb_buffer。
   ast_expression *xfb_buffer;

   // 通过GL_ARB_enhanced_layouts关键字指定的xfb_stride。
   ast_expression *xfb_stride;

   // 每个缓冲区的全局xfb_stride值。
   ast_layout_expression *out_xfb_stride[MAX_FEEDBACK_BUFFERS];

   // 通过GL_ARB_shader_atomic_counter的或GL_ARB_enhanced_layouts的"offset"关键字指定的偏移量，
   // 或通过GL_ARB_enhanced_layouts的"xfb_offset"关键字指定的偏移量。
   // 注意：仅在explicit_offset被设置时此字段有效。
   ast_expression *offset;


   bool merge_qualifier(YYLTYPE *loc,
			_mesa_glsl_parse_state *state,
                        const ast_type_qualifier &q,
                        bool is_single_layout_merge,
                        bool is_multiple_layouts_merge = false);
   ...
};
```
### 合并限定符对字段的处理


```c
bool
ast_type_qualifier::merge_qualifier(YYLTYPE *loc,
                                    _mesa_glsl_parse_state *state,
                                    const ast_type_qualifier &q,
                                    bool is_single_layout_merge,
                                    bool is_multiple_layouts_merge)
{

   ast_type_qualifier stream_layout_mask;
   stream_layout_mask.flags.i = 0;
   stream_layout_mask.flags.q.stream = 1;

   ...
   if (state->has_enhanced_layouts()) {
      if (!this->flags.q.explicit_xfb_buffer) {
         if (q.flags.q.xfb_buffer) {
            this->flags.q.xfb_buffer = 1;
            this->xfb_buffer = q.xfb_buffer;
         } else if (!this->flags.q.xfb_buffer && this->flags.q.out &&
                    !this->flags.q.in) {
            /* Assign global xfb_buffer value */
            this->flags.q.xfb_buffer = 1;
            this->xfb_buffer = state->out_qualifier->xfb_buffer;
         }
      }

      if (q.flags.q.explicit_xfb_stride) {
         this->flags.q.xfb_stride = 1;
         this->flags.q.explicit_xfb_stride = 1;
         this->xfb_stride = q.xfb_stride;
      }
   }
}

```
### 全局xfb_stride在gl_shader的保存

```c

set_shader_inout_layout(struct gl_shader *shader,
		     struct _mesa_glsl_parse_state *state)
   for (unsigned i = 0; i < MAX_FEEDBACK_BUFFERS; i++) {
      if (state->out_qualifier->out_xfb_stride[i]) {
         unsigned xfb_stride;
         if (state->out_qualifier->out_xfb_stride[i]->
                process_qualifier_constant(state, "xfb_stride", &xfb_stride,
                true)) {
            shader->TransformFeedbackBufferStride[i] = xfb_stride;
         }
      }
   }

```

#### glsl IR生成时对xfb_stride的处理

```c

static void
set_shader_inout_layout(struct gl_shader *shader,
		     struct _mesa_glsl_parse_state *state)
{
   ...
   for (unsigned i = 0; i < MAX_FEEDBACK_BUFFERS; i++) {
      if (state->out_qualifier->out_xfb_stride[i]) {
         unsigned xfb_stride;
         if (state->out_qualifier->out_xfb_stride[i]->
                process_qualifier_constant(state, "xfb_stride", &xfb_stride,
                true)) {
            shader->TransformFeedbackBufferStride[i] = xfb_stride;
         }
      }
   }
   ...
}


```


### ast 对接口块中的xfb的处理

```c
_mesa_ast_process_interface_block(YYLTYPE *locp,
```

### GLSL IR 生成时xfb的处理

#### IR 变量中xfb的表示形式

```c++
class ir_variable : public ir_instruction {
public:
    ...
public:
   struct ir_variable_data {
      ...
      /**
       * Is this varying used only by transform feedback?
       *
       * This is used by the linker to decide if its safe to pack the varying.
       */
      unsigned is_xfb_only:1;

      /**
       * Was a transfor feedback buffer set in the shader?
       */
      unsigned explicit_xfb_buffer:1;

      /**
       * Was a transfor feedback offset set in the shader?
       */
      unsigned explicit_xfb_offset:1;

      /**
       * Was a transfor feedback stride set in the shader?
       */
      unsigned explicit_xfb_stride:1;

      /**
       * Transform feedback buffer.
       */
      unsigned xfb_buffer;

      /**
       * Transform feedback stride.
       */
      unsigned xfb_stride;

      /**
       * Allow (only) ir_variable direct access private members.
       */
      friend class ir_variable;
   } data;
   ...
};

```

### ast -> hir 对 xfb的处理

```c++
ir_rvalue *
ast_declarator_list::hir(exec_list *instructions,
                         struct _mesa_glsl_parse_state *state)
{
    ...

    // foreach 处理每个变量see shader.pdf
    apply_layout_qualifier_to_variable(&this->type->qualifier, var, state,
                                         &loc);
        ...
       if (qual->flags.q.out && qual->flags.q.xfb_buffer) {
          unsigned qual_xfb_buffer;
          if (process_qualifier_constant(state, loc, "xfb_buffer",
                                         qual->xfb_buffer, &qual_xfb_buffer) &&
              validate_xfb_buffer_qualifier(loc, state, qual_xfb_buffer)) {
             var->data.xfb_buffer = qual_xfb_buffer;
             if (qual->flags.q.explicit_xfb_buffer)
                var->data.explicit_xfb_buffer = true;
          }
       }
    
       if (qual->flags.q.explicit_xfb_offset) {
          unsigned qual_xfb_offset;
          unsigned component_size = var->type->contains_double() ? 8 : 4;
    
          if (process_qualifier_constant(state, loc, "xfb_offset",
                                         qual->offset, &qual_xfb_offset) &&
              validate_xfb_offset_qualifier(loc, state, (int) qual_xfb_offset,
                                            var->type, component_size)) {
             var->data.offset = qual_xfb_offset;
             var->data.explicit_xfb_offset = true;
          }
       }
    
       if (qual->flags.q.explicit_xfb_stride) {
          unsigned qual_xfb_stride;
          if (process_qualifier_constant(state, loc, "xfb_stride",
                                         qual->xfb_stride, &qual_xfb_stride)) {
             var->data.xfb_stride = qual_xfb_stride;
             var->data.explicit_xfb_stride = true;
          }
       }
    
 
}

```
##  shader链接时对xfb的处理

### hir-> gl_shader_program 链接varying时对变换反馈的处理

#### 链接同一shader类型时对xfb_stride的处理

```c

// previous call is link_intrastage_shaders
static void
link_xfb_stride_layout_qualifiers(struct gl_context *ctx,
                                  struct gl_shader_program *prog,
                                  struct gl_shader **shader_list,
                                  unsigned num_shaders)
{
   for (unsigned i = 0; i < MAX_FEEDBACK_BUFFERS; i++) {
      prog->TransformFeedback.BufferStride[i] = 0;
   }

   for (unsigned i = 0; i < num_shaders; i++) {
      struct gl_shader *shader = shader_list[i];

      for (unsigned j = 0; j < MAX_FEEDBACK_BUFFERS; j++) {
         if (shader->TransformFeedbackBufferStride[j]) {
            if (prog->TransformFeedback.BufferStride[j] == 0) {
               prog->TransformFeedback.BufferStride[j] =
                  shader->TransformFeedbackBufferStride[j];
               if (!validate_xfb_buffer_stride(ctx, j, prog))
                  return;
            } else if (prog->TransformFeedback.BufferStride[j] !=
                       shader->TransformFeedbackBufferStride[j]){
               linker_error(prog,
                            "intrastage shaders defined with conflicting "
                            "xfb_stride for buffer %d (%d and %d)\n", j,
                            prog->TransformFeedback.BufferStride[j],
                            shader->TransformFeedbackBufferStride[j]);
               return;
            }
         }
      }
   }
}

```
* 暂时无需重点关注


#### 不同shader类型之间链接varyings对xfb的处理

```c

// previous call is from link_shaders
bool
link_varyings(struct gl_shader_program *prog, unsigned first, unsigned last,
              struct gl_context *ctx, void *mem_ctx)
{
   // 检查是否存在变量使用了变换反馈布局限定符
   bool has_xfb_qualifiers = false;
   unsigned num_tfeedback_decls = 0;
   char **varying_names = NULL;
   tfeedback_decl *tfeedback_decls = NULL;

   /* 从ARB_enhanced_layouts规范：
    *
    *    "如果用于记录变换反馈输出变量的着色器使用了"xfb_buffer"、"xfb_offset"或"xfb_stride"布局
    *    限定符，则忽略TransformFeedbackVaryings指定的值，而是从指定的布局限定符派生用于变换反馈的变量集。"
    */
   for (int i = MESA_SHADER_FRAGMENT - 1; i >= 0; i--) {
      /* 找到片段着色器之前的最后一个阶段 */
      if (prog->_LinkedShaders[i]) {
         // 遍历ir获取num_tfeedback_decls, varying_names
         has_xfb_qualifiers =
            process_xfb_layout_qualifiers(mem_ctx, prog->_LinkedShaders[i],
                                          prog, &num_tfeedback_decls,
                                          &varying_names);
         break;
      }
   }

    // 获取OpenGL api接口指定的varyings see _mesa_TransformFeedbackVaryings_no_error
   if (!has_xfb_qualifiers) {
      num_tfeedback_decls = prog->TransformFeedback.NumVarying;
      varying_names = prog->TransformFeedback.VaryingNames;
   }

   if (num_tfeedback_decls != 0) {
      /* 从GL_EXT_transform_feedback:
       *   如果：
       *
       *   * TransformFeedbackVaryingsEXT指定的<count>非零，但程序对象没有顶点或几何着色器
       */
      if (first >= MESA_SHADER_FRAGMENT) {
         linker_error(prog, "指定了变换反馈变量，但没有顶点、镶嵌或几何着色器存在。\n");
         return false;
      }

      tfeedback_decls = rzalloc_array(mem_ctx, tfeedback_decl,
                                      num_tfeedback_decls);
      if (!parse_tfeedback_decls(ctx, prog, mem_ctx, num_tfeedback_decls,
                                 varying_names, tfeedback_decls))
            [jump parse_tfeedback_decls]
         return false;
   }

   /* 如果没有片段着色器，我们需要设置变换反馈。
    *
    * 对于SSO，我们还需要分配输出位置。我们在这里进行分配
    * 因为我们需要为单阶段程序和多阶段程序都这样做。
    */
   if (last < MESA_SHADER_FRAGMENT &&
       (num_tfeedback_decls != 0 || prog->SeparateShader)) {
      const uint64_t reserved_out_slots =
         reserved_varying_slot(prog->_LinkedShaders[last], ir_var_shader_out);
      if (!assign_varying_locations(ctx, mem_ctx, prog,
                                    prog->_LinkedShaders[last], NULL,
                                    num_tfeedback_decls, tfeedback_decls,
                                    reserved_out_slots))
         return false;
   }

   // 存储变换反馈信息
   if (!store_tfeedback_info(ctx, prog, num_tfeedback_decls, tfeedback_decls,
                             has_xfb_qualifiers))
      return false;

   return true;
}
```


#### 解析shader里的变换反馈声明

```c
/**
 * 解析通过glTransformFeedbackVaryings()传递的所有变换反馈声明，并将它们存储在tfeedback_decl对象中。
 *
 * 如果发生错误，通过linker_error()报告错误并返回false。
 */
static bool
parse_tfeedback_decls(struct gl_context *ctx, struct gl_shader_program *prog,
                      const void *mem_ctx, unsigned num_names,
                      char **varying_names, tfeedback_decl *decls)
{
   for (unsigned i = 0; i < num_names; ++i) {
      // 为每个变量名初始化一个对应的tfeedback_decl对象
      decls[i].init(ctx, mem_ctx, varying_names[i]);

      // 如果不是一个有效的变换反馈变量，继续下一个
      if (!decls[i].is_varying())
         continue;

      /* 从GL_EXT_transform_feedback中引用:
       *   如果：
       *   * <varyings>数组中的任意两个条目指定相同的变量，
       *     则程序链接失败；
       *
       * 我们解释这意味着“<varyings>数组中的任意两个条目指定相同的变量和数组索引”，
       * 因为否则数组的变换反馈将毫无意义。
       */
      for (unsigned j = 0; j < i; ++j) {
         if (decls[j].is_varying()) {
            // 如果发现变量声明重复，报告链接错误并返回false
            if (tfeedback_decl::is_same(decls[i], decls[j])) {
               linker_error(prog, "Transform feedback varying %s 指定了多次。",
                            varying_names[i]);
               return false;
            }
         }
      }
   }
   return true;
}

```

* 这段代码是用于解析通过`glTransformFeedbackVaryings()`传递的所有变换反馈声明，并将它们存储在`tfeedback_decl`对象中。如果发生错误，将通过`linker_error()`报告错误，并返回`false`。

* 具体而言，它遍历了传递的变量名数组，并为每个变量名初始化一个对应的`tfeedback_decl`对象。然后，它检查是否有重复的变量声明，如果发现重复，则报告链接错误并返回`false`。这个检查确保了在变量数组中没有重复声明相同的变量和数组索引，因为对数组的变换反馈否则将没有意义。

* 总体来说，这段代码的目的是确保变换反馈声明的正确性，并在发现问题时报告错误。

#### 变换反馈信息存储到sh.LinkedTransformFeedback

```cpp
/**
 * 将变换反馈位置分配存储到prog->sh.LinkedTransformFeedback中，基于tfeedback_decls中存储的数据。
 *
 * 如果发生错误，通过linker_error()报告错误并返回false。
 */
static bool
store_tfeedback_info(struct gl_context *ctx, struct gl_shader_program *prog,
                     unsigned num_tfeedback_decls,
                     tfeedback_decl *tfeedback_decls, bool has_xfb_qualifiers)
{
   // 如果没有顶点着色器程序，直接返回true
   if (!prog->last_vert_prog)
      return true;

   // 确保MaxTransformFeedbackBuffers小于32，以防止用于跟踪缓冲区数量的位掩码溢出
   assert(ctx->Const.MaxTransformFeedbackBuffers < 32);

   // 检查是否采用GL_SEPARATE_ATTRIBS模式
   bool separate_attribs_mode =
      prog->TransformFeedback.BufferMode == GL_SEPARATE_ATTRIBS;

   // 获取最后一个顶点着色器程序
   struct gl_program *xfb_prog = prog->last_vert_prog;
   xfb_prog->sh.LinkedTransformFeedback =
      rzalloc(xfb_prog, struct gl_transform_feedback_info);

   // 如果存在xfb_offset限定符，不一定要按升序使用，但有些驱动程序希望按顺序接收变换反馈声明的列表，所以现在对其进行排序以方便处理
   if (has_xfb_qualifiers) {
      qsort(tfeedback_decls, num_tfeedback_decls, sizeof(*tfeedback_decls),
            cmp_xfb_offset);
   }

   // 初始化LinkedTransformFeedback中的Varyings和Outputs数组
   xfb_prog->sh.LinkedTransformFeedback->Varyings =
      rzalloc_array(xfb_prog, struct gl_transform_feedback_varying_info,
                    num_tfeedback_decls);
   unsigned num_outputs = 0;
   for (unsigned i = 0; i < num_tfeedback_decls; ++i) {
      if (tfeedback_decls[i].is_varying_written())
         num_outputs += tfeedback_decls[i].get_num_outputs();
   }
   xfb_prog->sh.LinkedTransformFeedback->Outputs =
      rzalloc_array(xfb_prog, struct gl_transform_feedback_output,
                    num_outputs);

   unsigned num_buffers = 0;
   unsigned buffers = 0;

   // 如果没有xfb_offset限定符并且采用GL_SEPARATE_ATTRIBS模式
   if (!has_xfb_qualifiers && separate_attribs_mode) {
      /* GL_SEPARATE_ATTRIBS模式 */
      for (unsigned i = 0; i < num_tfeedback_decls; ++i) {
         if (!tfeedback_decls[i].store(ctx, prog,
                                       xfb_prog->sh.LinkedTransformFeedback,
                                       num_buffers, num_buffers, num_outputs,
                                       NULL, has_xfb_qualifiers))
            return false;

         buffers |= 1 << num_buffers;
         num_buffers++;
      }
   }
   else {
      /* GL_INVERLEAVED_ATTRIBS模式 */
      int buffer_stream_id = -1;
      unsigned buffer =
         num_tfeedback_decls ? tfeedback_decls[0].get_buffer() : 0;
      bool explicit_stride[MAX_FEEDBACK_BUFFERS] = { false };

      /* 应用任何xfb_stride全局限定符 */
      if (has_xfb_qualifiers) {
         for (unsigned j = 0; j < MAX_FEEDBACK_BUFFERS; j++) {
            if (prog->TransformFeedback.BufferStride[j]) {
               explicit_stride[j] = true;
               xfb_prog->sh.LinkedTransformFeedback->Buffers[j].Stride =
                  prog->TransformFeedback.BufferStride[j] / 4;
            }
         }
      }

      for (unsigned i = 0; i < num_tfeedback_decls; ++i) {
         if (has_xfb_qualifiers &&
             buffer != tfeedback_decls[i].get_buffer()) {
            /* 移动到下一个缓冲区，重置流ID */
            buffer_stream_id = -1;
            num_buffers++;
         }

         if (tfeedback_decls[i].is_next_buffer_separator()) {
            if (!tfeedback_decls[i].store(ctx, prog,
                                          xfb_prog->sh.LinkedTransformFeedback,
                                          buffer, num_buffers, num_outputs,
                                          explicit_stride, has_xfb_qualifiers))
               return false;
            num_buffers++;
            buffer_stream_id = -1;
            continue;
         }

         if (has_xfb_qualifiers) {
            buffer = tfeedback_decls[i].get_buffer();
         } else {
            buffer = num_buffers;
         }

         if (tfeedback_decls[i].is_varying()) {
            if (buffer_stream_id == -1)  {
               /* 第一个写入此缓冲区的变量：记住它的流ID */
               buffer_stream_id = (int) tfeedback_decls[i].get_stream_id();

               /* 仅当有变量附加到它时，才标记缓冲区为活动状态。这种行为基于GL 4.6规范第13.2.2节的修订版本。 */
               buffers |= 1 << buffer;
            } else if (buffer_stream_id !=
                       (int) tfeedback_decls[i].get_stream_id()) {
               /* 写入相同缓冲区的变量来自不同流 */
               linker_error(prog,
                            "Transform feedback不能在单个缓冲区中捕获属于不同顶点流的变量。"
                            "变量%s写入流%u的缓冲区，同一缓冲区中的其他变量写入流%u。",
                            tfeedback_decls[i].name(),
                            tfeedback_decls[i].get_stream_id(),
                            buffer_stream_id);
               return false;
            }
         }

         if (!tfeedback_decls[i].store(ctx, prog,
                                       xfb_prog->sh.LinkedTransformFeedback,
                                       buffer, num_buffers, num_outputs,
                                       explicit_stride, has_xfb_qualifiers))
            return false;
      }
   }

   // 确保LinkedTransformFeedback中的NumOutputs等于num_outputs
   assert(xfb_prog->sh.LinkedTransformFeedback->NumOutputs == num_outputs);

   // 设置LinkedTransformFeedback的ActiveBuffers字段
   xfb_prog->sh.LinkedTransformFeedback->ActiveBuffers = buffers;
   return true;
}
```

* 这段代码的主要目的是将变换反馈位置分配存储到`prog->sh.LinkedTransformFeedback`中，基于存储在`tfeedback_decls`中的数据。如果发生错误，通过`linker_error()`报告错误并返回`false`。


#### gl_shader_program ->  tgsi

```c
/**
 * Translate a vertex program.
 */
bool
st_translate_vertex_program(struct st_context *st,
                            struct st_vertex_program *stvp)
{
      st_translate_stream_output_info(stvp->glsl_to_tgsi,
                                      stvp->result_to_output,
                                      &stvp->tgsi.stream_output);
           ---------------------------------------------------------
           struct gl_transform_feedback_info *info =
              glsl_to_tgsi->shader_program->last_vert_prog->sh.LinkedTransformFeedback;
           st_translate_stream_output_info2(info, outputMapping, so);

}


void
st_translate_stream_output_info2(struct gl_transform_feedback_info *info,
                                const ubyte outputMapping[],
                                struct pipe_stream_output_info *so)
{
   unsigned i;

   for (i = 0; i < info->NumOutputs; i++) {
      so->output[i].register_index =
         outputMapping[info->Outputs[i].OutputRegister];
      so->output[i].start_component = info->Outputs[i].ComponentOffset;
      so->output[i].num_components = info->Outputs[i].NumComponents;
      so->output[i].output_buffer = info->Outputs[i].OutputBuffer;
      so->output[i].dst_offset = info->Outputs[i].DstOffset;
      so->output[i].stream = info->Outputs[i].StreamId;
   }

    // PIPE_MAX_SO_BUFFERS = 4
   for (i = 0; i < PIPE_MAX_SO_BUFFERS; i++) {
      so->stride[i] = info->Buffers[i].Stride;
   }
   so->num_outputs = info->NumOutputs;
}
```
* 将处理后信息保存到pipe_shader_state.stream_output中,最终在生成selector时， 保存到selector中的so中

//### tgsi -> tgsi_shader_info对streamout 维度的处理
//
//```c
//scan_declaration(struct tgsi_shader_info *info,
//                 const struct tgsi_full_declaration *fulldecl)
//
//      ...
//      case TGSI_FILE_OUTPUT:
//           info->output_semantic_name[reg] = (ubyte) semName;
//           info->output_semantic_index[reg] = (ubyte) semIndex;
//           info->output_usagemask[reg] |= fulldecl->Declaration.UsageMask;
//           info->num_outputs = MAX2(info->num_outputs, reg + 1);
//  
//           if (fulldecl->Declaration.UsageMask & TGSI_WRITEMASK_X) {
//              info->output_streams[reg] |= (ubyte)fulldecl->Semantic.StreamX;
//              info->num_stream_output_components[fulldecl->Semantic.StreamX]++;
//           }
//           if (fulldecl->Declaration.UsageMask & TGSI_WRITEMASK_Y) {
//              info->output_streams[reg] |= (ubyte)fulldecl->Semantic.StreamY << 2;
//              info->num_stream_output_components[fulldecl->Semantic.StreamY]++;
//           }
//           if (fulldecl->Declaration.UsageMask & TGSI_WRITEMASK_Z) {
//              info->output_streams[reg] |= (ubyte)fulldecl->Semantic.StreamZ << 4;
//              info->num_stream_output_components[fulldecl->Semantic.StreamZ]++;
//           }
//           if (fulldecl->Declaration.UsageMask & TGSI_WRITEMASK_W) {
//              info->output_streams[reg] |= (ubyte)fulldecl->Semantic.StreamW << 6;
//              info->num_stream_output_components[fulldecl->Semantic.StreamW]++;
//           }
//  
//           switch (semName) {
//           case TGSI_SEMANTIC_PRIMID:
//              info->writes_primid = true;
//              break;
//           case TGSI_SEMANTIC_VIEWPORT_INDEX:
//              info->writes_viewport_index = true;
//              break;
//           case TGSI_SEMANTIC_LAYER:
//              info->writes_layer = true;
//              break;
//           case TGSI_SEMANTIC_PSIZE:
//              info->writes_psize = true;
//              break;
//           case TGSI_SEMANTIC_CLIPVERTEX:
//              info->writes_clipvertex = true;
//              break;
//           case TGSI_SEMANTIC_COLOR:
//              info->colors_written |= 1 << semIndex;
//              break;
//           case TGSI_SEMANTIC_STENCIL:
//              info->writes_stencil = true;
//              break;
//           case TGSI_SEMANTIC_SAMPLEMASK:
//              info->writes_samplemask = true;
//              break;
//           case TGSI_SEMANTIC_EDGEFLAG:
//              info->writes_edgeflag = true;
//              break;
//           case TGSI_SEMANTIC_POSITION:
//              if (procType == PIPE_SHADER_FRAGMENT)
//                 info->writes_z = true;
//              else
//                 info->writes_position = true;
//              break;
//           }
//           break;
//      ...
//      }
//      ...
//```

## shader LLVM IR 对xfb streamout 处理

可发射数据的有vs, tes, gs_copy_shader
gs 本身不处理streamout交给gs_copy_shader 处理
vs和tes 发射数据的接口为si_llvm_emit_vs_epilogue

### vs 的streamout参数声明

```c
static void declare_streamout_params(struct si_shader_context *ctx,
				     struct pipe_stream_output_info *so,
				     struct si_function_info *fninfo)
{
	int i;

	/* Streamout SGPRs. */
	if (so->num_outputs) {
		if (ctx->type != PIPE_SHADER_TESS_EVAL)
			ctx->param_streamout_config = add_arg(fninfo, ARG_SGPR, ctx->ac.i32);
		else
			ctx->param_streamout_config = fninfo->num_params - 1;

		ctx->param_streamout_write_index = add_arg(fninfo, ARG_SGPR, ctx->ac.i32);
	}
	/* A streamout buffer offset is loaded if the stride is non-zero. */
	for (i = 0; i < 4; i++) {
		if (!so->stride[i])
			continue;

		ctx->param_streamout_offset[i] = add_arg(fninfo, ARG_SGPR, ctx->ac.i32);
	}
}

```

关于vs可总结如下
```
|  |i32 | param_streamout_config | SGPR| |
|  |i32 | param_streamout_write_index | SGPR| |
|  |i32 | param_streamout_offset[0] | SGPR| |
|  |i32 | param_streamout_offset[1] | SGPR| |
|  |i32 | param_streamout_offset[2] | SGPR| |
|  |i32 | param_streamout_offset[3] |
```
其中param_streamout_*等参数来自vgt的寄存器配置


### HW vs streamout 输出 si_llvm_emit_vs_epilogue

```c
static void si_llvm_emit_vs_epilogue(struct ac_shader_abi *abi,
				     unsigned max_outputs,
				     LLVMValueRef *addrs)
{
	struct si_shader_context *ctx = si_shader_context_from_abi(abi);
	struct tgsi_shader_info *info = &ctx->shader->selector->info;
	struct si_shader_output_values *outputs = NULL;
	int i,j;

	assert(!ctx->shader->is_gs_copy_shader);
	assert(info->num_outputs <= max_outputs);

	outputs = MALLOC((info->num_outputs + 1) * sizeof(outputs[0]));

	/* Vertex color clamping.
	 *
	 * This uses a state constant loaded in a user data SGPR and
	 * an IF statement is added that clamps all colors if the constant
	 * is true.
	 */
     ...
     ...

	for (i = 0; i < info->num_outputs; i++) {
		outputs[i].semantic_name = info->output_semantic_name[i];
		outputs[i].semantic_index = info->output_semantic_index[i];

		for (j = 0; j < 4; j++) {
			outputs[i].values[j] =
				LLVMBuildLoad(ctx->ac.builder,
					      addrs[4 * i + j],
					      "");
			outputs[i].vertex_stream[j] =
				(info->output_streams[i] >> (2 * j)) & 3;
		}
	}

	if (ctx->shader->selector->so.num_outputs)
		si_llvm_emit_streamout(ctx, outputs, i, 0);

	/* Export PrimitiveID. */
	if (ctx->shader->key.mono.u.vs_export_prim_id) {
		outputs[i].semantic_name = TGSI_SEMANTIC_PRIMID;
		outputs[i].semantic_index = 0;
		outputs[i].values[0] = ac_to_float(&ctx->ac, get_primitive_id(ctx, 0));
		for (j = 1; j < 4; j++)
			outputs[i].values[j] = LLVMConstReal(ctx->f32, 0);

		memset(outputs[i].vertex_stream, 0,
		       sizeof(outputs[i].vertex_stream));
		i++;
	}

	si_llvm_export_vs(ctx, outputs, i);
	FREE(outputs);
}



```cpp
/**
 * 将流输出数据写入顶点流@p stream（GS复制着色器可能会有不同的顶点流）的缓冲区。
 */
static void si_llvm_emit_streamout(struct si_shader_context *ctx,
				   struct si_shader_output_values *outputs,
				   unsigned noutput, unsigned stream)
{
	struct si_shader_selector *sel = ctx->shader->selector;
	struct pipe_stream_output_info *so = &sel->so;
	LLVMBuilderRef builder = ctx->ac.builder;
	int i;
	struct lp_build_if_state if_ctx;

	/* 获取位[22:16]，即 (so_param >> 16) & 127； */
	LLVMValueRef so_vtx_count =
		si_unpack_param(ctx, ctx->param_streamout_config, 16, 7);

	LLVMValueRef tid = ac_get_thread_id(&ctx->ac);

	/* can_emit = tid < so_vtx_count; */
	LLVMValueRef can_emit =
		LLVMBuildICmp(builder, LLVMIntULT, tid, so_vtx_count, "");

	/* 有条件地生成流输出代码。实际上，这样可以避免越界缓冲区访问。
	 * 硬件通过SGPR（so_vtx_count）告诉我们哪些线程被允许发射流输出数据。 */
	lp_build_if(&if_ctx, &ctx->gallivm, can_emit);
	{
		/* 计算缓冲区偏移量：
		 *   ByteOffset = streamout_offset[buffer_id]*4 +
		 *                (streamout_write_index + thread_id)*stride[buffer_id] +
		 *                attrib_offset
		 */

		LLVMValueRef so_write_index =
			LLVMGetParam(ctx->main_fn,
				     ctx->param_streamout_write_index);

		/* 计算（streamout_write_index + thread_id）。 */
		so_write_index = LLVMBuildAdd(builder, so_write_index, tid, "");

		/* 加载描述符并计算每个已启用缓冲区的写入偏移量。 */
		LLVMValueRef so_write_offset[4] = {};
		LLVMValueRef so_buffers[4];
		LLVMValueRef buf_ptr = LLVMGetParam(ctx->main_fn,
						    ctx->param_rw_buffers);

		for (i = 0; i < 4; i++) {
			if (!so->stride[i])
				continue;

			LLVMValueRef offset = LLVMConstInt(ctx->i32,
							   SI_VS_STREAMOUT_BUF0 + i, 0);

			so_buffers[i] = ac_build_load_to_sgpr(&ctx->ac, buf_ptr, offset);

			LLVMValueRef so_offset = LLVMGetParam(ctx->main_fn,
							      ctx->param_streamout_offset[i]);
			so_offset = LLVMBuildMul(builder, so_offset, LLVMConstInt(ctx->i32, 4, 0), "");

			so_write_offset[i] = ac_build_imad(&ctx->ac, so_write_index,
							   LLVMConstInt(ctx->i32, so->stride[i]*4, 0),
							   so_offset);
		}

		/* 写入流输出数据。 */
		for (i = 0; i < so->num_outputs; i++) {
			unsigned reg = so->output[i].register_index;

			if (reg >= noutput)
				continue;

			if (stream != so->output[i].stream)
				continue;

			emit_streamout_output(ctx, so_buffers, so_write_offset,
					      &so->output[i], &outputs[reg]);
		}
	}
	lp_build_endif(&if_ctx);
}


static void emit_streamout_output(struct si_shader_context *ctx,
				  LLVMValueRef const *so_buffers,
				  LLVMValueRef const *so_write_offsets,
				  struct pipe_stream_output *stream_out,
				  struct si_shader_output_values *shader_out)
{
	unsigned buf_idx = stream_out->output_buffer;
	unsigned start = stream_out->start_component;
	unsigned num_comps = stream_out->num_components;
	LLVMValueRef out[4];

	assert(num_comps && num_comps <= 4);
	if (!num_comps || num_comps > 4)
		return;

	/* Load the output as int. */
	for (int j = 0; j < num_comps; j++) {
		assert(stream_out->stream == shader_out->vertex_stream[start + j]);

		out[j] = ac_to_integer(&ctx->ac, shader_out->values[start + j]);
	}

	/* Pack the output. */
	LLVMValueRef vdata = NULL;

	switch (num_comps) {
	case 1: /* as i32 */
		vdata = out[0];
		break;
	case 2: /* as v2i32 */
	case 3: /* as v4i32 (aligned to 4) */
		out[3] = LLVMGetUndef(ctx->i32);
		/* fall through */
	case 4: /* as v4i32 */
		vdata = ac_build_gather_values(&ctx->ac, out, util_next_power_of_two(num_comps));
		break;
	}

	ac_build_buffer_store_dword(&ctx->ac, so_buffers[buf_idx],
				    vdata, num_comps,
				    so_write_offsets[buf_idx],
				    ctx->i32_0,
				    stream_out->dst_offset * 4, 1, 1, true, false);
}


```

* 最后处理后的数据写入rw buffer中的SI_VS_STREAMOUT_BUF*中


### gs_copy_shader中 对streamout数据的导出

```c
/* Generate code for the hardware VS shader stage to go with a geometry shader */
struct si_shader *
si_generate_gs_copy_shader(struct si_screen *sscreen,
			   struct ac_llvm_compiler *compiler,
			   struct si_shader_selector *gs_selector,
			   struct pipe_debug_callback *debug)
{
    ...

	if (gs_selector->so.num_outputs)
		stream_id = si_unpack_param(&ctx, ctx.param_streamout_config, 24, 2);
	else
		stream_id = ctx.i32_0;

	/* Fill in output information. */
	for (i = 0; i < gsinfo->num_outputs; ++i) {
		outputs[i].semantic_name = gsinfo->output_semantic_name[i];
		outputs[i].semantic_index = gsinfo->output_semantic_index[i];

        // 写入分量，vertex_stream[4]
		for (int chan = 0; chan < 4; chan++) {
			outputs[i].vertex_stream[chan] =
				(gsinfo->output_streams[i] >> (2 * chan)) & 3;
		}
	}

    ...
    ...

	for (int stream = 0; stream < 4; stream++) {
		LLVMBasicBlockRef bb;
		unsigned offset;

		if (!gsinfo->num_stream_output_components[stream])
			continue;

		if (stream > 0 && !gs_selector->so.num_outputs)
			continue;

		/* Fetch vertex data from GSVS ring */
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

		/* Streamout and exports. */
		if (gs_selector->so.num_outputs) {
			si_llvm_emit_streamout(&ctx, outputs,
					       gsinfo->num_outputs,
					       stream);
		}

		if (stream == 0) {
			/* Vertex color clamping.
			 *
			 * This uses a state constant loaded in a user data SGPR and
			 * an IF statement is added that clamps all colors if the constant
			 * is true.
			 */
			struct lp_build_if_state if_ctx;
			LLVMValueRef v[2], cond = NULL;
			LLVMBasicBlockRef blocks[2];

			for (unsigned i = 0; i < gsinfo->num_outputs; i++) {
				if (gsinfo->output_semantic_name[i] != TGSI_SEMANTIC_COLOR &&
				    gsinfo->output_semantic_name[i] != TGSI_SEMANTIC_BCOLOR)
					continue;

				/* We've found a color. */
				if (!cond) {
					/* The state is in the first bit of the user SGPR. */
					cond = LLVMGetParam(ctx.main_fn,
							    ctx.param_vs_state_bits);
					cond = LLVMBuildTrunc(ctx.ac.builder, cond,
							      ctx.i1, "");
					lp_build_if(&if_ctx, &ctx.gallivm, cond);
					/* Remember blocks for Phi. */
					blocks[0] = if_ctx.true_block;
					blocks[1] = if_ctx.entry_block;
				}

				for (unsigned j = 0; j < 4; j++) {
					/* Insert clamp into the true block. */
					v[0] = ac_build_clamp(&ctx.ac, outputs[i].values[j]);
					v[1] = outputs[i].values[j];

					/* Insert Phi into the endif block. */
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
	return shader;
}

```


* 通过gsvs_ring[0]读出 gs 处理后的输出数据,保存在output[i].values中
* 后续同样通过si_llvm_emit_streamout 导出数据


## 流输出的开始与终止

### 开始流输出si_emit_streamout_begin

在激活变换反馈流程接口中， 当设置流输出目标时， 设置si_streamout_buffers_dirty脏位，draw时发射

```c
static void si_emit_streamout_begin(struct si_context *sctx)
{
	struct radeon_cmdbuf *cs = sctx->gfx_cs;
	struct si_streamout_target **t = sctx->streamout.targets;
	uint16_t *stride_in_dw = sctx->streamout.stride_in_dw;
	unsigned i;

	si_flush_vgt_streamout(sctx);
    	struct radeon_cmdbuf *cs = sctx->gfx_cs;
    	unsigned reg_strmout_cntl;
    
    	/* The register is at different places on different ASICs. */
    	if (sctx->chip_class >= CIK) {
    		reg_strmout_cntl = R_0300FC_CP_STRMOUT_CNTL;
    		radeon_set_uconfig_reg(cs, reg_strmout_cntl, 0);
    	} else {
    		reg_strmout_cntl = R_0084FC_CP_STRMOUT_CNTL;
    		radeon_set_config_reg(cs, reg_strmout_cntl, 0);
    	}
    
    	radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
    	radeon_emit(cs, EVENT_TYPE(EVENT_TYPE_SO_VGTSTREAMOUT_FLUSH) | EVENT_INDEX(0));
    
    	radeon_emit(cs, PKT3(PKT3_WAIT_REG_MEM, 5, 0));
    	radeon_emit(cs, WAIT_REG_MEM_EQUAL); /* wait until the register is equal to the reference value */
    	radeon_emit(cs, reg_strmout_cntl >> 2);  /* register */
    	radeon_emit(cs, 0);
    	radeon_emit(cs, S_0084FC_OFFSET_UPDATE_DONE(1)); /* reference value */
    	radeon_emit(cs, S_0084FC_OFFSET_UPDATE_DONE(1)); /* mask */
    	radeon_emit(cs, 4); /* poll interval */


	for (i = 0; i < sctx->streamout.num_targets; i++) {
		if (!t[i])
			continue;

		t[i]->stride_in_dw = stride_in_dw[i];

		/* SI binds streamout buffers as shader resources.
		 * VGT only counts primitives and tells the shader
		 * through SGPRs what to do. */
		radeon_set_context_reg_seq(cs, R_028AD0_VGT_STRMOUT_BUFFER_SIZE_0 + 16*i, 2);
		radeon_emit(cs, (t[i]->b.buffer_offset +
				 t[i]->b.buffer_size) >> 2);	/* BUFFER_SIZE (in DW) */
		radeon_emit(cs, stride_in_dw[i]);		/* VTX_STRIDE (in DW) */

		if (sctx->streamout.append_bitmask & (1 << i) && t[i]->buf_filled_size_valid) {
			uint64_t va = t[i]->buf_filled_size->gpu_address +
				      t[i]->buf_filled_size_offset;

			/* Append. */
			radeon_emit(cs, PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0));
			radeon_emit(cs, STRMOUT_SELECT_BUFFER(i) |
				    STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_FROM_MEM)); /* control */
			radeon_emit(cs, 0); /* unused */
			radeon_emit(cs, 0); /* unused */
			radeon_emit(cs, va); /* src address lo */
			radeon_emit(cs, va >> 32); /* src address hi */

			radeon_add_to_buffer_list(sctx,  sctx->gfx_cs,
						  t[i]->buf_filled_size,
						  RADEON_USAGE_READ,
						  RADEON_PRIO_SO_FILLED_SIZE);
		} else {
			/* Start from the beginning. */
			radeon_emit(cs, PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0));
			radeon_emit(cs, STRMOUT_SELECT_BUFFER(i) |
				    STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_FROM_PACKET)); /* control */
			radeon_emit(cs, 0); /* unused */
			radeon_emit(cs, 0); /* unused */
			radeon_emit(cs, t[i]->b.buffer_offset >> 2); /* buffer offset in DW */
			radeon_emit(cs, 0); /* unused */
		}
	}

	sctx->streamout.begin_emitted = true;
}

```
* 该函数的功能刷新vgt中streamout配置，以及配置streamout buffer buf_filled_size 地址来源
* 其中只有连续使用且上一次流已经终止时才会设置buf_filled_size_valid 标志，此时可使用上次的buf_filled_size缓冲获取写入位置大小, 否则为初次使用，使用FROM_PACKET，直接使用缓冲的偏移量
* 当使用buf_filled_size且有效时，使用STRMOUT_OFFSET_FROM_MEM 表示使用分配的bo内存buf_filled_size读取，
* 该函数的最后将begin_emitted设置为真,如此可发射end

### si_emit_streamout_end

在开始启用新的gfx_cs流时或者进行下一次的streamout输出或者flush时，根据begin_emitted标志如果上次的streamout已经发射，此时终止上次的streamout 

```c
void si_emit_streamout_end(struct si_context *sctx)
{
	struct radeon_cmdbuf *cs = sctx->gfx_cs;
	struct si_streamout_target **t = sctx->streamout.targets;
	unsigned i;
	uint64_t va;

	si_flush_vgt_streamout(sctx);

	for (i = 0; i < sctx->streamout.num_targets; i++) {
		if (!t[i])
			continue;

		va = t[i]->buf_filled_size->gpu_address + t[i]->buf_filled_size_offset;
		radeon_emit(cs, PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0));
		radeon_emit(cs, STRMOUT_SELECT_BUFFER(i) |
			    STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_NONE) |
			    STRMOUT_STORE_BUFFER_FILLED_SIZE); /* control */
		radeon_emit(cs, va);     /* dst address lo */
		radeon_emit(cs, va >> 32); /* dst address hi */
		radeon_emit(cs, 0); /* unused */
		radeon_emit(cs, 0); /* unused */

		radeon_add_to_buffer_list(sctx,  sctx->gfx_cs,
					  t[i]->buf_filled_size,
					  RADEON_USAGE_WRITE,
					  RADEON_PRIO_SO_FILLED_SIZE);

		/* Zero the buffer size. The counters (primitives generated,
		 * primitives emitted) may be enabled even if there is not
		 * buffer bound. This ensures that the primitives-emitted query
		 * won't increment. */
		radeon_set_context_reg(cs, R_028AD0_VGT_STRMOUT_BUFFER_SIZE_0 + 16*i, 0);

		t[i]->buf_filled_size_valid = true;
	}

	sctx->streamout.begin_emitted = false;
}

```
* 可以看出此时也是下发STRMOUT_BUFFER_UPDATE更新的pm4包，表示将上次的steamout filledsize填入这个buf_filled_size 内存地址,为之后的ring 新的streamout读取,同时设置t[i]->buf_filled_size_valid标志表示可从filled_size buf读取大小
* STRMOUT_STORE_BUFFER_FILLED_SIZE表示将填充大小填入buf_filled_size bo地址

# case 测试分析


## 测试case

使用piglit primitive-type.c，测试GL_POINTS

```c
#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 32;
	config.supports_gl_core_version = 32;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END


#define MAX_OUTPUT_VERTICES 24


static const char *vs_text =
	"#version 150\n"
	"\n"
	"out int vertex_id;\n"
	"\n"
	"void main()\n"
	"{\n"
	"  vertex_id = gl_VertexID;\n"
	"}\n";

static const char *gs_template =
	"#version 150\n"
	"#define INPUT_LAYOUT %s\n"
	"#define VERTICES_PER_PRIM %d\n"
	"layout(INPUT_LAYOUT) in;\n"
	"layout(points, max_vertices = VERTICES_PER_PRIM) out;\n"
	"\n"
	"in int vertex_id[VERTICES_PER_PRIM];\n"
	"out int vertex_out[VERTICES_PER_PRIM];\n"
	"\n"
	"void main()\n"
	"{\n"
	"  for (int i = 0; i < VERTICES_PER_PRIM; i++) {\n"
	"    vertex_out[i] = vertex_id[i] + 1;\n"
	"  }\n"
	"  EmitVertex();\n"
	"}\n";


static const char *varyings[] = {
	"vertex_out[0]",
	"vertex_out[1]",
	"vertex_out[2]",
	"vertex_out[3]",
	"vertex_out[4]",
	"vertex_out[5]",
};


struct test_vector
{
	/** Number of vertices to send down the pipeline */
	unsigned num_input_vertices;

	/** Number of GS invocations expected */
	unsigned expected_gs_invocations;

	/**
	 * Vertices that each GS invocation is expected to see.
	 */
	GLint expected_results[MAX_OUTPUT_VERTICES];
};

static const struct test_vector points_tests[] = {
//	{ 0, 0, { 0 } },
//	{ 1, 1, { 1 } },
//	{ 2, 2, { 1, 2 } },
	{ 3, 3, { 1, 2,3 } },
};

static const struct test_vector line_loop_tests[] = {
	{ 1, 0, { 0 } },
	{ 2, 2, { 1, 2,
		  2, 1 } },
	{ 3, 3, { 1, 2,
		  2, 3,
		  3, 1 } },
	{ 4, 4, { 1, 2,
		  2, 3,
		  3, 4,
		  4, 1 } },
};

static const struct test_vector line_strip_tests[] = {
	{ 1, 0, { 0 } },
	{ 2, 1, { 1, 2 } },
	{ 3, 2, { 1, 2,
		  2, 3 } },
	{ 4, 3, { 1, 2,
		  2, 3,
		  3, 4 } },
};

static const struct test_vector lines_tests[] = {
	{ 1, 0, { 0 } },
	{ 2, 1, { 1, 2 } },
	{ 3, 1, { 1, 2 } },
	{ 4, 2, { 1, 2,
		  3, 4 } },
};

static const struct test_vector triangles_tests[] = {
	{ 2, 0, { 0 } },
	{ 3, 1, { 1, 2, 3 } },
	{ 5, 1, { 1, 2, 3 } },
	{ 6, 2, { 1, 2, 3,
		  4, 5, 6 } },
};

static const struct test_vector triangle_strip_tests[] = {
	{ 2, 0, { 0 } },
	{ 3, 1, { 1, 2, 3 } },
	{ 4, 2, { 1, 2, 3,
		  3, 2, 4 } },
	{ 5, 3, { 1, 2, 3,
		  3, 2, 4,
		  3, 4, 5 } },
};

static const struct test_vector triangle_fan_tests[] = {
	{ 2, 0, { 0 } },
	{ 3, 1, { 1, 2, 3 } },
	{ 4, 2, { 1, 2, 3,
		  1, 3, 4 } },
	{ 5, 3, { 1, 2, 3,
		  1, 3, 4,
		  1, 4, 5 } },
};

static const struct test_vector lines_adjacency_tests[] = {
	{ 3, 0, { 0 } },
	{ 4, 1, { 1, 2, 3, 4 } },
	{ 7, 1, { 1, 2, 3, 4 } },
	{ 8, 2, { 1, 2, 3, 4,
		  5, 6, 7, 8 } },
};

static const struct test_vector line_strip_adjacency_tests[] = {
	{ 3, 0, { 0 } },
	{ 4, 1, { 1, 2, 3, 4 } },
	{ 5, 2, { 1, 2, 3, 4,
		  2, 3, 4, 5 } },
	{ 6, 3, { 1, 2, 3, 4,
		  2, 3, 4, 5,
		  3, 4, 5, 6 } },
};

static const struct test_vector triangles_adjacency_tests[] = {
	{ 5, 0, { 0 } },
	{ 6, 1, { 1, 2, 3, 4, 5, 6 } },
	{ 11, 1, { 1, 2, 3, 4, 5, 6 } },
	{ 12, 2, { 1, 2, 3, 4, 5, 6,
		   7, 8, 9, 10, 11, 12 } },
};

/* Note: the required vertex order is surprisingly non-obvious for
 * GL_TRIANGLE_STRIP_ADJACENCY.
 *
 * Table 2.4 in the GL 3.2 core spec (Triangles generated by triangle
 * strips with adjacency) defines how the vertices in the triangle
 * strip are to be interpreted:
 *
 *                               Primitive Vertices  Adjacent Vertices
 *     Primitive                 1st   2nd   3rd     1/2   2/3   3/1
 *     only (i = 0, n = 1)        1     3     5       2     6     4
 *     first (i = 0)              1     3     5       2     7     4
 *     middle (i odd)            2i+3  2i+1  2i+5    2i-1  2i+4  2i+7
 *     middle (i even)           2i+1  2i+3  2i+5    2i-1  2i+7  2i+4
 *     last (i = n - 1, i odd)   2i+3  2i+1  2i+5    2i-1  2i+4  2i+6
 *     last (i = n - 1, i even)  2i+1  2i+3  2i+5    2i-1  2i+6  2i+4
 *
 * But it does not define the order in which these vertices should be
 * delivered to the geometry shader.  That's defined in section 2.12.1
 * of the GL 3.2 core spec (Geometry Shader Input Primitives):
 *
 *     Geometry shaders that operate on triangles with adjacent
 *     vertices are valid for the TRIANGLES_ADJACENCY and
 *     TRIANGLE_STRIP_ADJACENCY primitive types. There are six
 *     vertices available for each program invocation. The first,
 *     third and fifth vertices refer to attributes of the first,
 *     second and third vertex of the triangle, respectively. The
 *     second, fourth and sixth vertices refer to attributes of the
 *     vertices adjacent to the edges from the first to the second
 *     vertex, from the second to the third vertex, and from the third
 *     to the first vertex, respectively.
 *
 * Therefore the order in which the columns of table 2.4 should be
 * read is 1st, 1/2, 2nd, 2/3, 3rd, 3/1.
 *
 * So, for example, in the case where there is just a single triangle
 * delivered to the pipeline, we consult the first row of table 2.4 to
 * find:
 *
 *     Primitive Vertices  Adjacent Vertices
 *     1st   2nd   3rd     1/2   2/3   3/1
 *      1     3     5       2     6     4
 *
 * Rearranging into the order that should be delivered to the geometry
 * shader, we get:
 *
 *     1st   1/2   2nd   2/3   3rd   3/1
 *      1     2     3     6     5     4
 */
static const struct test_vector triangle_strip_adjacency_tests[] = {
	{ 5, 0, { 0 } },
	{ 6, 1, { 1, 2, 3, 6, 5, 4 } },
	{ 7, 1, { 1, 2, 3, 6, 5, 4 } },
	{ 8, 2, { 1, 2, 3, 7, 5, 4,
		  5, 1, 3, 6, 7, 8 } },
	{ 9, 2, { 1, 2, 3, 7, 5, 4,
		  5, 1, 3, 6, 7, 8 } },
	{ 10, 3, { 1, 2, 3, 7, 5, 4,
		   5, 1, 3, 6, 7, 9,
		   5, 3, 7, 10, 9, 8 } },
	{ 11, 3, { 1, 2, 3, 7, 5, 4,
		   5, 1, 3, 6, 7, 9,
		   5, 3, 7, 10, 9, 8 } },
	{ 12, 4, { 1, 2, 3, 7, 5, 4,
		   5, 1, 3, 6, 7, 9,
		   5, 3, 7, 11, 9, 8,
		   9, 5, 7, 10, 11, 12 } },
};


static const struct test_set
{
	GLenum prim_type;
	const char *input_layout;
	unsigned vertices_per_prim;
	const struct test_vector *test_vectors;
} tests[] = {
	{ GL_POINTS, "points", 1, points_tests },
	{ GL_LINE_LOOP, "lines", 2, line_loop_tests },
	{ GL_LINE_STRIP, "lines", 2, line_strip_tests },
	{ GL_LINES, "lines", 2, lines_tests },
	{ GL_TRIANGLES, "triangles", 3, triangles_tests },
	{ GL_TRIANGLE_STRIP, "triangles", 3, triangle_strip_tests },
	{ GL_TRIANGLE_FAN, "triangles", 3, triangle_fan_tests },
	{ GL_LINES_ADJACENCY, "lines_adjacency", 4, lines_adjacency_tests },
	{ GL_LINE_STRIP_ADJACENCY, "lines_adjacency", 4,
		line_strip_adjacency_tests },
	{ GL_TRIANGLES_ADJACENCY, "triangles_adjacency", 6,
		triangles_adjacency_tests },
	{ GL_TRIANGLE_STRIP_ADJACENCY, "triangles_adjacency", 6,
		triangle_strip_adjacency_tests },
};


static GLuint generated_query;


static void
print_usage_and_exit(const char *prog_name)
{
	int i;
	printf("Usage: %s <primitive>\n"
	       "  where <primitive> is one of the following:\n", prog_name);
	for (i = 0; i < ARRAY_SIZE(tests); i++)
		printf("    %s\n", piglit_get_prim_name(tests[i].prim_type));
	piglit_report_result(PIGLIT_FAIL);
}


static bool
do_test_vector(const struct test_set *test, const struct test_vector *vector)
{
	GLuint primitives_generated;
	int i;
	const GLint *readback;
	unsigned expected_output_points;
	unsigned actual_output_points;
	bool pass = true;

	printf("Testing %s(%d vertices)\n",
	       piglit_get_prim_name(tests->prim_type),
	       vector->num_input_vertices);

	/* Run vertices through the pipeline */
	glBeginQuery(GL_PRIMITIVES_GENERATED, generated_query);
	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(test->prim_type, 0, vector->num_input_vertices);
	glEndTransformFeedback();
	glEndQuery(GL_PRIMITIVES_GENERATED);

	/* Check that the GS got invoked the right number of times */
	glGetQueryObjectuiv(generated_query, GL_QUERY_RESULT,
			    &primitives_generated);
	if (primitives_generated != vector->expected_gs_invocations) {
		printf("  Expected %d GS invocations, got %d\n",
		       vector->expected_gs_invocations, primitives_generated);
		pass = false;
	}
	expected_output_points =
		vector->expected_gs_invocations * test->vertices_per_prim;
	actual_output_points = primitives_generated * test->vertices_per_prim;

	/* Check the data output by the GS */
	readback = glMapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	if (memcmp(readback, vector->expected_results,
		   expected_output_points * sizeof(GLint)) != 0) {
		pass = false;
	}

	/* Output details if the result was wrong */
	//if (!pass) {
		printf("  Expected vertex IDs:");
		for (i = 0; i < expected_output_points; i++)
			printf(" %d", vector->expected_results[i]);
		printf("\n");
		printf("  Actual vertex IDs:  ");
		for (i = 0; i < actual_output_points; i++)
			printf(" %d", readback[i]);
		printf("\n");
	//}

	glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

	return pass;
}


void
piglit_init(int argc, char **argv)
{
	int i;
	const struct test_set *test = NULL;
	GLuint prog, vs, gs, vao, xfb_buf;
	char *gs_text;
	bool pass = true;

	/* Parse params */
	if (argc != 2)
		print_usage_and_exit(argv[0]);
	for (i = 0; i < ARRAY_SIZE(tests); i++) {
		if (strcmp(piglit_get_prim_name(tests[i].prim_type),
			   argv[1]) == 0) {
			test = &tests[i];
			break;
		}
	}
	if (test == NULL)
		print_usage_and_exit(argv[0]);

	/* Compile shaders */
	prog = glCreateProgram();
	vs = piglit_compile_shader_text(GL_VERTEX_SHADER, vs_text);
	glAttachShader(prog, vs);
	(void)!asprintf(&gs_text, gs_template, test->input_layout,
		 test->vertices_per_prim);
	gs = piglit_compile_shader_text(GL_GEOMETRY_SHADER, gs_text);
	free(gs_text);
	glAttachShader(prog, gs);
	glTransformFeedbackVaryings(prog, test->vertices_per_prim, varyings,
				    GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(prog);
	if (!piglit_link_check_status(prog))
		piglit_report_result(PIGLIT_FAIL);
	glUseProgram(prog);

	/* Set up other GL state */
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &xfb_buf);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, xfb_buf);
	glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,
		     MAX_OUTPUT_VERTICES * sizeof(GLint), NULL,
		     GL_STREAM_READ);
	glGenQueries(1, &generated_query);
	glEnable(GL_RASTERIZER_DISCARD);
	int s = ARRAY_SIZE(test->test_vectors);
	for (i = 0; i < 1; i++) {
		pass = do_test_vector(test, &test->test_vectors[i]) && pass;
	}

	piglit_report_result(pass ? PIGLIT_PASS : PIGLIT_FAIL);
}


enum piglit_result
piglit_display(void)
{
	/* Should never be reached */
	return PIGLIT_FAIL;
}
```

## 调试输出

### shader流输出信息

```

begin shader: VERTEX
shader_state: {tokens = "
VERT
PROPERTY NEXT_SHADER GEOM
DCL SV[0], VERTEXID
DCL OUT[0].x, GENERIC[0]
  0: MOV OUT[0].x, SV[0].xxxx
  1: END
", stream_output = {num_outputs = 1, {1, 0, 0, 0, }{{register_index = 0, start_component = 0, num_components = 1, output_buffer = 0, }, }}, }
end shader: VERTEX


num stream output targets = 1
stream_output_target 0: {buffer = 0x557618c64230, buffer_offset = 0, buffer_size = 96, }
  buffer: {target = buffer, format = PIPE_FORMAT_R8_UNORM, width0 = 96, height0 = 1, depth0 = 1, array_size = 1, last_level = 0, nr_samples = 0, nr_storage_samples = 0, usage = 4, bind = 1024, flags = 0,
 }
  offset = 0


begin shader: GEOMETRY
shader_state: {tokens = "
GEOM
PROPERTY GS_INPUT_PRIMITIVE POINTS
PROPERTY GS_OUTPUT_PRIMITIVE POINTS
PROPERTY GS_MAX_OUTPUT_VERTICES 1
PROPERTY GS_INVOCATIONS 1
DCL IN[][0], GENERIC[0]
DCL OUT[0].x, GENERIC[0]
DCL TEMP[0], LOCAL
IMM[0] INT32 {1, 0, 0, 0}
  0: UADD TEMP[0].x, IN[0][0].xxxx, IMM[0].xxxx
  1: MOV OUT[0].x, TEMP[0].xxxx
  2: EMIT IMM[0].yyyy
  3: END
", stream_output = {num_outputs = 1, {1, 0, 0, 0, }{{register_index = 0, start_component = 0, num_components = 1, output_buffer = 0, }, }}, }
end shader: GEOMETRY


```
### 寄存器配置

```
c0026900 SET_CONTEXT_REG:
000002e5 
0000000f         VGT_STRMOUT_CONFIG <- STREAMOUT_0_EN = 1
                                       STREAMOUT_1_EN = 1
                                       STREAMOUT_2_EN = 1
                                       STREAMOUT_3_EN = 1
                                       RAST_STREAM = 0
                                       RAST_STREAM_MASK = 0
                                       USE_RAST_STREAM_MASK = 0
00000001         VGT_STRMOUT_BUFFER_CONFIG <- STREAM_0_BUFFER_EN = 1
                                              STREAM_1_BUFFER_EN = 0
                                              STREAM_2_BUFFER_EN = 0
                                              STREAM_3_BUFFER_EN = 0


```
无法从gallium_ddebug日志获取PKT3_STRMOUT_BUFFER_UPDATE，暂时si中未加入此包打印功能

### 变换反馈buffer数据

rw实际写入slot数与shader流输出信息的对应
```
RW buffers slot 3 (GPU list):
        SQ_BUF_RSRC_WORD0 <- 0x00060200
        SQ_BUF_RSRC_WORD1 <- BASE_ADDRESS_HI = 1
                             STRIDE = 0
                             CACHE_SWIZZLE = 0
                             SWIZZLE_ENABLE = 0
        SQ_BUF_RSRC_WORD2 <- 0xffffffff
        SQ_BUF_RSRC_WORD3 <- DST_SEL_X = SQ_SEL_X
                             DST_SEL_Y = SQ_SEL_Y
                             DST_SEL_Z = SQ_SEL_Z
                             DST_SEL_W = SQ_SEL_W
                             NUM_FORMAT = BUF_NUM_FORMAT_UNORM
                             DATA_FORMAT = BUF_DATA_FORMAT_32
                             ELEMENT_SIZE = 0
                             INDEX_STRIDE = 0
                             ADD_TID_ENABLE = 0
                             ATC = 0
                             HASH_ENABLE = 0
                             HEAP = 0
                             MTYPE = 0
                             TYPE = SQ_RSRC_BUF

RW buffers slot 4 (GPU list):

```
由上获取变换反馈buffer地址

```
user@user:/home/shiji/piglit/bin$ od -j 0x100060200 -N 100 -t u4 bo_glsl-1.50-geometry-primitive-types_12608_00000003.bin
40001401000          1          2          3          0
40001401020          0          0          0          0
*
40001401140          0
40001401144
```

实际输出

```
  Expected vertex IDs: 1 2 3
dd: dumping to file /home/user/shiji_ddebug_dumps/glsl-1.50-geometry-primitive-types_12608_00000001
  Actual vertex IDs:   1 2 3
PIGLIT: {"result": "pass" }

```



# 附录相关PM4包及寄存器

## STRMOUT_BUFFER_UPDATE

STRMOUT_BUFFER_UPDATE数据包预计在各种流输出场景中使用。当一个流输出操作跨越两个命令缓冲区时，驱动程序需要确保在第一个命令缓冲区结束时捕获每个流输出缓冲区的BufferFilledSize，并在下一个命令缓冲区中使用捕获的值重新启动流输出缓冲区。


STRMOUT_BUFFER_UPDATE Packet Description
| DW | 字段名            | 描述 |
|----|-------------------|-------------|
| 1  | HEADER            | 数据包头部 |
| 2  | CONTROL           |
|    | [31:10]           | 保留字段 |
|    | [9:8]             | Buffer Select：指示正在更新的流输出缓冲区
|    |                   | - 00: 流输出缓冲区 0
|    |                   | - 01: 流输出缓冲区 1
|    |                   | - 10: 流输出缓冲区 2
|    |                   | - 11: 流输出缓冲区 3
|    | [7:3]             | 保留字段
|    | [2:1]             | Source_Select：写入VGT_STRMOUT_BUFFER_OFFSET的来源
|    |                   | - 00: 在此数据包中使用BUFFER_OFFSET
|    |                   | - 01: 从VGT_STRMOUT_BUFFER_FILLED_SIZE读取
|    |                   | - 10: 从SRC_ADDRESS读取
|    |                   | - 11: 无
|    | [0]               | Update_Memory：将BufferFilledSize存储到内存中
|    |                   | - 0: 不更新内存；DST_ADDRESS_LO/HI不重要，但必须提供
|    |                   | - 1: 在DST_ADDRESS处使用VGT_STRMOUT_BUFFER_FILLED_SIZE更新内存
| 3  | DST_ADDRESS_LO    | [31:2] - 目标地址的低位 [31:2]。仅在Update_Memory为“1”时有效。 [1:0] - 用于数据写入的交换[1:0]功能。
| 4  | DST_ADDRESS_HI    | [15:0] - 目标地址的高位 [47:32]。仅在Store BufferFilledSize为“01”时有效。
| 5  | BUFFER_OFFSET 或  | 如果Source Select = “00”，则位[31:0]包含要写入VGT_STRMOUT_BUFFER_OFFSET的BUFFER_OFFSET[31:0]。如果Source Select = “01”，则此序数不重要。
|    | SRC_ADDRESS_LO    | 如果Source Select = “10”，则位[31:2]为“SRC_ADDRESS_LO”（DWord-Aligned Source Address [31:2]的低位）。位[1:0] - 用于数据读取的交换[1:0]功能。仅在Source_Select为“10”时有效。
| 6  | SRC_ADDRESS_HI    | 位[15:0] - 源地址的高位 [47:32]。仅在Source_Select为“10”时有效。


## VGT_STRMOUT_BUFFER_CONFIG 寄存器

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_CONFIG |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28b98 |

**描述：** 用于流输出的启用位。CP将用于SO（流输出）的一致性寄存器的有效性。

| 字段名              | 位   | 默认值 | 描述                                            |
|---------------------|------|--------|-------------------------------------------------|
| STREAM_0_BUFFER_EN  | 3:0  | 0x0    | 绑定流0的缓冲区。将第0位设置为1表示绑定缓冲区0，第1位表示缓冲区1，第2位表示缓冲区2，第3位表示缓冲区3。|
| STREAM_1_BUFFER_EN  | 7:4  | 0x0    | 绑定流1的缓冲区。将第0位设置为1表示绑定缓冲区0，第1位表示缓冲区1，第2位表示缓冲区2，第3位表示缓冲区3。|
| STREAM_2_BUFFER_EN  | 11:8 | 0x0    | 绑定流2的缓冲区。将第0位设置为1表示绑定缓冲区0，第1位表示缓冲区1，第2位表示缓冲区2，第3位表示缓冲区3。|
| STREAM_3_BUFFER_EN  | 15:12| 0x0    | 绑定流3的缓冲区。将第0位设置为1表示绑定缓冲区0，第1位表示缓冲区1，第2位表示缓冲区2，第3位表示缓冲区3。|

## VGT_STRMOUT_BUFFER_FILLED_SIZE 寄存器（0、1、2）

#### VGT_STRMOUT_BUFFER_FILLED_SIZE_0

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_FILLED_SIZE_0 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x8960 |

**描述：** 流输出调整大小。

| 字段名 | 位   | 默认值 | 描述                                                 |
|--------|------|--------|------------------------------------------------------|
| SIZE   | 31:0 | 无     | DWORD，给定缓冲区的（SO_BufferOffset + BufDwordWritten）之和。只读。要读取此寄存器，需要将VGT刷新到BufDwordWritten计数得以维护的点。

### VGT_STRMOUT_BUFFER_FILLED_SIZE_1

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_FILLED_SIZE_1 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x8964 |

**描述：** 流输出调整大小。

| 字段名 | 位   | 默认值 | 描述                                                 |
|--------|------|--------|------------------------------------------------------|
| SIZE   | 31:0 | 无     | DWORD，给定缓冲区的（SO_BufferOffset + BufDwordWritten）之和。只读。要读取此寄存器，需要将VGT刷新到BufDwordWritten计数得以维护的点。

### VGT_STRMOUT_BUFFER_FILLED_SIZE_2

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_FILLED_SIZE_2 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x8968 |

**描述：** 流输出调整大小。

| 字段名 | 位   | 默认值 | 描述                                                 |
|--------|------|--------|------------------------------------------------------|
| SIZE   | 31:0 | 无     | DWORD，给定缓冲区的（SO_BufferOffset + BufDwordWritten）之和。只读。要读取此寄存器，需要将VGT刷新到BufDwordWritten计数得以维护的点。



下面是对提供的文本的翻译，以Markdown表格的形式呈现：

## VGT_STRMOUT_BUFFER_OFFSET 寄存器（0、1、2、3）

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_OFFSET_0 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28adc |

**描述：** 流输出偏移。

| 字段名 | 位   | 默认值 | 描述                                           |
|--------|------|--------|------------------------------------------------|
| OFFSET | 31:0 | 无     | DWORD，给定流输出缓冲区的偏移。               |

### VGT_STRMOUT_BUFFER_OFFSET_1

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_OFFSET_1 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28aec |

**描述：** 流输出偏移。

| 字段名 | 位   | 默认值 | 描述                                           |
|--------|------|--------|------------------------------------------------|
| OFFSET | 31:0 | 无     | DWORD，给定流输出缓冲区的偏移。               |

### VGT_STRMOUT_BUFFER_OFFSET_2

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_OFFSET_2 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28afc |

**描述：** 流输出偏移。

| 字段名 | 位   | 默认值 | 描述                                           |
|--------|------|--------|------------------------------------------------|
| OFFSET | 31:0 | 无     | DWORD，给定流输出缓冲区的偏移。               |

### VGT_STRMOUT_BUFFER_OFFSET_3

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_OFFSET_3 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28b0c |

**描述：** 流输出偏移。

| 字段名 | 位   | 默认值 | 描述                                           |
|--------|------|--------|------------------------------------------------|
| OFFSET | 31:0 | 无     | DWORD，给定流输出缓冲区的偏移。               |

## VGT_STRMOUT_BUFFER_SIZE 寄存器（0、1、2、3）

### VGT_STRMOUT_BUFFER_SIZE_0

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_SIZE_0 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28ad0 |

**描述：** 流输出大小。

| 字段名 | 位   | 默认值 | 描述                                           |
|--------|------|--------|------------------------------------------------|
| SIZE   | 31:0 | 无     | DWORD，给定流输出缓冲区的大小。               |

### VGT_STRMOUT_BUFFER_SIZE_1

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_SIZE_1 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28ae0 |

**描述：** 流输出大小。

| 字段名 | 位   | 默认值 | 描述                                           |
|--------|------|--------|------------------------------------------------|
| SIZE   | 31:0 | 无     | DWORD，给定流输出缓冲区的大小。               |

### VGT_STRMOUT_BUFFER_SIZE_2

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_SIZE_2 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28af0 |

**描述：** 流输出大小。

| 字段名 | 位   | 默认值 | 描述                                           |
|--------|------|--------|------------------------------------------------|
| SIZE   | 31:0 | 无     | DWORD，给定流输出缓冲区的大小。               |

### VGT_STRMOUT_BUFFER_SIZE_3

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_BUFFER_SIZE_3 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28b00 |

**描述：** 流输出大小。

| 字段名 | 位   | 默认值 | 描述                                           |
|--------|------|--------|------------------------------------------------|
| SIZE   | 31:0 | 无

 | DWORD，给定流输出缓冲区的大小。               |

## VGT_STRMOUT_CONFIG 寄存器

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_CONFIG |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28b94 |

**描述：** 此寄存器启用流输出。

| 字段名           | 位   | 默认值 | 描述                                                     |
|------------------|------|--------|----------------------------------------------------------|
| STREAMOUT_0_EN   | 0    | 0x0    | 如果设置，启用流输出到流0。                                |
| STREAMOUT_1_EN   | 1    | 0x0    | 如果设置，启用流输出到流1。                                |
| STREAMOUT_2_EN   | 2    | 0x0    | 如果设置，启用流输出到流2。                                |
| STREAMOUT_3_EN   | 3    | 0x0    | 如果设置，启用流输出到流3。                                |
| RAST_STREAM      | 6:4  | 0x0    | 启用光栅化的流，如果设置了bit[6]，则对任何流都不启用光栅化。|
| RAST_STREAM_MASK | 11:8 | 0x0    | 指示哪个流已启用的掩码。仅在USE_RAST_STREAM_MASK为1时有效。    |
| USE_RAST_STREAM_MASK | 31 | 0x0  | 当为1时，使用RAST_STREAM_MASK。当为0时，使用RAST_STREAM。    |

## VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE 寄存器

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28b2c |

**描述：** 不透明绘制大小。

| 字段名 | 位   | 默认值 | 描述                                                 |
|--------|------|--------|------------------------------------------------------|
| SIZE   | 31:0 | 无     | 由CP在DrawOpaque调用中加载，通过获取与前一个绑定到IA的流输出缓冲区相关的last bufferfilledsize的内存地址。|

## VGT_STRMOUT_DRAW_OPAQUE_OFFSET 寄存器

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_DRAW_OPAQUE_OFFSET |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28b28 |

**描述：** 不透明绘制偏移。

| 字段名 | 位   | 默认值 | 描述                                                 |
|--------|------|--------|------------------------------------------------------|
| OFFSET | 31:0 | 无     | 从流输出缓冲区的IASetVertexBuffers绑定的pOffsets，用作src数据的流输出缓冲区的poffset。如果检索到的BufferFilledSize减去这个poffset是正数，则确定可以从中创建图元的数据量。|

## VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE 寄存器

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28b30 |

**描述：** 不透明绘制顶点步长。

| 字段名        | 位   | 默认值 | 描述                              |
|---------------|------|--------|-----------------------------------|
| VERTEX_STRIDE | 8:0  | 无     | DrawOpaque调用中使用的顶点步长。 |

## VGT_STRMOUT_VTX_STRIDE 寄存器（0、1、2、3）

### VGT_STRMOUT_VTX_STRIDE_0

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_VTX_STRIDE_0 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28ad4 |

**描述：** 流输出步长。

| 字段名 | 位   | 默认值 | 描述                                                 |
|--------|------|--------|------------------------------------------------------|
| STRIDE | 9:0  | 无     | DWORD，在给定流输出缓冲区中顶点之间的步长。根据dx10规范的流输出声明详细信息，最大步长为2048字节或512个字定义为每个顶点之间的起始间距。

### VGT_STRMOUT_VTX_STRIDE_1

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_VTX_STRIDE_1 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28ae4 |

**描述：** 流输出步长。

| 字段名 | 位   | 默认值 | 描述                                                 |
|--------|------|--------|------------------------------------------------------|
| STRIDE | 9:0  | 无     | DWORD，在给定流输出缓冲区中顶点之间的步长。根据dx10规范的流输出声明详细信息，最大步长为2048字节或512个字定义为每个顶点之间的起始间距。

### VGT_STRMOUT_VTX_STRIDE_2

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_VTX_STRIDE_2 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28af4 |

**描述：** 流输出步长。

| 字段名 | 位   | 默认值 | 描述                                                 |
|--------|------|--------|------------------------------------------------------|
| STRIDE | 9:0  | 无     | DWORD，在给定流输出缓冲区中顶点之间的步长。根据dx10规范的流输出声明详细信息，最大步长为2048字节或512个字定义为每个顶点之间的起始间距。

### VGT_STRMOUT_VTX_STRIDE_3

| 寄存器属性 | 值 |
|-------------|----|
| 寄存器名称 | VGT_STRMOUT_VTX_STRIDE_3 |
| 类型 | 读/写 |
| 位数 | 32位 |
| 访问 | 32 |
| 地址 | GpuF0MMReg:0x28b04 |

**描述：** 流输出步长。

| 字段名 | 位   | 默认值 | 描述                                                 |
|--------|------|--------|------------------------------------------------------|
| STRIDE | 9:0  | 无     | DWORD，在给定流输出缓冲区中顶点之间的步长。根据dx10规范的流输出声明详细信息，最大步长为2048字节或512个字定义为每个顶点之间的起始间距。

