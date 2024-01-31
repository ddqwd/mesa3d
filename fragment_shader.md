
# fs 的glsl 4.5 规范

## 4 Variables and Types

### 4.3 Storage Qualifiers

**varying**仅适用于兼容性配置文件，仅在顶点和片段语言中有效；在顶点着色器中使用时，与`out`相同，在片段着色器中使用时，与`in`相同。

#### 4.3.4 Input Variables

片段着色器输入获取每个片段的数值，通常是从先前阶段的输出进行插值得到的。它们在片段着色器中使用`in`存储限定符声明。也可以应用辅助存储限定符`centroid`和`sample`，以及插值限定符`flat`、`noperspective`和`smooth`。在声明片段着色器输入时，如果包含以下任何内容，将在编译时产生错误：
- 布尔类型（`bool`、`bvec2`、`bvec3`、`bvec4`）
- 不透明类型

具有有符号或无符号整数、整数向量或任何双精度浮点类型的片段着色器输入必须使用插值限定符`flat`进行限定。片段输入的声明如下所示：

```glsl
in vec3 normal;
centroid in vec2 TexCoord;
invariant centroid in vec4 Color;
noperspective in float temperature;
flat in vec3 myColor;
noperspective centroid in vec2 myTexCoord;
```
片段着色器输入构成了与顶点处理管线中最后一个活动着色器的接口。对于这个接口，最后一个活动着色器阶段的输出变量和片段着色器输入变量的相同名称必须在类型和限定符方面匹配，当然也有一些例外：存储限定符必须不同（一个是`in`，另一个是`out`）。此外，插值限定符（例如，`flat`）和辅助限定符（例如，`centroid`）可能不同。这些不匹配允许发生在任意两个阶段之间。当插值或辅助限定符不匹配时，片段着色器中提供的限定符将覆盖先前阶段提供的限定符。如果在片段着色器中完全缺少任何这些限定符，则使用默认值，而不是在先前阶段中可能声明的任何限定符。换句话说，重要的是片段着色器中的声明，而不是在先前阶段的着色器中声明的内容。



#### 4.3.6 输出变量

片段输出输出每个片段的数据，并使用`out`存储限定符进行声明。在片段着色器中，在输出上使用辅助存储限定符或插值限定符是编译时错误。在声明片段着色器输出时，如果包含以下任何内容，将会产生编译时错误：
- 布尔类型（`bool`、`bvec2`、`bvec3`、`bvec4`）
- 双精度标量或向量（`double`、`dvec2`、`dvec3`、`dvec4`）
- 不透明类型
- 任何矩阵类型
- 结构体

片段输出的声明如下所示：
```glsl
out vec4 FragmentColor;
out uint Luminosity;
```


### 4.4.1 Input Layout Qualifiers

除了计算着色器之外的所有着色器，允许在输入变量声明、输入块声明和输入块成员声明上使用位置布局修饰符。其中，变量和块成员（但不包括块本身）还允许使用分量布局修饰符。

#### 4.4.1.3 片段着色器输入

针对 `gl_FragCoord`，额外的片段布局限定符标识符包括以下两个：
```
layout-qualifier-id :
origin_upper_left
pixel_center_integer
```
默认情况下，`gl_FragCoord` 假定窗口坐标采用左下角为原点，并假定像素中心位于半像素坐标。例如，在窗口中，左下角像素的位置 (0.5, 0.5) 会被返回。可以通过使用 `origin_upper_left` 标识符重新声明 `gl_FragCoord` 来更改原点，将 `gl_FragCoord` 的原点移至窗口的左上角，其中 y 的值向窗口底部增加。返回的值也可以通过 `pixel_center_integer` 标识符在 x 和 y 方向上各偏移半个像素，使得像素看起来位于整数像素偏移的中心。

重新声明的方式如下：
```glsl
in vec4 gl_FragCoord; // 允许不改变的重新声明
// 以下所有的重新声明都是允许的，会改变行为
layout(origin_upper_left) in vec4 gl_FragCoord;
layout(pixel_center_integer) in vec4 gl_FragCoord;
layout(origin_upper_left, pixel_center_integer) in vec4 gl_FragCoord;
```

如果在程序的任何片段着色器中重新声明了 `gl_FragCoord`，则在该程序中所有使用 `gl_FragCoord` 的片段着色器都必须重新声明它，并且在同一程序中所有片段着色器对 `gl_FragCoord` 的所有重新声明都必须具有相同的一组限定符。在任何着色器中，对 `gl_FragCoord` 的第一个重新声明必须出现在对 `gl_FragCoord` 的任何使用之前。内建的 `gl_FragCoord` 仅在片段着色器中被预声明，因此在任何其他着色器语言中重新声明它将导致编译时错误。

使用 `origin_upper_left` 和/或 `pixel_center_integer` 限定符重新声明 `gl_FragCoord` 只影响 `gl_FragCoord.x` 和 `gl_FragCoord.y`。它对光栅化、变换或 OpenGL 管道或语言功能的任何其他部分都没有影响。

片段着色器还允许在 `in` 上使用以下布局限定符（仅在变量声明中不适用）：
```
layout-qualifier-id :
early_fragment_tests
```
请求在片段着色器执行之前执行片段测试，如 OpenGL 规范的第 15.2.4 节“早期片段测试”中所描述。

例如：
```glsl
layout(early_fragment_tests) in;
```
指定将在片段着色器执行之前执行每个片段的测试。如果未声明此内容，片段测试将在片段着色器执行后执行。只需要一个片段着色器（编译单元）声明这个，尽管可以有多个。如果至少有一个声明了这个，那么它就被启用。

### 4.4.2 Output Layout Qualifiers

一些输出布局修饰符适用于所有着色器语言，而一些仅适用于特定语言。后者在下面的各个小节中讨论。

与输入布局修饰符一样，除了计算着色器之外的所有着色器都允许在输出变量声明、输出块声明和输出块成员声明上使用位置布局修饰符。其中，变量和块成员（但不包括块本身）还允许使用分量布局修饰符。


对于组件限定符的使用和规则，以及将位置限定符应用于块和结构体，其用法和规则与第4.4.1节"输入布局限定符"中描述的完全相同。此外，**对于片段着色器输出，如果两个变量被放置在相同的位置，它们必须具有相同的基础类型（浮点或整数）。**

片段着色器允许使用额外的索引输出布局限定符：
```
layout-qualifier-id :
  index = integer-constant-expression
```
这些限定符每个最多只能出现一次。如果指定了`index`，则必须同时指定`location`。如果未指定`index`，则使用值0。例如，在片段着色器中：
```glsl
layout(location = 3) out vec4 color;
```
将确立片段着色器输出颜色被分配给混合方程的第一个（索引为零）输入的片段颜色3。而：
```glsl
layout(location = 3, index = 1) out vec4 factor;
```
将确立片段着色器输出因子被分配给混合方程的第二个（索引为一）输入的片段颜色3。

对于片段着色器输出，`location`和`index`指定接收输出值的颜色输出编号和索引。对于所有其他着色器阶段的输出，`location`指定一个矢量编号，可用于与后续着色器阶段的输入匹配，即使该着色器位于不同的程序对象中。

如果声明的输出是除`dvec3`或`dvec4`之外的标量或矢量类型，它将占用一个位置。类型为`dvec3`或`dvec4`的输出将占用两个连续的位置。类型为`double`和`dvec2`的输出在所有阶段中仅占用一个位置。

如果声明的输出是一个数组，它将被分配连续的位置，从指定的位置开始。例如：
```glsl
layout(location = 2) out vec4 colors[3];
```
将确立`colors`被分配给矢量位置编号2、3和4。

如果声明的输出是一个n x m的单精度或双精度矩阵，它将被分配多个位置，从指定的位置开始。分配的位置数量与具有m分量的矢量的nelement数组相同。

如果声明的输出是一个结构体，其成员将按照声明的顺序分配连续的位置，其中第一个成员被分配给为结构体指定的位置。结构体成员所占用的位置数量由递归应用上述规则确定，就好像结构体成员被声明为相同类型的输出变量一样。

位置布局限定符可以用于声明为结构的输出变量。但是，在结构成员上使用位置限定符是编译时错误。

对于一个着色器可用的输出位置数量是有限的。对于片段着色器，限制是广告中的绘制缓冲区的数量。对于所有其他着色器，限制是依赖于实现的，且必须不少于广告的最大输出分量计数的四分之一。 （计算着色器没有输出。）如果任何附加的着色器使用的位置大于或等于支持的位置的数量，则程序将无法链接，除非设备相关的优化能够使程序适应可用的硬件资源。还可能在编译时出现错误，如果在编译时知道链接将失败。如果输出位置为负数，将导致编译时错误。如果片段着色器将布局索引设置为小于0或大于1，也会导致编译时错误。

如果发生以下任何情况，程序将无法链接：
- 任何两个片段着色器输出变量分配给相同的位置和索引，或者
- 任何两个几何着色器输出变量分配给相同的位置和流，或者
- 如果来自同一顶点或细分着色器阶段的任何两个输出变量分配给相同的位置。

对于片段着色器的输出，可以使用布局限定符或通过OpenGL API分配位置。对于所有着色器类型，如果显式位置分配使链接器无法为其他变量找到空间而无法链接程序。

如果在着色器文本中未为没有分配位置或索引的输出变量指定位置，则将使用OpenGL API分配的位置。否则，链接器将为这样的变量分配一个位置。所有这样的分配都将具有颜色索引为零。有关详细信息，请参见OpenGL图形系统规范的第15.2节“着色器执行”。如果在相同语言的多个着色器中声明了输出变量，并且它们具有冲突的位置或索引值，则将发生链接时错误。

为了确定非片段输出是否与后续着色器阶段的输入匹配，位置布局限定符（如果有）必须匹配。



#### 4.4.2.4 片段输出

内建的片段着色器变量 `gl_FragDepth` 可以通过以下布局限定符之一进行重新声明。
```
layout-qualifier-id :
depth_any
depth_greater
depth_less
depth_unchanged
```
例如：
```glsl
layout(depth_greater) out float gl_FragDepth;
```

`gl_FragDepth` 的布局限定符约束了任何着色器调用写入的 `gl_FragDepth` 的最终值的意图。OpenGL 实现允许执行优化，假设对于给定片段，如果所有与布局限定符一致的 `gl_FragDepth` 的值都会失败（或通过），则深度测试将失败（或通过）。这可能包括跳过着色器执行，如果片段被丢弃因为被遮挡且着色器没有副作用。如果 `gl_FragDepth` 的最终值与其布局限定符不一致，则相应片段的深度测试结果未定义。然而，在这种情况下不会生成错误。如果深度测试通过并且启用深度写入，则写入深度缓冲区的值始终为 `gl_FragDepth` 的值，无论它是否与布局限定符一致。

默认情况下，`gl_FragDepth` 被限定为 `depth_any`。当 `gl_FragDepth` 的布局限定符为 `depth_any` 时，着色器编译器将注意到对 `gl_FragDepth` 的任何修改都以未知的方式修改它，深度测试将在着色器执行后始终执行。当布局限定符为 `depth_greater` 时，GL 可以假设 `gl_FragDepth` 的最终值大于或等于片段的插值深度值，即由 `gl_FragCoord` 的 z 分量给出。当布局限定符为 `depth_less` 时，GL 可以假设对 `gl_FragDepth` 的任何修改只会减小其值。当布局限定符为 `depth_unchanged` 时，着色器编译器将遵守对 `gl_FragDepth` 的任何修改，但GL的其余部分可以假定 `gl_FragDepth` 没有被赋予新值。

重新声明 `gl_FragDepth` 的方式如下：
```glsl
// 允许不改变的重新声明
out float gl_FragDepth;
// 假设它可能以任何方式修改
layout(depth_any) out float gl_FragDepth;
// 假设它可能以只增加的方式修改
layout(depth_greater) out float gl_FragDepth;
// 假设它可能以只减小的方式修改
layout(depth_less) out float gl_FragDepth;
// 假设它不会被修改
layout(depth_unchanged) out float gl_FragDepth;
```

如果在程序的任何片段着色器中重新声明了 `gl_FragDepth`，则在该程序中所有对 `gl_FragDepth` 有静态赋值的片段着色器中都必须重新声明它，并且在同一程序中所有片段着色器对 `gl_FragDepth` 的所有重新声明都必须具有相同的一组限定符。在任何着色器中，对 `gl_FragDepth` 的第一个重新声明必须出现在对 `gl_FragDepth` 的任何使用之前。内建的 `gl_FragDepth` 仅在片段着色器中被预声明，因此在任何其他着色器语言中重新声明它将导致编译时错误。


### 4.5 插值限定符

可能需要插值的输入和输出可以进一步由以下插值限定符之一进行修饰，至多一个：
| Qualifier       | Meaning                           |
| --------------- | ----------------------------------|
| smooth          | Perspective correct interpolation |
| flat            | No interpolation                   |
| noperspective   | Linear interpolation               |


以上插值限定符以及辅助存储限定符`centroid`和`sample`控制了插值的存在和类型。辅助存储限定符`patch`不用于插值；在`patch`上使用插值限定符是编译时错误。
被标记为`flat`的变量将不会被插值。相反，它在三角形内的每个片段中都将具有相同的值。该值将来自于一个单一的引发顶点，如OpenGL图形系统规范所述。被标记为`flat`的变量也可以被标记为`centroid`或`sample`，这将意味着与仅标记为`flat`时相同的含义。
被标记为`smooth`的变量将以透视校正的方式在渲染的原语上进行插值。透视校正方式的插值在OpenGL图形系统规范的第14.5节“线段”中的方程14.7中有详细说明。
被标记为`noperspective`的变量必须在屏幕空间中进行线性插值，如OpenGL图形系统规范第3.5节“线段”中的方程3.7中所述。
当禁用多采样光栅化时，或者对于没有标记`centroid`或`sample`的片段着色器输入变量，分配的变量值可以在像素内的任意位置进行插值，并且可以为像素内的每个样本分配一个单一值，这受OpenGL图形系统规范的限制。
当启用多采样光栅化时，`centroid`和`sample`可用于控制对受限定片段着色器输入进行采样的位置和频率。如果片段着色器输入使用`centroid`进行标记，可以为像素内的所有样本分配一个单一值，但是该值必须被插值到像素和正在渲染的原语的位置，包括原语覆盖的像素样本。由于变量插值的位置在相邻像素中可能不同，且由于可以通过计算相邻像素之间的差异来计算导数，因此`centroid`采样输入的导数可能比非`centroid`插值变量的导数不准确。如果片段着色器输入使用`sample`进行标记，则必须为像素内的每个受覆盖样本分配一个单独的值，并且该值必须在个别样本的位置进行采样。
如果在同一阶段内，同名变量的插值限定符不匹配，则会发生链接时错误。

### 6.4 跳转语句

- `continue;`
- `break;`
- `return;`
- `return expression;`
- `discard;` // 仅适用于片元着色器语言

没有“goto”或其他非结构化的控制流。

`discard` 关键字仅允许在片元着色器中使用。它可在片元着色器内部用于放弃对当前片元的操作。此关键字导致片元被丢弃，不会对任何缓冲区进行更新。控制流退出着色器，当此退出是不均匀的时，随后的隐式或显式导数是未定义的。通常会在条件语句内部使用，例如：

```glsl
if (intensity < 0.0)
    discard;
```

片元着色器可以测试片元的 alpha 值并基于该测试丢弃片元。但应注意，在片元着色器运行之后会执行覆盖测试，而覆盖测试可能会改变 alpha 值。

`return` 跳转导致当前函数立即退出。如果有表达式，那么它就是函数的返回值。

函数 `main` 可以使用 `return`。这只是导致 `main` 以与到达函数末尾时相同的方式退出。它不意味着在片元着色器中使用 `discard`。在定义输出之前在 `main` 中使用 `return` 将具有与在定义输出之前到达 `main` 末尾相同的行为。


#### 4.5.1 在兼容性配置文件中重新声明内建插值变量

在使用兼容性配置文件时，可以使用插值限定符重新声明以下预定义变量：
顶点、曲面细分控制、曲面细分评估和几何语言：
- `gl_FrontColor`
- `gl_BackColor`
- `gl_FrontSecondaryColor`
- `gl_BackSecondaryColor`

片元语言：
- `gl_Color`
- `gl_SecondaryColor`

例如：
```glsl
in vec4 gl_Color;           // 片元语言预定义
flat in vec4 gl_Color;       // 用户重新声明为flat
flat in vec4 gl_FrontColor;  // 几何着色器的输入，没有“gl_in[]”
flat out vec4 gl_FrontColor; // 几何着色器的输出
```

理想情况下，这些变量应该作为接口块重新声明，如第7.1.1节“兼容性配置文件内建语言变量”所述。然而，为了上述目的，它们可以作为全局范围的独立变量重新声明，不在接口块中。这种重新声明还允许将 transform-feedback 限定符 xfb_buffer、xfb_stride 和 xfb_offset 添加到输出变量。（在变量上使用 xfb_buffer 不会更改全局默认缓冲区。）如果一个着色器既有接口块重新声明又在接口块外重新声明该接口块的成员，将导致编译时错误。
如果使用插值限定符重新声明 gl_Color，则必须使用相同的插值限定符重新声明 gl_FrontColor 和 gl_BackColor（如果它们被写入），反之亦然。如果使用插值限定符重新声明 gl_SecondaryColor，则必须使用相同的插值限定符重新声明 gl_FrontSecondaryColor 和 gl_BackSecondaryColor（如果它们被写入），反之亦然。对于在程序中的着色器中静态使用的变量，只有在预定义变量上需要进行限定符匹配。

## 7 Build-in Variabes


In the fragment language, built-in variables are intrinsically declared as:

```glsl

in vec4 gl_FragCoord;
in bool gl_FrontFacing;
in float gl_ClipDistance[];
in float gl_CullDistance[];
in vec2 gl_PointCoord;
in int gl_PrimitiveID;
in int gl_SampleID;
in vec2 gl_SamplePosition;
in int gl_SampleMaskIn[];
in int gl_Layer;
in int gl_ViewportIndex;
in bool gl_HelperInvocation;
out float gl_FragDepth;
out int gl_SampleMask[];
```

变量 `gl_ClipDistance` 提供了用于控制用户裁剪的前向兼容机制。数组元素 `gl_ClipDistance[i]` 指定了每个平面 `i` 的裁剪距离。距离为 0 表示顶点在平面上，正距离表示顶点在裁剪平面内，负距离表示点在裁剪平面外。裁剪距离将在原语中进行线性插值，插值距离小于 0 的原语部分将被裁剪。

`gl_ClipDistance` 数组是预先声明的，大小未确定，必须由着色器显式地使用大小重新声明，或者通过仅使用整数常量表达式进行索引来隐式确定大小。这需要将数组的大小设置为包括通过 OpenGL API 启用的所有裁剪平面；如果大小不包括所有已启用的平面，结果是未定义的。大小最多可以是 `gl_MaxClipDistances`。由 `gl_ClipDistance` 消耗的变化分量的数量（参见 `gl_MaxVaryingComponents`）将与数组的大小匹配，无论启用了多少平面。着色器还必须设置通过 OpenGL API 启用的 `gl_ClipDistance` 中的所有值，否则结果是未定义的。对于未启用的平面写入 `gl_ClipDistance` 的值没有效果。

作为输出变量，`gl_ClipDistance` 提供了着色器写入这些距离的位置。在所有语言中除了片元语言，在这些语言中它是一个输入，读取前一个着色器阶段中写入的值。在片元语言中，`gl_ClipDistance` 数组包含由着色器写入 `gl_ClipDistance` 顶点输出变量的顶点值的线性插值。只有启用裁剪的数组中的元素才有定义的值。

变量 `gl_CullDistance` 提供了控制用户剔除的机制。数组元素 `gl_CullDistance[i]` 指定了平面 `i` 的剔除距离。距离为 0 表示顶点在平面上，正距离表示顶点在剔除体内，负距离表示点在剔除体外。如果顶点的所有剔除平面 `i` 的负裁剪距离，则将丢弃原语。

`gl_CullDistance` 数组是预先声明的，大小未确定，必须由着色器显式地使用大小重新声明，或者通过仅使用整数常量表达式进行索引来隐式确定大小。大小确定了启用的剔除距离的数量和集合，并且最多可以为 `gl_MaxCullDistances`。由 `gl_CullDistance` 消耗的变化分量的数量（参见 `gl_MaxVaryingComponents`）将与数组的大小匹配。写入 `gl_CullDistance` 的着色器必须写入所有启用的距离，否则剔除的结果是未定义的。

作为输出变量，`gl_CullDistance` 提供了着色器写入这些距离的位置。在所有语言中除了片元语言，在这些语言中它是一个输入，读取前一个着色器阶段中写入的值。在片元语言中，`gl_CullDistance` 数组包含由着色器写入 `gl_CullDistance` 顶点输出变量的顶点值的线性插值。

如果组成一个程序的着色器集的 `gl_ClipDistance` 和 `gl_CullDistance` 数组的大小之和大于 `gl_MaxCombinedClipAndCullDistances`，则是一个编译时或链接时错误。

输出变量 `gl_PrimitiveID` 仅在几何语言中可用，提供一个作为原语标识符的整数。然后，这将在片元着色器中作为片元输入 `gl_PrimitiveID` 可用，它将选择从正在着色的原语中引发顶点的已写入的原语 ID。如果激活了使用 `gl_PrimitiveID` 的片元着色器，并且几何着色器也是激活状态的，则几何着色器必须写入 `gl_PrimitiveID`，否则片元着色器输入 `gl_PrimitiveID` 未定义。有关更多信息，请参阅 OpenGL 图形系统规范的第 11.3.4.5 节“几何着色器输出”。

片元着色器使用声明的 `out` 变量将值输出到 OpenGL 流水线，同时使用内置变量 `gl_FragDepth` 和 `gl_SampleMask`，除非执行了 `discard` 语句。

通过读取 `gl_FragCoord.z`，可以获取片元的固定功能计算的深度。写入 `gl_FragDepth` 将建立正在处理的片元的深度值。如果启用了深度缓冲，并且没有着色器写入 `gl_FragDepth`，那么深度的固定功能值将用作片元的深度值。如果着色器静态分配了一个值给 `gl_FragDepth`，并且存在通过该着色器的执行路径，该路径没有设置 `gl_FragDepth`，那么对于采用该路径的着色器执行的片元深度的值可能是未定义的。也就是说，如果链接的片元着色器的集合静态包含对 `gl_FragDepth` 的写入，那么它负责始终写入它。

如果着色器执行了 `discard` 关键字，片元将被丢弃，用户定义的任何片元输出、`gl_FragDepth` 和 `gl_SampleMask` 的值将变得无关紧要。

`gl_FragCoord` 变量在片元着色器中作为输入变量可用，它保存片元的窗口相对坐标（x、y、z、1/w）值。如果启用了多重采样，这个值可以是像素内的任何位置，或者是片元样本中的一个。使用 `centroid` 不会进一步限制这个值在当前基元内。这个值是在顶点处理后插值原语以生成片元的固定功能的结果。`z` 分量是片元深度的深度值，如果没有着色器包含对 `gl_FragDepth` 的写入，将用作片元的深度值。这对于条件计算 `gl_FragDepth` 的着色器在其他情况下想要固定功能片元深度的情况非常有用。

变量 `gl_FragCoord` 在片元着色器中作为输入变量可用，它保存了片元的窗口相对坐标（x、y、z、1/w）值。如果启用了多重采样，该值可以是像素内的任何位置，或者是片元采样点之一。使用 `centroid` 限定符并不进一步将此值限制在当前基元内。该值是固定功能的结果，该功能在顶点处理后插值基元以生成片元。

变量 `gl_FrontFacing` 在片元着色器中可用，其值为 true 表示该片元属于前向基元。其中一个用途是通过选择由顶点或几何着色器计算的两个颜色之一来模拟双面光照。

变量 `gl_PointCoord` 中的值是二维坐标，指示当前片元在启用点精灵时位于点基元的何处。它们在整个点的范围内从 0.0 到 1.0。如果当前基元不是点，或者未启用点精灵，则从 `gl_PointCoord` 读取的值是未定义的。

对于输入数组 `gl_SampleMaskIn[]` 和输出数组 `gl_SampleMask[]`，掩码 `M` 的位 `B`（`gl_SampleMaskIn[M]` 或 `gl_SampleMask[M]`）对应于样本 `32*M+B`。这些数组有 ceil(s/32) 个元素，其中 s 是实现支持的最大颜色样本数。输入变量 `gl_SampleMaskIn` 表示多重采样光栅化期间生成片元的基元覆盖的样本集。仅当样本在此片元着色器调用中被视为已覆盖时，才设置其样本位。

输出数组 `gl_SampleMask[]` 为正在处理的片元设置样本掩码。当前片元的覆盖将成为覆盖掩码与输出 `gl_SampleMask` 的逻辑 AND。此数组必须在片元着色器中以隐式或显式方式调整大小，不得大于实现相关的最大样本掩码（作为32位元素的数组），由样本的最大数量确定。如果片元着色器为 `gl_SampleMask` 静态分配了一个值，那么对于未分配值的任何片元着色器调用的任何数组元素，样本掩码将是未定义的。如果着色器没有为 `gl_SampleMask` 静态分配一个值，样本掩码对片元的处理没有影响。

输入变量 `gl_SampleID` 包含当前正在处理的样本的样本编号。该变量的范围是从 0 到 `gl_NumSamples-1`，其中 `gl_NumSamples` 是帧缓冲区中样本的总数，如果渲染到非多重采样帧缓冲区，则为1。在片元着色器中对此变量的任何静态使用会导致整个着色器在每个样本上进行评估。

输入变量 `gl_SamplePosition` 包含多样本绘制缓冲区中当前样本的位置。`gl_SamplePosition` 的 x 和 y 分量包含当前样本的亚像素坐标，并且其值范围在 0.0 到 1.0 之间。在片元着色器中对此变量的任何静态使用会导致整个着色器在每个样本上进行评估。

如果片元着色器调用被视为辅助调用，变量 `gl_HelperInvocation` 的值为 true，否则为 false。辅助调用是仅为评估用于非辅助片元着色器调用中的导数的目的而创建的片元着色器调用。这样的导数在内建函数 `texture()`（参见第8.9节“纹理函数”）中隐式计算，并且在第8.13.1节“导数函数”中显式计算，例如 `dFdx()` 和 `dFdy()`。

片元着色器中的辅助调用执行与非辅助调用相同的着色器代码，但不会产生修改帧缓冲或其他着色器可访问内存的副作用。具体而言：

- 与辅助调用对应的片元在着色器执行完成后被丢弃，而不会更新帧缓冲。
- 辅助调用执行的对图像和缓冲变量的存储对底层图像或缓冲存储器没有影响。
- 辅助调用执行的对图像、缓冲或原子计数器变量的原子操作对底层图像或缓冲存储器没有影响。这些原子操作返回的值是未定义的。

辅助调用可能会为未被渲染的原语覆盖的像素生成。虽然通常带有 `centroid` 修饰符的片元着色器输入需要在像素和原语的交集中进行采样，但对于这些像素，由于像素与原语之间没有交集，因此可以忽略此要求。

当使用早期片元测试（使用 `early_fragment_tests` 修饰符）杀死片元，或者在实现能够确定执行片元着色器除了帮助计算其他片元着色器调用的导数之外没有其他效果时，可能会生成覆盖由原语渲染的片元的辅助调用。

在处理任何一组原语时生成的辅助调用的集合取决于实现。

片元着色器中的输入变量 `gl_ViewportIndex` 将具有与几何着色器中写入的输出变量 `gl_ViewportIndex` 相同的值。如果几何着色器没有动态分配给 `gl_ViewportIndex`，则片元着色器中的 `gl_ViewportIndex` 的值将是未定义的。如果几何着色器对 `gl_ViewportIndex` 没有进行静态分配，片元阶段将读取零。否则，片元阶段将读取几何阶段写入的相同值，即使该值超出范围。如果片元着色器包含对 `gl_ViewportIndex` 的静态访问，它将计入对片元阶段输入的最大数量的实现定义限制。

片元语言中的输入变量 `gl_Layer` 将具有与几何语言中写入的输出变量 `gl_Layer` 相同的值。如果几何阶段没有动态分配值给 `gl_Layer`，则片元阶段中的 `gl_Layer` 的值将是未定义的。如果几何阶段对 `gl_Layer` 没有进行静态分配，则片元阶段的输入值将为零。否则，片元阶段将读取几何阶段写入的相同值，即使该值超出范围。如果片元着色器包含对 `gl_Layer` 的静态访问，它将计入对片元阶段输入的最大数量的实现定义限制。




#### 7.1.1 兼容性配置文件内建语言变量

在使用兼容性配置文件时，GL可以为可编程顶点和片元管线阶段提供固定功能行为，例如将固定功能顶点阶段与可编程片元阶段混合。

以下内建的顶点、曲面细分控制、曲面细分评估和几何输出变量可用于指定后续可编程着色器阶段或固定功能片元阶段的输入。如果相应的片元着色器或固定管道使用它或从它派生状态，则应写入特定的变量。否则，行为是未定义的。以下成员被添加到输出 `gl_PerVertex` 块中：

```glsl
out gl_PerVertex {
    // 除了其他的 gl_PerVertex 成员之外...
    vec4 gl_ClipVertex;
    vec4 gl_FrontColor;
    vec4 gl_BackColor;
    vec4 gl_FrontSecondaryColor;
    vec4 gl_BackSecondaryColor;
    vec4 gl_TexCoord[];
    float gl_FogFragCoord;
};
```

输出变量 `gl_ClipVertex` 为顶点和几何着色器提供了一个位置，用于写入与用户裁剪平面一起使用的坐标。写入 `gl_ClipDistance` 是用户裁剪的首选方法。如果程序的一组着色器在程序形成时既静态读取或写入 `gl_ClipVertex` 又静态读取或写入 `gl_ClipDistance` 或 `gl_CullDistance`，则是编译时或链接时错误。如果既没有写入 `gl_ClipVertex` 也没有写入 `gl_ClipDistance`，它们的值是未定义的，对用户裁剪平面的任何裁剪也是未定义的。

与核心配置文件中先前描述的相似，`gl_PerVertex` 块可以在着色器中重新声明，以明确包括这些附加成员。例如：

```glsl
out gl_PerVertex {
    vec4 gl_Position;        // 将使用 gl_Position
    vec4 gl_FrontColor;      // 将在片元着色器中使用 gl_Color
    vec4 gl_BackColor;
    vec4 gl_TexCoord[3];     // 将使用 gl_TexCoord 的 3 个元素
};
```

用户必须确保裁剪顶点和用户裁剪平面在相同的坐标空间中定义。用户裁剪平面仅在线性变换下正常工作，在非线性变换下的行为是未定义的。

输出变量 `gl_FrontColor`、`gl_FrontSecondaryColor`、`gl_BackColor` 和 `gl_BackSecondaryColor` 分配了包含正在处理的顶点的前后面的主要和次要颜色。输出变量 `gl_TexCoord` 分配了正在处理的顶点的纹理坐标。对于 `gl_FogFragCoord`，写入的值将用作 OpenGL 图形系统规范兼容性配置文件第 16.4 节“雾”中的 “c” 值，由固定功能管道使用。

例如，如果需要将片段在眼空间中的 z 坐标作为 “c” 值，则顶点着色器可执行应该写入 `gl_FogFragCoord`。

与所有数组一样，用于下标 `gl_TexCoord` 的索引必须是整数常量表达式，或者该数组必须由着色器通过重新声明带有大小的数组来进行定义。大小最多可以是 `gl_MaxTextureCoords`。使用接近 0 的索引可能有助于实现保留变量资源。`gl_TexCoord` 的重新声明也可以在全局范围内完成，例如：

```glsl
in vec4 gl_TexCoord[3];
out vec4 gl_TexCoord[4];
```

（这对于 `gl_TexCoord[]` 是一个特殊情况，不是重新声明块的成员的一般方法。）如果在全局范围重新声明了 `gl_TexCoord[]`，并且存在相应内建块的重新声明，则在着色器中（因此在阶段中，因为块的重新声明必须在所有使用它的着色器中匹配）中不允许重新声明 `gl_TexCoord[]` 的两种形式。如果块的重新声明，则不允许在全局范围重新声明 `gl_TexCoord[]`；只允许在着色器内进行一种形式的重新声明（因此在阶段中，因为块的重新声明必须在所有使用它的着色器中匹配）。

在曲面细分控制、曲面细分评估和几何着色器中，上述先前阶段的输出也可在输入 `gl_PerVertex` 块中使用。

```glsl
in gl_PerVertex {
    vec4 gl_ClipVertex;
    vec4 gl_FrontColor;
    vec4 gl_BackColor;
    vec4 gl_FrontSecondaryColor;
    vec4 gl_BackSecondaryColor;
    vec4 gl_TexCoord[];
    float gl_FogFragCoord;
} gl_in[];
```

这些可以重新声明为建立显式的管线接口，与上述输出块 `gl_PerVertex` 的重新声明一样，输入的重新声明必须与前一阶段的输出重新声明匹配。然而，当重新声明带有实例名称的内建接口块（例如，`gl_in`）时，必须包含实例名称。如果不包括内建实例名称或更改其名称，则会编译时错误。例如：

```glsl
in gl_PerVertex {
    vec4 gl_ClipVertex;
    vec4 gl_FrontColor;
} gl_in[]; // 必须存在，必须是 “gl_in[]”
```

对于 `gl_TexCoord[]` 的重新声明处理也与上述输出块的

 `gl_TexCoord[]` 重新声明相同。

在使用兼容性配置文件时，片段着色器中还提供以下片段输入块：

```glsl
in gl_PerFragment {
    in float gl_FogFragCoord;
    in vec4 gl_TexCoord[];
    in vec4 gl_Color;
    in vec4 gl_SecondaryColor;
};
```

`gl_Color` 和 `gl_SecondaryColor` 中的值将根据哪个面在产生片段的原语中可见，由系统自动从 `gl_FrontColor`、`gl_BackColor`、`gl_FrontSecondaryColor` 和 `gl_BackSecondaryColor` 派生。如果使用固定功能进行顶点处理，那么 `gl_FogFragCoord` 将是片段在眼空间中的 z 坐标，或者是雾坐标的插值，如 OpenGL 图形系统规范兼容性配置文件第 16.4 节“雾”中所述。`gl_TexCoord[]` 的值是从顶点着色器的插值 `gl_TexCoord[]` 值或基于任何基于固定管道的顶点功能的纹理坐标。

与顶点和输出 `gl_PerVertex` 块一样，`gl_PerFragment` 块也可以重新声明为创建与另一个程序的显式接口。在匹配这些接口时，两个程序之间的 gl_PerVertex 输出块的成员必须在 gl_PerFragment 输入块中声明，只有在生成它们的着色器中存在。这些匹配在 OpenGL 图形系统规范的第 7.4.1 节“着色器接口匹配”中有详细描述。如果它们在程序内不匹配，将导致链接时错误。如果不匹配两个程序之间，则在程序之间传递的值是未定义的。与所有其他块匹配不同，`gl_PerFragment` 中声明的顺序不必在着色器之间匹配，并且不必与匹配的 `gl_PerVertex` 重新声明的声明顺序相对应。

在使用兼容性配置文件时，片段着色器中还提供以下片段输出变量：

```glsl
out vec4 gl_FragColor;
out vec4 gl_FragData[gl_MaxDrawBuffers];
```

写入 `gl_FragColor` 指定将由后续固定功能管线使用的片段颜色。如果后续固定功能消耗片段颜色，并且片段着色器可执行的执行没有向 `gl_FragColor` 写入值，则所消耗的片段颜色是未定义的。

变量 `gl_FragData` 是一个数组。写入 `gl_FragData[n]` 指定将由后续固定功能管线用于数据 n 的片段数据。如果后续固定功能消耗片段数据，并且片段着色器可执行的执行没有向其写入值，则所消耗的片段数据是未定义的。

如果一个着色器静态地为 `gl_FragColor` 分配一个值，它可能不会为 `gl_FragData` 的任何元素分配一个值。如果一个着色器静态地写入了 `gl_FragData` 的任何元素的值，它可能不会为 `gl_FragColor` 分配一个值。也就是说，一个着色器可以为 `gl_FragColor` 或 `gl_FragData` 中的一个分配值，但不能同时为两者分配值。链接在一起的多个着色器也必须一致地只写入这些变量中的一个。

类似地，如果正在使用用户声明的输出变量（静态地分配值），则不能将 `gl_FragColor` 和 `gl_FragData` 分配给内置变量。所有这些错误的用法都会生成编译时或链接时错误。

如果着色器执行 `discard` 关键字，则片段将被丢弃，而 `gl_FragDepth` 和 `gl_FragColor` 的值将变得无关紧要。


## 8 Build-in Functions

### 8.9 纹理函数

纹理查找函数在所有着色阶段都是可用的。然而，只有在片段着色器中才会计算自动细节级别。其他着色器的操作就好像基本细节级别是计算为零一样。下表中的函数通过OpenGL API设置的取样器提供对纹理的访问。纹理的属性，如大小、像素格式、维度数、过滤方法、多级贴图级别数、深度比较等，也是通过OpenGL API调用定义的。在访问纹理时，这些属性会被考虑在内，如下所定义的内建函数：

在下面的所有函数中，`bias` 参数对于片段着色器是可选的。在任何其他着色器阶段中，不接受 `bias` 参数。对于片段着色器，如果存在 `bias`，它将被添加到执行纹理访问操作之前的隐式细节级别上。不支持矩形纹理、多采样纹理或纹理缓冲区的无 `bias` 或 `lod` 参数，因为这些类型的纹理不允许使用 mipmaps。

隐式细节级别的选择如下：对于没有使用 mipmaps 的纹理，直接使用该纹理。如果它是 mipmapped，并且在片段着色器中运行，则使用实现计算的 LOD 进行纹理查找。如果它是 mipmapped 并且在顶点着色器中运行，则使用基础纹理。

某些纹理函数（非“Lod”和非“Grad”版本）可能需要隐式导数。在非均匀控制流和非片段着色器纹理获取的情况下，隐式导数是未定义的。



#### 8.9.1 纹理查询函数

textureSize 函数用于查询采样器的特定纹理层级的尺寸。
textureQueryLod 函数仅在片段着色器中可用。它们接受 P 的各个分量，并计算纹理管道在通过正常纹理查找访问该纹理时将使用的详细级别信息。详细级别 λ'（参见OpenGL图形系统规范中的方程3.18）是在任何 LOD 偏移之后获取的，但在夹紧到 [TEXTURE_MIN_LOD, TEXTURE_MAX_LOD] 之前。还计算将要访问的mipmap数组。如果将访问单个详细级别，则返回相对于基本级别的详细级别编号。如果将访问多个详细级别，则返回两个级别之间的浮点数，其中小数部分等于计算和夹紧的详细级别的小数部分。


所使用的算法如下伪代码：

```c
float ComputeAccessedLod(float computedLod)
{
    // 根据纹理LOD夹取范围来夹取计算出的LOD。
    if (computedLod < TEXTURE_MIN_LOD) computedLod = TEXTURE_MIN_LOD;
    if (computedLod > TEXTURE_MAX_LOD) computedLod = TEXTURE_MAX_LOD;

    // 将计算出的LOD夹取到可访问级别的范围。
    if (computedLod < 0.0)
        computedLod = 0.0;
    if (computedLod > (float) maxAccessibleLevel)
        computedLod = (float) maxAccessibleLevel;

    // 根据min filter返回一个值。
    if (TEXTURE_MIN_FILTER is LINEAR or NEAREST) {
        return 0.0;
    } else if (TEXTURE_MIN_FILTER is NEAREST_MIPMAP_NEAREST or LINEAR_MIPMAP_NEAREST) {
        return ceil(computedLod + 0.5) - 1.0;
    } else {
        return computedLod;
    }
}
```

其中，`maxAccessibleLevel` 是mipmap数组中最小可访问级别的级别编号（OpenGL图形系统规范中的第8.14.3节“Mipmapping”中的值 q）减去基本级别。


## 8.13 片段处理函数

片段处理函数仅在片段着色器中可用。

### 8.13.1 导数函数

导数的计算可能在计算上成本高昂且数值不稳定。因此，OpenGL 实现可以通过使用快速但不完全准确的导数计算来近似真实导数。在非均匀控制流中，导数是未定义的。
通过前向差分法：
\[ F(x+ \text{dx}) - F(x) \approx \frac{dF}{dx}(x) \cdot \text{dx} \]（公式1a）
\[ \frac{dF}{dx}(x) \approx \frac{F(x+ \text{dx}) - F(x)}{\text{dx}} \]（公式1b）
通过后向差分法：
\[ F(x- \text{dx}) - F(x) \approx -\frac{dF}{dx}(x) \cdot \text{dx} \]（公式2a）
\[ \frac{dF}{dx}(x) \approx \frac{F(x) - F(x- \text{dx})}{\text{dx}} \]（公式2b）
在单样本光栅化中，公式1b和2b中的 \(\text{dx} \leq 1.0\)。对于多样本光栅化，公式1b和2b中的 \(\text{dx} < 2.0\)。
\(dF/dy\) 的近似类似，用 \(y\) 替换 \(x\)。
对于给定的片段或样本，可能会考虑相邻的片段或样本。
通常，会考虑一个2x2的片段或样本区域，并沿行计算独立的 \(dFdxFine\)，沿列计算独立的 \(dFdyFine\)，同时只计算整个2x2区域的单个 \(dFdxCoarse\) 和单个 \(dFdyCoarse\)。因此，所有二阶粗导数（例如，\(dFdxCoarse(dFdxCoarse(x))\)）可能为0，即使对于非线性参数也是如此。然而，二阶细导数（例如，\(dFdxFine(dFdyFine(x))\)）将正确反映在2x2区域内计算的独立细导数之间的差异。
该方法可能会根据片段而异，受到该方法可能根据窗口坐标而不是屏幕坐标而变化的约束。对于导数计算，放宽了OpenGL图形系统规范第14.2节“不变性”中描述的不变性要求，因为该方法可能是片段位置的函数。
在某些实现中，可以通过提供OpenGL提示（OpenGL图形系统规范第21.4节“提示”）来获得不同程度的 \(dF/dx\) 和 \(dF/dy\) 的导数准确性，允许用户在图像质量与速度之间进行权衡。然而，这些提示对 \(dFdxCoarse\)、\(dFdyCoarse\)、\(dFdxFine\) 和 \(dFdyFine\) 没有影响。

### 8.13.2 插值函数

内建的插值函数可用于计算片段着色器输入变量在着色器指定的 (x, y) 位置的插值值。每次调用内建函数时，可以使用单独的 (x, y) 位置，这些位置可能与用于生成输入默认值的默认 (x, y) 位置不同。
对于所有的插值函数，插值器必须是一个输入变量或声明为数组的输入变量的元素。在指定插值器时，可以使用组件选择运算符（例如 .xy）。对于数组输入，可以使用一般（非均匀）整数表达式进行索引。如果插值器使用 flat 修饰符声明，对于单个基元，插值值将在整个区域内具有相同的值，因此用于插值的位置无效，函数将返回相同的值。
如果插值器使用 centroid 修饰符声明，interpolateAtSample() 和 interpolateAtOffset() 返回的值将在指定的位置进行评估，忽略通常与 centroid 修饰符一起使用的位置。如果插值器使用 noperspective 修饰符声明，将在不进行透视校正的情况下计算插值值。

## 8.14 噪声函数

从 GLSL 版本 4.4 开始，噪声函数 noise1、noise2、noise3 和 noise4 已被弃用。它们被定义为返回值为 0.0 或所有分量都为 0.0 的向量。然而，与之前的版本一样，它们在语义上不被视为编译时常量表达式。





#### 4.4.1.3 片段着色器输入

针对 `gl_FragCoord`，额外的片段布局限定符标识符包括以下两个：
```
layout-qualifier-id :
origin_upper_left
pixel_center_integer
```
默认情况下，`gl_FragCoord` 假定窗口坐标采用左下角为原点，并假定像素中心位于半像素坐标。例如，在窗口中，左下角像素的位置 (0.5, 0.5) 会被返回。可以通过使用 `origin_upper_left` 标识符重新声明 `gl_FragCoord` 来更改原点，将 `gl_FragCoord` 的原点移至窗口的左上角，其中 y 的值向窗口底部增加。返回的值也可以通过 `pixel_center_integer` 标识符在 x 和 y 方向上各偏移半个像素，使得像素看起来位于整数像素偏移的中心。

重新声明的方式如下：
```glsl
in vec4 gl_FragCoord; // 允许不改变的重新声明
// 以下所有的重新声明都是允许的，会改变行为
layout(origin_upper_left) in vec4 gl_FragCoord;
layout(pixel_center_integer) in vec4 gl_FragCoord;
layout(origin_upper_left, pixel_center_integer) in vec4 gl_FragCoord;
```

如果在程序的任何片段着色器中重新声明了 `gl_FragCoord`，则在该程序中所有使用 `gl_FragCoord` 的片段着色器都必须重新声明它，并且在同一程序中所有片段着色器对 `gl_FragCoord` 的所有重新声明都必须具有相同的一组限定符。在任何着色器中，对 `gl_FragCoord` 的第一个重新声明必须出现在对 `gl_FragCoord` 的任何使用之前。内建的 `gl_FragCoord` 仅在片段着色器中被预声明，因此在任何其他着色器语言中重新声明它将导致编译时错误。

使用 `origin_upper_left` 和/或 `pixel_center_integer` 限定符重新声明 `gl_FragCoord` 只影响 `gl_FragCoord.x` 和 `gl_FragCoord.y`。它对光栅化、变换或 OpenGL 管道或语言功能的任何其他部分都没有影响。

片段着色器还允许在 `in` 上使用以下布局限定符（仅在变量声明中不适用）：
```
layout-qualifier-id :
early_fragment_tests
```
请求在片段着色器执行之前执行片段测试，如 OpenGL 规范的第 15.2.4 节“早期片段测试”中所描述。

例如：
```glsl
layout(early_fragment_tests) in;
```
指定将在片段着色器执行之前执行每个片段的测试。如果未声明此内容，片段测试将在片段着色器执行后执行。只需要一个片段着色器（编译单元）声明这个，尽管可以有多个。如果至少有一个声明了这个，那么它就被启用。

### 4.4.2 Output Layout Qualifiers

一些输出布局修饰符适用于所有着色器语言，而一些仅适用于特定语言。后者在下面的各个小节中讨论。

与输入布局修饰符一样，除了计算着色器之外的所有着色器都允许在输出变量声明、输出块声明和输出块成员声明上使用位置布局修饰符。其中，变量和块成员（但不包括块本身）还允许使用分量布局修饰符。


#### 4.4.2.4 片段输出

内建的片段着色器变量 `gl_FragDepth` 可以通过以下布局限定符之一进行重新声明。
```
layout-qualifier-id :
depth_any
depth_greater
depth_less
depth_unchanged
```
例如：
```glsl
layout(depth_greater) out float gl_FragDepth;
```

`gl_FragDepth` 的布局限定符约束了任何着色器调用写入的 `gl_FragDepth` 的最终值的意图。OpenGL 实现允许执行优化，假设对于给定片段，如果所有与布局限定符一致的 `gl_FragDepth` 的值都会失败（或通过），则深度测试将失败（或通过）。这可能包括跳过着色器执行，如果片段被丢弃因为被遮挡且着色器没有副作用。如果 `gl_FragDepth` 的最终值与其布局限定符不一致，则相应片段的深度测试结果未定义。然而，在这种情况下不会生成错误。如果深度测试通过并且启用深度写入，则写入深度缓冲区的值始终为 `gl_FragDepth` 的值，无论它是否与布局限定符一致。

默认情况下，`gl_FragDepth` 被限定为 `depth_any`。当 `gl_FragDepth` 的布局限定符为 `depth_any` 时，着色器编译器将注意到对 `gl_FragDepth` 的任何修改都以未知的方式修改它，深度测试将在着色器执行后始终执行。当布局限定符为 `depth_greater` 时，GL 可以假设 `gl_FragDepth` 的最终值大于或等于片段的插值深度值，即由 `gl_FragCoord` 的 z 分量给出。当布局限定符为 `depth_less` 时，GL 可以假设对 `gl_FragDepth` 的任何修改只会减小其值。当布局限定符为 `depth_unchanged` 时，着色器编译器将遵守对 `gl_FragDepth` 的任何修改，但GL的其余部分可以假定 `gl_FragDepth` 没有被赋予新值。

重新声明 `gl_FragDepth` 的方式如下：
```glsl
// 允许不改变的重新声明
out float gl_FragDepth;
// 假设它可能以任何方式修改
layout(depth_any) out float gl_FragDepth;
// 假设它可能以只增加的方式修改
layout(depth_greater) out float gl_FragDepth;
// 假设它可能以只减小的方式修改
layout(depth_less) out float gl_FragDepth;
// 假设它不会被修改
layout(depth_unchanged) out float gl_FragDepth;
```

如果在程序的任何片段着色器中重新声明了 `gl_FragDepth`，则在该程序中所有对 `gl_FragDepth` 有静态赋值的片段着色器中都必须重新声明它，并且在同一程序中所有片段着色器对 `gl_FragDepth` 的所有重新声明都必须具有相同的一组限定符。在任何着色器中，对 `gl_FragDepth` 的第一个重新声明必须出现在对 `gl_FragDepth` 的任何使用之前。内建的 `gl_FragDepth` 仅在片段着色器中被预声明，因此在任何其他着色器语言中重新声明它将导致编译时错误。

