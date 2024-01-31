# RadeonSI寄存器分析 

首先分管线中各个阶段他们的设置使用到的寄存器。

# 3D Engine Programing 中的寄存器配置

在R6xx_R7xx_3D.pdf该书中寄存器配置分为   CP , PA ,SQ ,Vertex resource , Texture , Shader , 	SPI ,	 DB, CB , Draw,.

不过在South Islands中， 已经移除所有的常量硬件寄存器， 把这些资源放在内存中。 所以关于这些Vertex resource ,Texture寄存器已经被 **Shader Resource Descriptors (SRD)** 代替。 这类寄存器有SHADER BUFFER RESOURCE DESCRIPTOR， SHADER IMAGE RESOURCE DESCRIPTOR ， SHADER IMAGE RESOURCE SAMPLER DESCRIPTOR 。
接下来逐个分析各类寄存器

# CP（Command Processor) setup

CP微码必须加载，并且必须像以前的芯片一样初始化环形缓冲区以及读写指针，然后才能使用CP。R6xx/R7xx芯片与以前的一代芯片不同，因为必须为CP微引擎（ME）和CP预取解析器（PFP）分别加载微码。

* 表面同步数据包序列

通过一个单一的接口，PM4命令流和主机CPU可以确定渲染是否完成到一个表面，并刷新和使缓存无效。该机制基于使用四个寄存器（CP_COHER_STATUS、CP_COHER_CNTL、CP_COHER_SIZE、CP_COHER_BASE），状态管理逻辑以及SSU逻辑，以确定何时所有排队的渲染完成到一个表面。

在纹理设置时，驱动程序将同步源缓存，在绘制图元时，驱动程序将同步目标缓存。
源缓存位于纹理缓存（TC）、顶点缓存（VC）和序列指令缓存（SH）。对于每个这些提取缓存，与表面范围相交的每个缓存行都将被使无效。使源缓存无效与使目标缓存无效的方式不同。驱动程序将使用源缓存来写入CP_COHER_CNTL寄存器，以使指定的表面无效，但不会设置任何基地址寄存器。然后，它会设置CP_COHER_BASE。

对于目标缓存，表面一致性机制可以选择将CP_COHER_BASE地址与一个或多个硬件目标地址进行比较。CP中的目标地址寄存器包括：
1. CB_COLOR_BASE[7:0]
2. DB_DEPTH_BASE
3. VGT_STRMOUT_BUFFER_BASE[3:0]
4. COHER_DEST_BASE[1:0]

比较过程仅在每次写入CP_COHER_BASE寄存器时发生。

# 1. VGT(Vertex Grouper Tessellattor ) Setup 

这个对应Vertex Grouper and Tessellator Registers 

## IA_ENHANCE (0x28a70) 

"Late Additions of Control Bits"（晚期添加控制位）这个术语在计算机硬件或数字设计的上下文中，指的是在系统或电路的设计后期，将额外的控制信号或位加入到系统中的做法。

在数字系统中，控制位用于管理和控制各种功能或操作。它们决定了数据在系统内部的处理、路由和操作方式。在初始设计阶段，工程师可能无法预料到所有需要的控制信号，或者可能还没有最终确定系统行为的某些方面。因此，当有更多信息或需求可用时，可能会在后期添加或修改一些控制位。

晚期添加控制位可能是由于以下原因：

1. **需求演变：** 随着系统需求和规格的演变，可能需要引入新的控制信号以适应变化。

2. **调试和优化：** 在系统开发的早期，可能无法完全预测到最佳设计，因此可能需要在后期根据实际需求添加或调整控制位，以实现更好的性能或功能。

3. **外部接口：** 当与其他组件或设备进行集成时，可能需要在后期添加额外的控制位，以满足接口要求。

晚期添加控制位是一种常见的设计实践，允许在设计过程中灵活地适应变化和优化系统功能。


#### IA_MULTI_VGT_PARAM (0x28aa8) 

看描述是明确多个VGT配置信息
解释如下

| 字段名称            | 位   | 默认值 | 描述                                                                                                                                                                                                                           |
|-------------------|------|--------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PRIMGROUP_SIZE     | 15:0 | 0xFF   | 在切换到下一个VGT块之前发送到一个VGT块的图元数量。隐含加1（0 = 1 prim/group; 255 = 256 prims/group）。将此值设置大于255会导致性能下降。对于PATCH图元，此值不应大于（（256 /输入控制点数量）- 1）。对于tessellation，应将其设置为线程组每个patch的倍数。如果将此值设置为0（1 prim/group），则在内部视为1（2 prims/group）。如果PARTIAL_ES_WAVE_ON为OFF并且启用了streamout，2个SE设计的primgroup大小必须小于256。对于相邻的primtypes，它应该小于128。在Major Mode 1中，primgroup_size编程不能超过63。       |
| PARTIAL_VS_WAVE_ON | 16   | 0x0    | 如果设置了此位，则VGT将在一个primgroup完成后立即发出vswave。否则，VGT将在绘制调用内的一个primgroup到下一个primgroup继续vswave。如果启用了streamout，则必须启用此选项。                                                                                                               |
| SWITCH_ON_EOP      | 17   | 0x0    | 如果设置了此位，则IA将在数据包边界处切换VGT块，否则将根据PRIMGROUP_SIZE的编程来切换。如果在没有Tess的情况下使用Major Mode 1，即Passthru等，则必须设置为1。                                                                                               |
| PARTIAL_ES_WAVE_ON | 18   | 0x0    | 如果设置了此位，则VGT将在一个primgroup完成后立即发出eswave。否则，VGT将在绘制调用内的一个primgroup到下一个primgroup继续eswave。                                                                                                 |
| SWITCH_ON_EOI      | 19   | 0x0    | 如果设置了此位，则IA将在实例边界的末尾切换VGT块，否则将根据PRIMGROUP_SIZE的编程来切换。如果使用tessellation并且需要在HS中使用prim_id，则必须将此设置为1。如果设置了此项，则PARTIAL_ES_WAVE_ON必须设置。                                                   |

该寄存器在radeonsi中si_state_draw.c中的si_emit_draw_registers函数中用到,用来发射绘制状态。属于SET_CONTEXT_REG。 
流程：

```mermaid

graph TD

	A[si_draw_vbo] --> B[si_emit_all_states]
	B -.Emit draw states .-> C[si_emit_draw_registers]
	C -.set IA_MULTI_VGT_PARAM.-> D[radeon_set_context_reg]

```


#### VGT_CACHE_INVALIDATION 

这个寄存器用于指定是通过VC（Vertex Cache）还是TC（Texture Cache）进行ES2GS和GS2VS环形缓冲区的缓存失效。在低成本的部件中，可能没有VC存在，因此所有的ES2GS/GS2VS环形缓冲区的获取都是通过TC进行的，因此缓存失效将通过TC进行。

| 字段名称               | 位    | 默认值 | 描述                                                                                                                                                      |
|----------------------|-------|--------|---------------------------------------------------------------------------------------------------------------------------------------------------------|
| VS_NO_EXTRA_BUFFER   | 5     | 0x0    | 如果设置为1，则禁用gs_on位。                                                                                                                         |
| STREAMOUT_FULL_FLUSH | 13    | 0x0    | 如果设置为1，SO_VGTSTREAMOUT_FLUSH事件的工作方式类似于R7xx和之前的版本。VGT在通知CP之前等待VS线程完成。                                                     |
| ES_LIMIT             | 20:16 | 0x0    | 性能调整参数，用于限制ES波浪提前于GS波浪的程度。这是允许在ESGS环形缓冲区中存在的ES波浪数量。该字段经过位移，因此表示2的幂值，即ES波浪数量为2的ES_LIMIT次幂。 |


#### VGT_DMA_BASE 

这是一个只写寄存器。为了保持一致性，VGT DMA控制寄存器有8个地址集。写入特定的地址集对于VGT DMA控制寄存器与写入任何其他一对VGT DMA控制寄存器是相同的。换句话说，写入这个寄存器和其他相关的VGT DMA控制寄存器是类似的，没有特殊的区别或注意事项。


| 字段名称    | 位      | 默认值 | 描述                                                                                                                                                     |
|-----------|---------|--------|----------------------------------------------------------------------------------------------------------------------------------------------------------|
| BASE_ADDR | 31:0    | none   | VGT DMA基地址。                                                                                                                                           |
|           |         |        | 此地址必须自然对齐到一个16位字（word）。因此，该寄存器的第0位必须为0。                                                                                 |

该寄存器名为BASE_ADDR，占据32个位，用于存储VGT DMA的基地址。必须确保此地址是以16位字（word）为单位的自然对齐。这意味着该地址的最低有效位（bit 0）必须为0，确保地址按照16位对齐，而不是8位对齐或其他方式。对于有效访问和正确功能，应注意按照规定对BASE_ADDR进行设置。

#### VGT_DMA_BASE_HI

根据提供的信息，VGT_DMA_BASE_HI是一个只写寄存器，占据32位，用于VGT DMA控制寄存器的地址设置。为了保持一致性，VGT DMA控制寄存器有8个地址集。写入特定的地址集对于VGT DMA控制寄存器与写入任何其他一对VGT DMA控制寄存器是相同的。该寄存器包含DMA基地址的高8位（即40位地址中的位31到位24）。

具体字段如下：

| 字段名称    | 位   | 默认值 | 描述                                                 |
|-----------|------|--------|------------------------------------------------------|
| BASE_ADDR | 7:0  | none   | DMA地址的高8位，用于设置40位DMA地址中的位31到位24。|

根据描述，VGT_DMA_BASE_HI寄存器的作用是设置DMA的高位地址，它与VGT_DMA_BASE_LO寄存器（用于设置低位地址）一起组成完整的40位DMA地址。请注意，该寄存器是只写的，因此只能通过写入来配置DMA地址的高8位。

#### VGT_DMA_INDEX_TYPE

根据提供的信息，VGT_DMA_INDEX_TYPE是一个只写寄存器，占据32位，用于配置VGT DMA控制寄存器的索引类型和数据交换模式。为了保持一致性，VGT DMA控制寄存器有8个地址集。写入特定的地址集对于VGT DMA控制寄存器与写入任何其他一对VGT DMA控制寄存器是相同的。

具体字段如下：

| 字段名称     | 位    | 默认值 | 描述                                                              |
|------------|-------|--------|-------------------------------------------------------------------|
| INDEX_TYPE | 1:0   | none   | VGT DMA索引类型。                                                 |
|            |       |        | 可能的值：                                                        |
|            |       |        | 00 - VGT_INDEX_16：16位索引。                                       |
|            |       |        | 01 - VGT_INDEX_32：32位索引。                                       |
| SWAP_MODE  | 3:2   | none   | DMA数据交换模式。                                                 |
|            |       |        | 可能的值：                                                        |
|            |       |        | 00 - VGT_DMA_SWAP_NONE：无交换。                                     |
|            |       |        | 01 - VGT_DMA_SWAP_16_BIT：16位交换，0xAABBCCDD -> 0xBBAADDCC。       |
|            |       |        | 02 - VGT_DMA_SWAP_32_BIT：32位交换，0xAABBCCDD -> 0xDDCCBBAA。       |
|            |       |        | 03 - VGT_DMA_SWAP_WORD：字交换，0xAABBCCDD -> 0xCCDDAABB。            |

根据描述，VGT_DMA_INDEX_TYPE寄存器用于设置VGT DMA的索引类型和数据交换模式。其中，INDEX_TYPE字段用于选择索引类型（16位索引或32位索引），而SWAP_MODE字段用于指定DMA数据的交换模式（无交换、16位交换、32位交换或字交换）。请注意，该寄存器是只写的，因此只能通过写入来配置这些字段的值。

#### VGT_DMA_MAX_SIZE

根据提供的信息，VGT_DMA_MAX_SIZE是一个只写寄存器，占据32位，用于处理索引越界问题。驱动程序将此寄存器设置为小于或等于VGT_DMA_SIZE的值，指定从索引缓冲区中读取多少个实际有效的数据。如果VGT_MAX_SIZE < VGT_DMA_SIZE，则在VGT中将其余的获取的索引设置为零。

具体字段如下：

| 字段名称   | 位    | 默认值 | 描述                                               |
|----------|-------|--------|----------------------------------------------------|
| MAX_SIZE | 31:0  | none   | VGT DMA最大索引数量，直到访问越界的索引缓冲区。|

根据描述，VGT_DMA_MAX_SIZE寄存器用于指定在访问越界的索引缓冲区之前，从索引缓冲区中读取的最大索引数量。驱动程序应该将该寄存器的值设置为小于或等于VGT_DMA_SIZE，以确保读取的数据不会超出索引缓冲区的范围。如果VGT_MAX_SIZE小于VGT_DMA_SIZE，则VGT会将其余获取的索引设置为零，避免访问越界。请注意，由于该寄存器是只写的，因此只能通过写入来配置MAX_SIZE字段的值。

#### VGT_DMA_NUM_INSTANCES 

根据提供的信息，VGT_DMA_NUM_INSTANCES是一个只写寄存器，占据32位，用于指定绘制调用中指定的实例数量值。如果未启用实例，则此寄存器设置为零或一。该寄存器的GPUF0MMReg地址为0x28a88。

具体字段如下：

| 字段名称         | 位     | 默认值 | 描述                                             |
|----------------|--------|--------|--------------------------------------------------|
| NUM_INSTANCES  | 31:0   | none   | VGT DMA实例数量，最小值为1。                              |

根据描述，VGT_DMA_NUM_INSTANCES寄存器用于指定绘制调用中的实例数量。如果绘制调用未启用实例化，则此寄存器设置为0或1。请注意，该寄存器是只写的，因此只能通过写入来配置NUM_INSTANCES字段的值。最小值为1，即至少要有一个实例。

地址说明：GPUF0MMReg：0x28a88 表示该寄存器在GPU寄存器空间中的地址为0x28a88。这是在GPU内部的物理地址，用于对该寄存器进行读写操作。



#### VGT_DMA_SIZE

VGT_DMA_SIZE是一个只写寄存器，占据32位，用于指定VGT DMA控制寄存器中的索引数量。为了保持一致性，VGT DMA控制寄存器有8个地址集。写入特定的地址集对于VGT DMA控制寄存器与写入任何其他一对VGT DMA控制寄存器是相同的。该寄存器的GPUF0MMReg地址为`0x28a74`。

具体字段如下：

| 字段名称      | 位     | 默认值 | 描述                                              |
|-------------|--------|--------|---------------------------------------------------|
| NUM_INDICES | 31:0   | none   | VGT DMA索引数量。                                   |

根据描述，VGT_DMA_SIZE寄存器用于指定VGT DMA控制寄存器中的索引数量。该寄存器是只写的，用于配置索引数量的值。请注意，该寄存器是只写的，因此只能通过写入来配置NUM_INDICES字段的值。

#### VGT_DRAW_INITIATOR

VGT_DRAW_INITIATOR是一个只写寄存器，占据32位，用于触发执行绘制数据包（2D或3D）。该寄存器的GPUF0MMReg地址为`0x287f0`。

该寄存器用于在VGT中触发绘制数据包的执行。写入该寄存器是一个触发器，启动VGT中的处理过程。VGT_DRAW_INITIATOR寄存器有8个地址，但在Wekiva芯片中并没有8个寄存器的副本。写入特定地址的绘制启动器寄存器会导致将一个状态上下文分配给绘制触发器的其中一个八个状态上下文。该状态上下文分配会向下传播，并被芯片中所有涉及执行此绘制触发器的各个部件使用。以下是关于绘制启动器寄存器中信息的描述。

具体字段如下：

| 字段名称         | 位     | 默认值 | 描述                                           |
|----------------|--------|--------|------------------------------------------------|
| SOURCE_SELECT  | 1:0   | none   | 输入源选择。                                   |
|                |        |        | 如果源选择字段设置为“自动增量索引”模式，并且基本类型设置为“带标志的三角形列表”，则绘制启动器将被处理为普通的“三角形列表”。 |
|                |        |        | 可能的值：                                     |
|                |        |        | 00 - DI_SRC_SEL_DMA：VGT DMA数据                |
|                |        |        | 01 - DI_SRC_SEL_IMMEDIATE：立即数据              |
|                |        |        | 02 - DI_SRC_SEL_AUTO_INDEX：自动增量索引        |
|                |        |        | 03 - DI_SRC_SEL_RESERVED：保留 - 未使用           |
| MAJOR_MODE     | 3:2   | none   | 主模式。                                       |
|                |        |        | 可能的值：                                     |
|                |        |        | 00 - DI_MAJOR_MODE_0：正常（隐式）模式 - 仅适用于基本类型0-21。在此模式中，某些VGT状态寄存器将被忽略（其值隐含）。 |
|                |        |        | 01 - DI_MAJOR_MODE_1：显式模式 - 配置完全由状态寄存器指定。                                   |
| NOT_EOP        | 5      | none   | 此位表示该绘制启动器不应产生数据包结束信号，因为它将后跟一个或多个链式绘制启动器。必须确保该绘制启动器立即在硬件接口上后跟一个链式绘制启动器。换句话说，链式绘制启动器不能分隔在可以中断的驱动程序缓冲区边界上。该位主要由CP设置，以改善小型2D位块的处理并行性。 |
|                |        |        | 可能的值：                                     |
|                |        |        | 00 - 正常的eop                                |
|                |        |        | 01 - 抑制eop                                 |
| USE_OPAQUE     | 6      | none   | 此位表示此绘制调用是不透明绘制调用。          |
|                |        |        | 可能的值：                                     |
|                |        |        | 00 - 非不透明绘制                             |
|                |        |        | 01 - 不透明绘制                               |

根据描述，VGT_DRAW_INITIATOR寄存器用于触发执行绘制数据包（2D或3D）。写入该寄存器是一个触发器，启动VGT中的处理过程。通过写入特定地址的绘制启动器寄存器，可以将其中一个八个状态上下文分配给绘制触发器。该寄存器具有多个字段，用于选择输入源、主模式以及设置是否生成结束信号等。这些字段用于配置绘制数据包的处理方式。请注意，VGT_DRAW



#### VGT_ENHANCE

VGT_ENHANCE是一个可读写寄存器，占据32位，用于用于添加控制位的后期扩展。

该寄存器的GPUF0MMReg地址为`0x28a50`。

具体字段如下：

| 字段名称  | 位    | 默认值 | 描述      |
|---------|-------|--------|-----------|
| MISC    | 31:0  | none   | 杂项位。  |

根据描述，VGT_ENHANCE寄存器用于后期添加控制位。该寄存器具有一个名为"Misc"的32位字段，但没有提供更详细的说明或默认值。这可能意味着这个字段的具体功能在描述中没有详细解释，或者可能在不同的硬件版本中有不同的含义。由于缺乏具体信息，我们无法进一步解释这个字段的作用和功能。如果需要了解更多细节，最好查阅相关的技术文档或硬件规格表。

#### VGT_ESGS_RING_ITEMSIZE 

VGT_ESGS_RING_ITEMSIZE是一个可读写寄存器，占据32位，用于指定写入ESGS环形缓冲区的每个顶点的大小。

该寄存器的GPUF0MMReg地址为`0x28aac`。

具体字段如下：

| 字段名称   | 位     | 默认值 | 描述                                      |
|----------|--------|--------|-------------------------------------------|
| ITEMSIZE | 14:0   | none   | 指定以双字为单位的大小。必须至少为4个双字，并且必须是4个双字的倍数。  |

根据描述，VGT_ESGS_RING_ITEMSIZE寄存器用于指定写入ESGS环形缓冲区的每个顶点的大小。大小以双字为单位，字段的范围为14位，即可以表示的最大值为2^14个双字。然而，它必须至少为4个双字，并且必须是4个双字的倍数。这是因为ESGS环形缓冲区的写入需要满足一定的对齐和大小要求。通过配置ITEMSIZE字段的值，可以确保每个顶点写入ESGS环形缓冲区时满足这些要求。

该寄存器在radeonsi中si_state_shaders.c中的si_emit_shader_es用到

```mermaid
graph TD 
si_shader_es -.设置atom.emit.-> si_emit_shader_es
si_draw_vbo-->si_emit_all_states--> A
A[si_pm4_emit] --> B[si_emit_shader_es] 

```


#### VGT_ESGS_RING_SIZE

VGT_ESGS_RING_SIZE是一个可读写寄存器，占据32位，用于指定ESGS环形缓冲区的大小，以256字节的倍数来表示。

该寄存器的GPUF0MMReg地址为`0x88c8`。

具体字段如下：

| 字段名称   | 位     | 默认值 | 描述                                      |
|----------|--------|--------|-------------------------------------------|
| MEM_SIZE | 31:0   | none   | 指定以256字节为单位的大小。对于双着色器引擎的部分，大小必须设置为512字节的倍数，因为环形缓冲区的一半用于每个着色器引擎。  |

根据描述，VGT_ESGS_RING_SIZE寄存器用于指定ESGS环形缓冲区的大小，以256字节的倍数表示。字段MEM_SIZE表示环形缓冲区的大小，以256字节为单位。对于双着色器引擎的部分，环形缓冲区的大小必须设置为512字节的倍数，因为环形缓冲区的一半将用于每个着色器引擎。通过配置MEM_SIZE字段的值，可以确定ESGS环形缓冲区的实际大小。


改寄存器在radeonsi中的si_state_shaders.c中的si_update_gs_ring_buffers中用到。

```mermaid
graph TD
	si_update_shaders -.update GS .-> A
	A[si_update_gs_ring_buffer ] --> si_pm4_set_reg

```


#### VGT_ES_PER_GS

VGT_ES_PER_GS是一个可读写寄存器，占据32位，用于指定每个GS线程处理的最大ES顶点数。

该寄存器的GPUF0MMReg地址为`0x28a58`。

具体字段如下：

| 字段名称   | 位       | 默认值 | 描述                                |
|----------|----------|--------|-------------------------------------|
| ES_PER_GS | 10:0     | none   | 每个GS线程处理的最大ES顶点数。        |

根据描述，VGT_ES_PER_GS寄存器用于指定每个GS线程处理的最大ES顶点数。字段ES_PER_GS的范围为11位，因此可以表示的最大值为2^11，即2048。通过配置ES_PER_GS字段的值，可以确定GS线程在处理ES阶段时可以处理的最大ES顶点数。



```mermaid
graph TD

si_create_context --> si_init_state_functions --> si_init_config
si_init_config --> si_pm4_set_reg

```


#### VGT_EVENT_INITIATOR  

VGT_EVENT_INITIATOR是一个可写寄存器，占据32位，用于触发事件。

该寄存器的GPUF0MMReg地址为`0x28a90`。

这个寄存器字段描述较多， 需要结合原手册。

具体字段如下：

| 字段名称         | 位     | 默认值 | 描述                                                                                                              |
|----------------|--------|--------|-------------------------------------------------------------------------------------------------------------------|
| EVENT_TYPE     | 5:0    | none   | 事件类型（也称为事件ID）--目前，VGT和PA之间的硬件接口仅支持6位事件类型。                                                  |
| ADDRESS_HI     | 26:18  | none   | 用于ZPASS事件的地址位39:31。                                                                                          |
| EXTENDED_EVENT | 27     | none   | 0表示单个DW事件，1表示双DW事件。                                                                                      |

根据描述，VGT_EVENT_INITIATOR寄存器用于触发事件。EVENT_TYPE字段指定事件类型，共有多种可选值，例如用于样本统计、缓存刷新、性能计数等。不同的EVENT_TYPE值对应不同的事件，具体含义和功能由每个EVENT_TYPE决定。

ADDRESS_HI字段用于ZPASS事件，指定了事件地址的高位。

EXTENDED_EVENT字段用于指示事件是否是扩展事件，0表示单个DW事件，1表示双DW事件。

通过配置EVENT_TYPE字段，可以触发不同的事件，进而控制GPU的不同行为或功能。

#### VGT_GROUP_DECR 

VGT_GROUP_DECR是一个可读写寄存器，占据32位，用于控制绘制初始化器索引计数的递减量，适用于除第一组之外的所有从输入流中获取的组。

该寄存器的GPUF0MMReg地址为`0x28a2c`。

具体字段如下：

| 字段名称 | 位   | 默认值 | 描述                                      |
|--------|------|--------|-------------------------------------------|
| DECR   | 3:0  | none   | 除了第一组之外的所有组的递减量。              |

根据描述，在Major Mode 0下，对于原始类型（Prim Types）0到21，该寄存器将被忽略，不起作用。而在其他模式下，该寄存器控制从输入流中获取的所有组，除了第一组之外，绘制初始化器索引计数的递减量。

通过配置DECR字段的值，可以调整绘制初始化器索引计数在不同组之间的递减量，从而影响绘制操作的处理方式。

#### VGT_GSVS_RING_ITEMSIZE 

VGT_GSVS_RING_ITEMSIZE是一个可读写寄存器，占据32位，用于指定写入GSVS环形缓冲区的每个图元的大小。

该寄存器的GPUF0MMReg地址为`0x28ab0`。

具体字段如下：

| 字段名称   | 位    | 默认值 | 描述                                  |
|----------|-------|--------|---------------------------------------|
| ITEMSIZE | 14:0  | none   | 指定以双字（dword）为单位的大小。必须至少为4个dword，并且必须是4个dword的倍数。 |

根据描述，VGT_GSVS_RING_ITEMSIZE寄存器用于设置写入GSVS环形缓冲区的每个图元的大小。图元大小以双字（dword）为单位进行指定，并且必须至少为4个dword，并且必须是4个dword的倍数。

通过配置ITEMSIZE字段的值，可以确定GSVS环形缓冲区中每个图元的大小，以适应特定的图元数据格式和需求。

```mermaid
graph TD
si_shader_gs -.set si_emit_shader_gs.-> si_emit_shader_gs
si_pm4_emit--> si_emit_shader_gs
si_emit_shader_gs -.R_028AB0_VGT_GSVS_RING_ITEMSIZE.-> radeon_opt_set_context_reg

```

#### VGT_GSVS_RING_OFFSET_1 VGT_GSVS_RING_OFFSET_2 VGT_GSVS_RING_OFFSET_3 {#VGT_GSVS_RING_OFFSET_1 }

VGT_GSVS_RING_OFFSET_1-3是一个可读写寄存器，占据32位，用于设置GSVS环形缓冲区的偏移量。

该寄存器的GPUF0MMReg地址为`0x28a60`。

具体字段如下：

| 字段名称   | 位    | 默认值 | 描述                           |
|----------|-------|--------|--------------------------------|
| OFFSET   | 14:0  | none   | 用于指定GSVS环形缓冲区的偏移量。|

GSVS环形缓冲区是图形Shader（GS）和视口Shader（VS）之间的中间缓冲区，用于在图元传递阶段存储GS处理后的图元数据。通过配置OFFSET字段的值，可以设置GSVS环形缓冲区中的偏移量，以确定GS数据存储在缓冲区中的位置。这样，VS阶段就可以正确地读取GS产生的图元数据进行后续处理。

```mermaid
graph TD
si_emit_shader_gs -.VGT_GSVS_RING_OFFSET_1,2,3.-> radeon_opt_set_context_reg4

```

#### VGT_GSVS_RING_SIZE

VGT_GSVS_RING_SIZE是一个可读写寄存器，占据32位，用于设置GSVS环形缓冲区的大小。

该寄存器的GPUF0MMReg地址为`0x88cc`。

具体字段如下：

| 字段名称   | 位    | 默认值 | 描述                                                                                                 |
|----------|-------|--------|------------------------------------------------------------------------------------------------------|
| MEM_SIZE | 31:0  | none   | 用于指定GSVS环形缓冲区的大小，以256字节为单位的倍数表示。对于双着色器引擎部分，大小必须是512字节的倍数，因为环形缓冲区的一半用于每个着色器引擎。|

GSVS环形缓冲区是用于存储GS处理后的图元数据，供视口Shader（VS）使用的缓冲区。通过配置MEM_SIZE字段的值，可以设置GSVS环形缓冲区的大小，以256字节为单位的倍数表示。对于双着色器引擎（dual shader engine）的GPU部分，GSVS环形缓冲区的大小必须是512字节的倍数，因为环形缓冲区的一半用于每个着色器引擎，确保数据的正确处理和读取。


这个寄存器radeonsi中使用流程和VGT_ESGS_RING_SIZE一样。


#### VGT_GS_INSTANCE_CNT

VGT_GS_INSTANCE_CNT是一个可读写寄存器，占据32位，用于指定GS（几何着色器）图元实例化的数量。

该寄存器的GPUF0MMReg地址为`0x28b90`。

具体字段如下：

| 字段名称   | 位    | 默认值 | 描述                                                                                               |
|----------|-------|--------|----------------------------------------------------------------------------------------------------|
| ENABLE   | 0     | none   | 用于启用或禁用GS实例化。                                                                            |
| CNT      | 8:2   | none   | 用于指定GS图元实例的数量。如果设置为0，表示GS实例化被视为关闭，不提供实例ID。|

GS实例化是一种图形渲染技术，可以使用几何着色器在单次绘制调用中生成多个相似的图元实例。通过配置ENABLE字段来启用或禁用GS实例化，通过配置CNT字段来指定GS图元实例的数量。如果将CNT字段设置为0，则表示GS实例化被视为关闭，此时不提供实例ID。启用GS实例化后，GS将根据实例数量多次处理输入图元，并生成相应数量的实例输出。

该寄存器使用流程同[VGT_GSVS_RING_OFFSET_1](#VGT_GSVS_RING_OFFSET_1)



#### VGT_GS_MAX_VERT_OUT

VGT_GS_MAX_VERT_OUT是一个可读写寄存器，占据32位，用于指定GS（几何着色器）每个图元输出的最大顶点数。

该寄存器的GPUF0MMReg地址为`0x28b38`。

具体字段如下：

| 字段名称      | 位      | 默认值 | 描述                                                                                                                                                                      |
|-------------|---------|--------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| MAX_VERT_OUT | 10:0    | none   | GS场景C：在场景C中，VGT使用该寄存器来确定GS输出的每个图元的最大顶点数。PA（光栅化器）根据着色器的处理结果来构造图元。                                                                         |
| MAX_VERT_OUT | 10:0    | none   | GS场景G：在场景G中，如果启用10xx选项，则VGT将根据该值对GS着色器的输出顶点数进行截断（之前会自动截断为默认值1024）。该寄存器在复位时没有默认值，如果不需要该特性，API应在初始化时将其设置为1024。|

VGT_GS_MAX_VERT_OUT寄存器用于控制GS输出的顶点数量。在不同的场景下（场景C和场景G），该寄存器的作用稍有不同。在场景C中，VGT根据该寄存器的设置来决定每个图元输出的最大顶点数，并由PA根据着色器生成的数据构造图元。而在场景G中，如果启用了10xx选项，VGT将对GS着色器的输出顶点数进行截断，使其不超过MAX_VERT_OUT指定的最大值。在寄存器复位时，没有默认值，如果不需要此功能，则应在初始化时将其设置为1024。

该寄存器使用流程同[VGT_GSVS_RING_OFFSET_1](#VGT_GSVS_RING_OFFSET_1)

#### VGT_GS_MODE 


```mermaid
graph LR
si_shader_init_pm4_state --> si_shader_vs
si_init_shader_selector_async -->
si_shader_vs --> si_emit_shader_vs
si_emit_shader_vs --> radeon_opt_set_context_reg

```

#### VGT_GS_OUT_PRIM_TYPE

VGT_GS_OUT_PRIM_TYPE是一个可读写寄存器，占据32位，用于控制VGT（顶点图形处理单元）中GS（几何着色器）的输出图元类型。

该寄存器的GPUF0MMReg地址为`0x28a6c`。

具体字段如下：

| 字段名称                  | 位       | 默认值 | 描述                                                                                                                                              |
|-------------------------|----------|--------|---------------------------------------------------------------------------------------------------------------------------------------------------|
| OUTPRIM_TYPE             | 5:0      | 0x0    | GS输出图元类型，表示流0的图元类型。                                                                                                                      |
| OUTPRIM_TYPE_1           | 13:8     | 0x0    | GS输出图元类型，表示流1的图元类型。                                                                                                                      |
| OUTPRIM_TYPE_2           | 21:16    | 0x0    | GS输出图元类型，表示流2的图元类型。                                                                                                                      |
| OUTPRIM_TYPE_3           | 27:22    | 0x0    | GS输出图元类型，表示流3的图元类型。                                                                                                                      |
| UNIQUE_TYPE_PER_STREAM   | 31       | 0x0    | 如果为1，则OUTPRIM_TYPE字段表示流0的图元类型。如果为0，则OUTPRIM_TYPE字段适用于所有流。                                                                                                 |

该寄存器使用流程同[VGT_GSVS_RING_OFFSET_1](#VGT_GSVS_RING_OFFSET_1)

###  VGT_GS_PER_ES

VGT_GS_PER_ES是一个可读写寄存器，占据32位，用于控制VGT（顶点图形处理单元）中每个ES（顶点着色器）线程所处理的最大GS（几何着色器）图元数量。

该寄存器的GPUF0MMReg地址为`0x28a54`。

具体字段如下：

| 字段名称     | 位    | 默认值 | 描述                                                     |
|------------|-------|--------|----------------------------------------------------------|
| GS_PER_ES  | 10:0  |  none  | 每个ES线程所处理的最大GS图元数量。                          |
| PARTIAL_ES_WAVE_ON | 16    |  none  | 如果为1，则允许ES线程在不完整的波束上执行。如果为0，则ES线程将完整地执行。如果GS_PER_ES / primgroup_size大于(GPU_VGT__GS_TABLE_DEPTH - 3)，则该位必须为0。  |
R_028A54_VGT_GS_PER_ES,

该寄存器使用流程同VGT_ES_PER_GS



#### VGT_GS_PER_VS

VGT_GS_PER_VS是一个可读写寄存器，占据32位，用于控制VGT（顶点图形处理单元）中每个VS（顶点着色器）线程所处理的最大GS（几何着色器）线程数量。

该寄存器的GPUF0MMReg地址为`0x28a5c`。

具体字段如下：

| 字段名称   | 位    | 默认值 | 描述                         |
|----------|-------|--------|------------------------------|
| GS_PER_VS | 3:0   | none   | 每个VS线程所处理的最大GS线程数量。 |


该寄存器使用流程同VGT_ES_PER_GS

#### VGT_GS_VERT_ITEMSIZE1-3

VGT_GS_VERT_ITEMSIZE是一个可读写寄存器，占据32位，用于指定写入GSVS（几何着色器-视口裁剪-存储器）环形缓冲区的Stream (0-3)中每个顶点的大小。


具体字段如下：

| 字段名称   | 位      | 默认值 | 描述                               |
|------------|-----------|--------|------------------------------------|
| ITEMSIZE   | 14:0     | none   | 指定的顶点大小，单位为dword（双字）。 |

该寄存器使用流程同[VGT_GSVS_RING_OFFSET_1](#VGT_GSVS_RING_OFFSET_1)



#### VGT_HOS_MAX_TESS_LEVEL

VGT_HOS_MAX_TESS_LEVEL是一个可读写寄存器，占据32位，用于在使用细分器时指定硬件将应用于提取的细分因子的最大细分级别限制。

该寄存器的GPUF0MMReg地址为`0x28a18`。

具体字段如下：

| 字段名称   | 位      | 默认值 | 描述                                        |
|------------|-----------|--------|---------------------------------------------|
| MAX_TESS   | 31:0     | none   | 允许的细分级别范围为(0.0, 64.0)。如果输入的细分因子是NaN、负数或零，则不会受到此值的限制。 |


该寄存器使用流程同VGT_ES_PER_GS

#### VGT_HOS_MIN_TESS_LEVEL

VGT_HOS_MIN_TESS_LEVEL是一个可读写寄存器，占据32位，用于在使用细分器时指定硬件将应用于提取的细分因子的最小细分级别限制。

该寄存器的GPUF0MMReg地址为`0x28a1c`。

具体字段如下：

| 字段名称   | 位      | 默认值 | 描述                                        |
|------------|-----------|--------|---------------------------------------------|
| MIN_TESS   | 31:0     | none   | 允许的细分级别范围为(0.0, 64.0)。如果输入的细分因子是NaN、负数或零，则不会受到此值的限制。 |


####  VGT_HS_OFFCHIP_PARAM

VGT_HS_OFFCHIP_PARAM是一个可写寄存器，占据32位，用于控制Offchip HS（细分表面图元离线化）模式的操作参数。

该寄存器的GPUF0MMReg地址为`0x89b0`。

具体字段如下：

| 字段名称           | 位      | 默认值 | 描述                                                         |
|--------------------|-----------|--------|--------------------------------------------------------------|
| OFFCHIP_BUFFERING | 6:0       | 0x0   | 可用的离线缓冲区数量，范围从1到64个8K双字缓冲区。 |

这些字段用于控制Offchip HS的工作方式，包括可用的离线缓冲区数量，以及其他相关参数。通过写入这些字段，可以配置细分表面图元的离线化过程。


```mermaid
graph TD

si_update_shaders --> si_init_tess_factor_ring --> si_pm4_set_reg

```

#### VGT_INDEX_TYPE

VGT_INDEX_TYPE是一个可写寄存器，占据32位，用于配置VGT（Vertex Graphics Table）的索引类型。

该寄存器的GPUF0MMReg地址为`0x895c`。

具体字段如下：

| 字段名称      | 位     | 默认值 | 描述                                                                                                      |
|---------------|----------|--------|-----------------------------------------------------------------------------------------------------------|
| INDEX_TYPE   | 1:0      | none   | 索引类型（仅适用于prim类型0-28）。如果Source Select字段设置为“Auto-increment Index”模式，则忽略此字段，索引类型为32位/索引。 |
|                      |                |        | 可能的值：                                                                                                 |
|                      |                |        | 00 - DI_INDEX_SIZE_16_BIT: 16位/索引。                                                                      |
|                      |                |        | 01 - DI_INDEX_SIZE_32_BIT: 32位/索引。                                                                      |

通过写入这些字段，可以配置VGT的索引类型，选择每个索引占用的位数，可以是16位或32位。该配置仅适用于特定的图元类型（prim类型0-28），如果Source Select字段设置为“Auto-increment Index”模式，则索引类型为32位。


```mermaid
graph TD

si_draw_vbo -->
si_emit_draw_packets --> radeon_set_uconfig_reg_idx

```

#### VGT_INDX_OFFSET 

VGT_INDX_OFFSET是一个可读写寄存器，占据32位，用于对指定为索引的组件进行偏移。该寄存器仅适用于Ring 0。

该寄存器的GPUF0MMReg地址为`0x28408`。

具体字段如下：

| 字段名称      | 位      | 默认值 | 描述                                                                                         |
|---------------|-----------|--------|------------------------------------------------------------------------------------------------|
| INDX_OFFSET  | 31:0      | none   | 索引偏移值（32位加法器），将其扩展为32位。                                               |

通过写入INDX_OFFSET字段，可以对被指定为索引的组件进行偏移。偏移操作发生在进行夹取和固定点转浮点转换之前。

该寄存器使用流程同VGT_ES_PER_GS

#### VGT_INSTANCE_STEP_RATE_0-1

VGT_INSTANCE_STEP_RATE_x是一个可读写寄存器，占据32位，用于定义第x个实例的步进率。


具体字段如下：

| 字段名称      | 位      | 默认值 | 描述                                    |
|---------------|-----------|--------|-------------------------------------------|
| STEP_RATE    | 31:0      | none   | 实例的步进率（Instance step rate）。| 

通过写入STEP_RATE字段，可以定义第x个实例的步进率。

该寄存器使用流程同VGT_ES_PER_GS

#### VGT_LS_HS_CONFIG

VGT_LS_HS_CONFIG是一个可读写寄存器，占据32位，用于指定LS/HS（Local Shader / Hull Shader）的控制值。

该寄存器的GpuF0MMReg地址为`0x28b58`。

具体字段如下：

| 字段名称            | 位      | 默认值 | 描述                                             |
|-----------------------|-----------|--------|-------------------------------------------------------|
| NUM_PATCHES        | 7:0       | none   | 指示一个线程组中的patch（图元片段）数量。         | 
| HS_NUM_INPUT_CP   | 13:8      | none   | HS输入patch（图元片段）中的控制点数量。         | 
| HS_NUM_OUTPUT_CP | 19:14   | none   | HS输出patch（图元片段）中的控制点数量。       | 

通过写入这些字段，可以指定LS/HS的控制值。

```mermaid
graph LR
si_draw_vbo-->
si_emit_all_states --> si_emit_derived_tess_state-->
radeon_set_context_reg_idx

```


#### VGT_MAX_VTX_INDX

VGT:VGT_MAX_VTX_INDX是一个可读写寄存器，占据32位，用于索引值（indices）的最大值截断。该寄存器是针对特定的环（Ring）而设定的，但只存在于环0中。索引值在指定为索引的组件中时（请参阅VGT_GROUP_VECT_0_FMT_CNTL寄存器），会在偏移之后和fix->flt（固定点数到浮点数）转换之前进行截断。

该寄存器的GpuF0MMReg地址为`0x28400`。

具体字段如下：

| 字段名称       | 位     | 默认值 | 描述                                      |
|----------------|--------|--------|------------------------------------------|
| MAX_INDX       | 31:0 | none   | 索引值的最大截断值，扩展为32位。   |

该寄存器使用流程同VGT_ES_PER_GS

#### VGT_MIN_VTX_INDX

VGT:VGT_MIN_VTX_INDX是一个可读写寄存器，占据32位，用于索引值（indices）的最小值截断。该寄存器是针对特定的环（Ring）而设定的，但只存在于环0中。索引值在指定为索引的组件中时（请参阅VGT_GROUP_VECT_0_FMT_CNTL寄存器），会在偏移之后和fix->flt（固定点数到浮点数）转换之前进行截断。

该寄存器的GpuF0MMReg地址为`0x28404`。

具体字段如下：

| 字段名称       | 位     | 默认值 | 描述                                      |
|----------------|--------|--------|------------------------------------------|
| MIN_INDX       | 31:0 | none   | 索引值的最小截断值，扩展为32位。   |

通过写入这些字段，可以指定索引值的最大截断值和最小截断值。

该寄存器使用流程同VGT_ES_PER_GS

####  VGT_MULTI_PRIM_IB_RESET_EN

VGT:VGT_MULTI_PRIM_IB_RESET_EN是一个可读写寄存器，占据32位，用于启用基于重置索引（reset index）的图元重置。该寄存器允许根据重置索引来重置图元。

该寄存器的GpuF0MMReg地址为`0x28a94`。

具体字段如下：

| 字段名称       | 位     | 默认值 | 描述                                      |
|----------------|--------|--------|------------------------------------------|
| RESET_EN       | 0     | none   | 如果设置为1，则启用重置索引来重置图元。   |

通过写入该字段，可以启用或禁用基于重置索引的图元重置功能。如果设置为1，重置索引将用于重置图元。如果设置为0，将禁用图元重置功能。

该寄存器使用流程同IA_MULTI_VGT_PARAM

#### VGT_MULTI_PRIM_IB_RESET_INDX

VGT:VGT_MULTI_PRIM_IB_RESET_INDX是一个可读写寄存器，占据32位，用于指定用于重置图元顺序（strip/fan/polygon）的32位索引值。当IB（Index Buffer）中的索引值与该寄存器中的值匹配时，将开始一个新的图元集。

该寄存器的GpuF0MMReg地址为`0x2840c`。

具体字段如下：

| 字段名称    | 位     | 默认值 | 描述                          |
|-------------|--------|--------|----------------------------------|
| RESET_INDX  | 31:0 | none   | 用于重置图元顺序的32位索引值。 |

通过写入RESET_INDX字段，可以指定用于重置图元顺序的索引值。当IB中的索引值等于该寄存器中的值时，会触发开始一个新的图元集。

该寄存器使用流程同IA_MULTI_VGT_PARAM


####  **VGT_NUM_INDICES**

VGT:VGT_NUM_INDICES是一个可写寄存器，占据32位，用于指定要处理的索引数量。需要注意的是，这个计数不一定是图元的计数，也不是索引缓冲在内存中的大小。当使用计算着色器（compute shaders）时，驱动程序需要将这个寄存器写入x、y和z的乘积，这三个维度定义了计算着色器线程组的大小。

该寄存器的GpuF0MMReg地址为`0x8970`。

具体字段如下：

| 字段名称      | 位     | 默认值 | 描述                                     |
|---------------|--------|--------|---------------------------------------------|
| NUM_INDICES   | 31:0 | none   | 指定要处理的索引数量。 |

通过写入NUM_INDICES字段，可以指定要处理的索引数量。

#### **VGT_NUM_INSTANCES**

VGT:VGT_NUM_INSTANCES是一个可写寄存器，占据32位，用于指定绘制调用中的实例数量。如果设置为零，则会被解释为1个实例。最大允许的值是2^32-1。

该寄存器的GpuF0MMReg地址为`0x8974`。

具体字段如下：

| 字段名称         | 位     | 默认值 | 描述                                        |
|------------------|--------|--------|--------------------------------------------------|
| NUM_INSTANCES   | 31:0 | none   | 指定绘制调用中的实例数量。如果设置为零，则会被解释为1个实例。最大允许的值是2^32-1。 |

通过写入NUM_INSTANCES字段，可以指定绘制调用中的实例数量。


这个在radeonsi没有显式用到，而是用的PKT3_NUM_INSTANCES

```mermaid
graph TD

	si_draw_vbo--> si_emit_draw_packets --> radeon_emit  

```

#### VGT_OUTPUT_PATH_CNTL

VGT:VGT_OUT_DEALLOC_CNTL是一个可读写寄存器，占据32位，用于控制在处理向量中上一个处理向量何时被释放（de-allocated）。

该寄存器的GpuF0MMReg地址为`0x28c5c`。

具体字段如下：

| 字段名称         | 位     | 默认值 | 描述                                       |
|------------------|--------|--------|-------------------------------------------------|
| DEALLOC_DIST   | 6:0   | none   | 控制在处理向量中上一个处理向量何时被释放（de-allocated）。从r7xx开始，该寄存器应该被设置为16。 |

通过设置DEALLOC_DIST字段，可以控制在处理向量中上一个处理向量的释放距离。在r7xx架构及之后，该寄存器应设置为16。

该寄存器使用流程同VGT_ES_PER_GS


#### VGT_PRIMITIVEID_EN

VGT:VGT_PRIMITIVEID_EN是一个可读写寄存器，占据32位，用于启用32位原始ID（PrimitiveID）的生成。

该寄存器的GpuF0MMReg地址为`0x28a84`。

具体字段如下：

| 字段名称                 | 位     | 默认值 | 描述                                                                                      |
|----------------------------|--------|--------|-----------------------------------------------------------------------------------------------|
| PRIMITIVEID_EN      | 0       | none   | 启用原始ID（PrimitiveID）的生成。                                                     |
| DISABLE_RESET_ON_EOI | 1       | none   | 决定原始ID（PrimitiveID）是否在每个实例的结束时重置。        |

通过设置PRIMITIVEID_EN字段，可以启用或禁止原始ID（PrimitiveID）的生成。可能的值如下：
- 00: 禁止原始ID（PrimitiveID）输出
- 01: 输出原始ID（PrimitiveID）

通过设置DISABLE_RESET_ON_EOI字段，可以决定原始ID（PrimitiveID）是否在每个实例的结束时重置。可能的值如下：
- 00: 原始ID（PrimitiveID）在每个实例和数据包结束时重置
- 01: 原始ID（PrimitiveID）仅在数据包结束时重置

该寄存器使用流程同VGT_GS_MODE

#### VGT_PRIMITIVEID_RESET

VGT:VGT_PRIMITIVEID_RESET是一个可读写寄存器，占据32位，用于指定用户指定的32位起始原始ID（PrimitiveID）值，该值在每个新的图元中递增。

该寄存器的GpuF0MMReg地址为`0x28a8c`。

具体字段如下：

| 字段名称 | 位      | 默认值 | 描述                   |
|----------|----------|--------|---------------------------|
| VALUE    | 31:0     | 0x0    | 原始ID（PrimitiveID）的重置值。 |

通过设置VALUE字段，用户可以指定原始ID（PrimitiveID）的起始值。在每个新的图元生成时，该值将递增。初始值可以用于生成唯一的原始ID，以便在图元处理过程中进行标识和追踪。

该寄存器使用流程同VGT_ES_PER_GS

#### VGT_PRIMITIVE_TYPE

VGT:VGT_PRIMITIVE_TYPE是一个可写寄存器，占据32位，用于指定VGT（Vertex Graphics Tesselator）的原始类型。

该寄存器的GpuF0MMReg地址为`0x8958`。

具体字段如下：

| 字段名称   | 位      | 默认值 | 描述                                 |
|-------------|----------|--------|----------------------------------------|
| PRIM_TYPE   | 5:0      | none   | 原始类型。仅在Major mode 0中使用。对于Major Mode 1，使用VGT_GRP_PRIM_TYPE寄存器中指定的原始类型。 |

通过设置PRIM_TYPE字段，可以指定要在VGT中使用的原始类型。不同的原始类型对应不同的图元类型，如点、线段、三角形等。这可以影响图元的绘制方式和渲染效果。注意，在Major Mode 1中，这个寄存器不会被使用，而是使用VGT_GRP_PRIM_TYPE寄存器中的原始类型。

该寄存器使用流程同IA_MULTI_VGT_PARAM



####  VGT_REUSE_OFF

VGT:VGT_REUSE_OFF是一个可读写寄存器，占据32位，用于控制VS（Vertex Shader）处理向量生成中的重用。

该寄存器的GpuF0MMReg地址为`0x28ab4`。

具体字段如下：

| 字段名称     | 位    | 默认值 | 描述                 |
|---------------|--------|--------|------------------------|
| REUSE_OFF     | 0        | none    | 是否关闭重用。设置为1表示重用关闭，设置为0表示重用打开。 |

通过设置REUSE_OFF字段，可以控制VS处理向量的生成是否进行重用。重用指的是在VS过程中，是否允许对之前处理过的数据进行再次使用，以减少冗余计算。将REUSE_OFF设置为1将关闭重用功能，而设置为0将打开重用功能。注意，该寄存器只会关闭VS处理向量的重用，对于ES（Exposure Shader）处理向量的重用不受影响。此外，重用也将在流输出和视口绘制过程中被关闭。



该寄存器使用流程同VGT_GS_MODE

#### VGT_SHADER_STAGES_EN

VGT:VGT_SHADER_STAGES_EN是一个可读写寄存器，占据32位，用于指定启用哪些着色器阶段。在更改某些组合时，可能需要进行VGT_FLUSH或PIPE FLUSH操作。

该寄存器的GpuF0MMReg地址为`0x28b54`。

具体字段如下：

| 字段名称     | 位    | 默认值 | 描述                 |
|---------------|--------|--------|------------------------|
| LS_EN         | 1:0    | none   | 控制LS（Local Shader）阶段的行为。 |
| HS_EN         | 2      | none   | 控制HS（Hull Shader）阶段的行为。 |
| ES_EN         | 4:3    | none   | 控制ES（Exposure Shader）阶段的行为。 |
| GS_EN         | 5      | none   | 控制GS（Geometry Shader）阶段的行为。 |
| VS_EN         | 7:6    | none   | 控制VS（Vertex Shader）阶段的行为。 |
| DYNAMIC_HS    | 8      | none   | 表示HS阶段的输出是否始终保留在芯片上（Evergreen模式）或者动态决定使用离散式存储器（off-chip memory），从而使用多个SIMD来执行后续DS波束。 |

各字段取值的含义如下：

- LS_EN：控制LS阶段的行为，可选值为LS_STAGE_OFF、LS_STAGE_ON、CS_STAGE_ON和RESERVED_LS。
- HS_EN：控制HS阶段的行为，可选值为HS_STAGE_OFF和HS_STAGE_ON。
- ES_EN：控制ES阶段的行为，可选值为ES_STAGE_OFF、ES_STAGE_DS、ES_STAGE_REAL和RESERVED_ES。
- GS_EN：控制GS阶段的行为，可选值为GS_STAGE_OFF和GS_STAGE_ON，但要求VGT_GS_MODE.bits.MODE必须设置为SCENARIO_G。
- VS_EN：控制VS阶段的行为，可选值为VS_STAGE_REAL、VS_STAGE_DS、VS_STAGE_COPY_SHADER和RESERVED_VS。
- DYNAMIC_HS：表示HS阶段的输出方式，可选值为hs_onchip和hs_dynamic_off_chip。

通过设置这些字段，可以启用或禁用不同的着色器阶段以满足特定的着色器需求。


```mermaid
graph TD
	si_update_shaders -->
	si_update_vgt_shader_config-->si_pm4_set_reg
```



#### VGT_STRMOUT_BUFFER_CONFIG

VGT:VGT_STRMOUT_BUFFER_CONFIG是一个可读写寄存器，占据32位，用于启用流输出（Stream Out）功能。流输出允许GPU将处理过的图元数据流写入缓冲区，通常用于GPU计算和几何着色阶段的输出。

该寄存器的GpuF0MMReg地址为`0x28b98`。

具体字段如下：

| 字段名称            | 位     | 默认值 | 描述                      |
|---------------------|---------|--------|---------------------------|
| STREAM_0_BUFFER_EN   | 3:0     | 0x0    | 绑定流0的缓冲区。位0设置为1表示绑定缓冲区0，位1表示绑定缓冲区1，位2表示绑定缓冲区2，位3表示绑定缓冲区3。 |
| STREAM_1_BUFFER_EN   | 7:4     | 0x0    | 绑定流1的缓冲区。位0设置为1表示绑定缓冲区0，位1表示绑定缓冲区1，位2表示绑定缓冲区2，位3表示绑定缓冲区3。 |
| STREAM_2_BUFFER_EN   | 11:8    | 0x0    | 绑定流2的缓冲区。位0设置为1表示绑定缓冲区0，位1表示绑定缓冲区1，位2表示绑定缓冲区2，位3表示绑定缓冲区3。 |
| STREAM_3_BUFFER_EN   | 15:12   | 0x0    | 绑定流3的缓冲区。位0设置为1表示绑定缓冲区0，位1表示绑定缓冲区1，位2表示绑定缓冲区2，位3表示绑定缓冲区3。 |

通过设置这些字段，可以指定哪些流输出缓冲区将被启用，从而使得GPU可以将处理过的图元数据流写入相应的缓冲区。


该寄存器使用流程同VGT_ES_PER_GS

 

#### VGT_STRMOUT_BUFFER_SIZE_0-3

VGT:VGT_STRMOUT_BUFFER_SIZE_x是一个可读写寄存器，占据32位，用于设置流输出（Stream Out）缓冲区x的大小。


具体字段如下：

| 字段名称 | 位    | 默认值 | 描述                         |
|----------|-------|--------|------------------------------|
| SIZE     | 31:0 | none   | 流输出缓冲区0的大小（以DWORD为单位）。 |

通过设置SIZE字段，可以指定流输出缓冲区0的大小，以DWORD为单位。GPU将处理过的图元数据流写入该缓冲区，当达到指定大小后，GPU将停止写入并继续处理下一个图元。


这个寄存器在state_streamout.c文件中si_emit_streamout_begin中用到。

```mermaid
graph TD
si_init_streamout_functions -.初始化si_context atoms.s.streamout_begin.emit.->  si_init_streamout_functions
si_emit_streamout_begin
```

#### VGT_STRMOUT_CONFIG

VGT:VGT_STRMOUT_CONFIG是一个可读写寄存器，占据32位，用于启用流输出（Stream Out）功能。

该寄存器的GpuF0MMReg地址为`0x28b94`。

具体字段如下：

| 字段名称             | 位    | 默认值 | 描述                                                         |
|-----------------------|-------|--------|----------------------------------------------------------------|
| STREAMOUT_0_EN    | 0     | 0x0    | 如果设置为1，启用到流输出流0的流输出。              |
| STREAMOUT_1_EN    | 1     | 0x0    | 如果设置为1，启用到流输出流1的流输出。              |
| STREAMOUT_2_EN    | 2     | 0x0    | 如果设置为1，启用到流输出流2的流输出。              |
| STREAMOUT_3_EN    | 3     | 0x0    | 如果设置为1，启用到流输出流3的流输出。              |
| RAST_STREAM        | 6:4 | 0x0    | 启用光栅化的流，如果设置了bit[6]，则对于任何流都不启用光栅化。|
| RAST_STREAM_MASK | 11:8 | 0x0    | 表示哪个流已启用的掩码。仅在USE_RAST_STREAM_MASK为1时有效。 |
| USE_RAST_STREAM_MASK | 31   | 0x0    | 当设置为1时，将使用RAST_STREAM_MASK。设置为0时，使用RAST_STREAM。 |

通过设置这些字段，可以选择启用特定的流输出和光栅化功能。流输出允许将GPU处理过的图元数据流写入指定的缓冲区，以供后续处理和存储。光栅化则用于决定哪些图元将被传递到图形流水线的后续阶段进行处理。


```mermaid
graph TD
si_init_streamout_functions -.初始化atoms.s.streamout_enable.emit.-> si_emit_streamout_enable
si_emit_streamout_enable-->radeon_set_context_reg_seq

```

#### VGT_STRMOUT_DRAW_OPAQUE_OFFSET

VGT:VGT_STRMOUT_DRAW_OPAQUE_OFFSET是一个可读写寄存器，占据32位，用于设置绘制不透明物体时的偏移量。

该寄存器的GpuF0MMReg地址为`0x28b28`。

具体字段如下：

| 字段名称 | 位    | 默认值 | 描述                                                                                                                                                                                |
|-------------|-------|--------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| OFFSET      | 31:0 | none   | 从IASetVertexBuffers绑定的流输出缓冲区的pOffsets（偏移量），用作源数据。如果从中减去BufferFilledSize（检索到的BufferFilledSize）是正数，则会确定可以从中创建基元的数据量。|

该寄存器用于指定用作源数据的流输出缓冲区的偏移量。在绘制不透明物体时，CP会根据从IASetVertexBuffers绑定的流输出缓冲区检索到的BufferFilledSize和该寄存器的偏移量，确定可以从缓冲区中创建基元的数据量。这有助于确保绘制不透明物体时数据的正确处理。


该寄存器使用流程同VGT_ES_PER_GS

#### VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE 

VGT:VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE是一个可读写寄存器，占据32位，用于设置绘制不透明物体时的顶点跨距。

该寄存器的GpuF0MMReg地址为`0x28b30`。

具体字段如下：

| 字段名称         | 位     | 默认值 | 描述                                                                                           |
|-------------------|--------|--------|--------------------------------------------------------------------------------------------------|
| VERTEX_STRIDE | 8:0 | none   | 绘制不透明调用中使用的顶点跨距。|

该寄存器用于指定绘制不透明物体时的顶点跨距。顶点跨距是指相邻顶点之间在缓冲区中的字节偏移量。它决定了如何在缓冲区中读取顶点数据以绘制不透明物体。通过设置该寄存器，可以调整顶点数据在缓冲区中的布局和排列方式，以满足绘制需求。


```mermaid
graph TD
	si_draw_vbo--> si_emit_draw_packets -->radeon_set_context_reg

```

####  VGT_TF_MEMORY_BASE

VGT:VGT_TF_MEMORY_BASE是一个可写寄存器，占据32位，用于设置细分因子存储器的基地址。

该寄存器的GpuF0MMReg地址为`0x89b8`。

具体字段如下：

| 字段名称 | 位    | 默认值 | 描述                                    |
|----------|-------|--------|-------------------------------------------|
| BASE     | 31:0  | 0x0    | 细分因子存储器的基地址。地址需要256字节对齐，有效地址位为[39:8]。 |

细分因子存储器用于存储细分曲面细分因子的数据。细分因子存储器基地址可以由程序员设置，以指定存储细分因子数据的内存位置。通常，细分因子存储器位于显存中，并且需要按照256字节对齐的方式进行存放。

设置该寄存器的基地址，可以将细分因子数据存储到所需的内存位置，供硬件使用进行细分曲面绘制。


```mermaid

graph TD

si_update_shaders-->si_init_tess_factor_ring--> si_pm4_set_reg

```


#### VGT_TF_PARAM

VGT:VGT_TF_PARAM是一个可读写寄存器，占据32位，用于指定细分引擎的控制参数。

该寄存器的GpuF0MMReg地址为`0x28b6c`。

具体字段如下：

| 字段名称           | 位      | 默认值 | 描述                                       |
|------------------|---------|--------|----------------------------------------------|
| TYPE             | 1:0     | 无     | 使用的细分类型（域）                         |
| PARTITIONING     | 4:2     | 无     | 使用的分割类型                               |
| TOPOLOGY         | 7:5     | 无     | 输出原语的拓扑                               |
| RESERVED_REDUC_AXIS | 8     | 无     | 不再使用，现在变为保留字段                       |
| NUM_DS_WAVES_PER_SIMD | 13:10 | 无    | 在将数据写入多个SIMD之前，将多少DS波（ES/VS）发送到同一个SIMD |
| DISABLE_DONUTS   | 14      | 无     | 决定细分器中使用的行走模式                     |

这些字段的详细说明如下：

- TYPE字段（1:0）：用于指定细分类型（域），它决定了细分引擎使用的细分域类型。可能的取值包括：
  - 00: TESS_ISOLINE - 线段细分
  - 01: TESS_TRIANGLE - 三角形细分
  - 02: TESS_QUAD - 四边形细分

- PARTITIONING字段（4:2）：用于指定分割类型，它决定了细分引擎如何对输入图元进行分割。可能的取值包括：
  - 00: PART_INTEGER - 整数分割
  - 01: PART_POW2 - 2的幂分割
  - 02: PART_FRAC_ODD - 奇数分数分割
  - 03: PART_FRAC_EVEN - 偶数分数分割

- TOPOLOGY字段（7:5）：用于指定输出原语的拓扑，即细分后形成的原始图元类型。可能的取值包括：
  - 00: OUTPUT_POINT - 输出点
  - 01: OUTPUT_LINE - 输出线段
  - 02: OUTPUT_TRIANGLE_CW - 输出顺时针三角形
  - 03: OUTPUT_TRIANGLE_CCW - 输出逆时针三角形

- RESERVED_REDUC_AXIS字段（8）：这个字段曾经用于规约轴，但是现在不再需要，已变为保留字段。

- NUM_DS_WAVES_PER_SIMD字段（13:10）：指定在将数据写入多个SIMD之前，将多少DS波（ES/VS）发送到同一个SIMD。这影响了数据在细分引擎内的处理方式。

- DISABLE_DONUTS字段（14）：确定在细分器中使用的行走模式。可能的取值包括：
  - 00: 使用donut行走模式以获得最佳复用
  - 01: 使用单环行走模式

通过设置这些控制参数，可以配置细分引擎的行为，以满足不同细分需求。细分引擎用于将低细节的几何图元转换为更高细节的曲面，通常用于细节丰富的图形渲染。

流程同VGT_ESGS_RING_ITEMSIZE

```mermaid
graph TD

si_shader_es -.设置atom.emit.-> si_emit_shader_es,gs
si_emit_shader_es--> radeon_opt_set_context_reg
si_emit_shader_gs--> radeon_opt_set_context_reg

```

####  VGT_TF_RING_SIZE

VGT:VGT_TF_RING_SIZE是一个可读写寄存器，占据32位，用于指定细分因子缓冲区的大小。

该寄存器的GpuF0MMReg地址为`0x8988`。

具体字段如下：

| 字段名称 | 位     | 默认值 | 描述                                                     |
|----------|--------|--------|--------------------------------------------------------|
| SIZE     | 15:0   | 0x2000 | 细分因子缓冲区的大小（以双字为单位），在具有双VGT的项目中，该环会在两个VGT之间内部划分。 |

这个字段的详细说明如下：

- SIZE字段（15:0）：用于指定细分因子缓冲区的大小，以双字为单位。细分因子是在细分过程中计算得出的值，用于控制细分程度。细分因子缓冲区是一个用于存储细分因子的内存区域。该字段可以设置为合适的值，以适应细分引擎的需求。

在具有双VGT的项目中，VGT（Vertex Generation Unit，顶点生成单元）有两个，而细分因子缓冲区在这两个VGT之间进行内部划分，因此SIZE字段的设置应该考虑到这种划分。

通过设置SIZE字段，可以调整细分因子缓冲区的大小，以满足不同细分引擎的需求和限制。


使用流程同VGT_HS_OFFCHIP_PARAM

####  VGT_VERTEX_REUSE_BLOCK_CNTL

VGT:VGT_VERTEX_REUSE_BLOCK_CNTL 是一个可读写寄存器，占据32位，用于控制VGT（Vertex Generation Unit，顶点生成单元）后端的顶点复用块（Vertex Reuse Block）的行为。该寄存器仅在VGT_OUTPUT_PATH_CNTL寄存器（或Major Mode 0中的prim type）指定顶点复用块作为VGT后端路径时才有效。

该寄存器的GpuF0MMReg地址为 0x28c58。

具体字段如下：

| 字段名称 | 位      | 默认值 | 描述                                                                                     |
|-----------|---------|---------|----------------------------------------------------------------------------------------|
| VTX_REUSE_DEPTH | 7:0   | none    | 从r7xx开始，复用深度（VTX_REUSE_DEPTH）应设置为14。也可以设置为15（如果prim type是线）和16（如果prim type是点）。  |

这个字段的详细说明如下：

- VTX_REUSE_DEPTH字段（7:0）：用于控制顶点复用块的复用深度。顶点复用是一种优化技术，它允许在图形渲染过程中复用已经计算过的顶点数据，从而减少顶点数据的计算和传输开销，提高性能。

在该字段中，可以设置复用深度的值。复用深度表示在处理新的绘制命令时，复用块会保留多少个前面处理过的顶点数据。例如，如果复用深度设置为14，则复用块会保留最近处理的14个顶点数据，以便在新的绘制命令中复用它们。

在r7xx以及之后的版本中，复用深度建议设置为14。如果绘制类型（prim type）是线，则可以设置为15；如果绘制类型是点，则可以设置为16。具体的设置取决于应用的需求和硬件支持的特性。

通过调整VTX_REUSE_DEPTH字段的值，可以优化顶点复用块的行为，从而获得更好的性能和效率。


```mermaid

graph TD
si_emit_shader_es --> radeon_set_context_reg
si_emit_shader_gs-->  radeon_set_context_reg
si_emit_shader_vs-->  radeon_set_context_reg

```


####  VGT_VTX_CNT_EN

VGT:VGT_VTX_CNT_EN 是一个可读写寄存器，占据32位，用于在复用模式下指定自动索引生成。第一个向量输出的y分量将具有自动索引值。自动索引值将通过向VGT发送的事件来重置为零。

该寄存器的GpuF0MMReg地址为 0x28ab8。

具体字段如下：

| 字段名称   | 位    | 默认值 | 描述                                                                                           |
|-------------|--------|---------|--------------------------------------------------------------------------------------------------|
| VTX_CNT_EN | 0      | none    | 如果启用自动索引生成，则将此字段设置为1。这是为了通过顶点着色器在y通道上导入的索引。这与DRAW_INDEX_AUTO不同。  |

这个字段的详细说明如下：

- VTX_CNT_EN字段（0）：用于启用或禁用自动索引生成。在复用模式下，当设置为1时，自动索引值将被生成，并存储在第一个向量输出的y分量中。自动索引值是一个在特定条件下自动生成的索引，它用于优化图形渲染过程中的顶点处理。

通过设置VTX_CNT_EN字段的值，可以控制是否启用自动索引生成。如果设置为1，自动索引将按照特定规则自动生成。如果设置为0，则禁用自动索引，此时应用程序需要通过其他方式提供顶点索引。

自动索引的使用可以提高绘制效率，尤其是在顶点复用的情况下。通过生成自动索引，可以减少对顶点索引的存储和传输，从而提高性能。


该寄存器使用流程同VGT_ES_PER_GS


### PA(Primitive Assembly ) Registers



#### PA_CL_CLIP_CNTL

PA:PA_CL_CLIP_CNTL 是一个可读写寄存器，占据32位，用于控制裁剪器的一些参数。

该寄存器的GpuF0MMReg地址为 0x28810。

具体字段如下：

| 字段名称                  | 位       | 默认值 | 描述                                                     |
|-----------------------------|-----------|---------|------------------------------------------------------------|
| UCP_ENA_0                  | 0         | none    | 启用用户裁剪平面0                                        |
| UCP_ENA_1                  | 1         | none    | 启用用户裁剪平面1                                        |
| UCP_ENA_2                  | 2         | none    | 启用用户裁剪平面2                                        |
| UCP_ENA_3                  | 3         | none    | 启用用户裁剪平面3                                        |
| UCP_ENA_4                  | 4         | none    | 启用用户裁剪平面4                                        |
| UCP_ENA_5                  | 5         | none    | 启用用户裁剪平面5                                        |
| PS_UCP_Y_SCALE_NEG   | 13       | none    | PS_UCP模式下y轴的缩放是否为负值              |
| PS_UCP_MODE              | 15:14 | none    | PS_UCP模式                                                 |
| CLIP_DISABLE             | 16       | none    | 禁用TCL中的裁剪代码生成和裁剪过程               |
| UCP_CULL_ONLY_ENA   | 17       | none    | 对UCP进行裁剪，但不进行裁剪                     |
| BOUNDARY_EDGE_FLAG_ENA | 18       | none    | 目前未使用：保留为占位符                          |
| DX_CLIP_SPACE_DEF     | 19       | none    | 剪辑空间的定义                                         |
| DIS_CLIP_ERR_DETECT | 20       | none    | 禁用对裁剪检测错误的原语进行裁剪      |
| VTX_KILL_OR                | 21       | none    | 如果从顶点着色器导出了顶点杀死标志，则指定顶点杀死标志的逻辑模式   |
| DX_RASTERIZATION_KILL | 22       | none    | DirectX光栅化杀死                                       |
| DX_LINEAR_ATTR_CLIP_ENA | 24     | none    | DirectX线性属性剪辑是否启用                 |
| VTE_VPORT_PROVOKE_DISABLE | 25 | none    | 是否禁用VTE口启发                                       |
| ZCLIP_NEAR_DISABLE   | 26       | none    | 是否禁用Z轴近裁剪                                     |
| ZCLIP_FAR_DISABLE     | 27       | none    | 是否禁用Z轴远裁剪                                     |

这些字段用于配置裁剪控制，以实现裁剪、剪裁和优化绘制效果。它们的具体作用取决于图形渲染管道的配置和特定的应用场景。通过对这些字段进行设置，可以调整渲染管线的行为和渲染效果。


```mermaid
graph LR

si_init_state_functions -->  |设置si_context atoms.s.clip_regs.emit| si_emit_clip_regs

si_emit_clip_regs  --> radeon_set_context_reg


```

####  PA_CL_ENHANCE

PA:PA_CL_ENHANCE 是一个可读写寄存器，占据32位，用于进行控制位的追加。

该寄存器的GpuF0MMReg地址为 0x8a14。

具体字段如下：

| 字段名称                     | 位       | 默认值 | 描述                                                     |
|--------------------------------|----------|---------|------------------------------------------------------------|
| CLIP_VTX_REORDER_ENA | 0         | 0x1       | 启用顶点顺序无关的裁剪                            |
| NUM_CLIP_SEQ               | 2:1       | 0x3       | 激活的裁剪序列数（+1）。应设置为3（4个序列）以获得最佳性能 |
| CLIPPED_PRIM_SEQ_STALL | 3         | none    | 如果NUM_CLIP_SEQ设置为0，则强制使用更快的裁剪路径 |
| VE_NAN_PROC_DISABLE    | 4         | none    | 禁用处理顶点着色器中的NaN值                      |

这些字段用于进行后期控制位的添加，用于优化裁剪和剪裁的性能。CLIP_VTX_REORDER_ENA字段用于启用一种顶点顺序无关的裁剪模式，NUM_CLIP_SEQ字段用于指定激活的裁剪序列数，CLIPPED_PRIM_SEQ_STALL字段用于强制使用更快的裁剪路径，VE_NAN_PROC_DISABLE字段用于禁用处理顶点着色器中的NaN值。

这些控制位的设置可以根据具体的应用场景和渲染需求进行调整，以获得最佳的性能和渲染效果。

该寄存器使用流程同VGT_ES_PER_GS

#### PA_CL_GB_HORZ_CLIP_ADJ

PA:PA_CL_GB_VERT_CLIP_ADJ 是一个可读写寄存器，占据32位，用于垂直边界带裁剪的调整。

该寄存器的 GpuF0MMReg 地址为 0x28be8。

具体字段如下：

| 字段名称       | 位      | 默认值 | 描述                                                         |
|-----------------|---------|---------|------------------------------------------------------------|
| DATA_REGISTER | 31:0    | none    | 32位浮点值。应设置为1.0以禁用垂直边界带裁剪。  |

这个寄存器用于调整垂直边界带的裁剪范围。通常，1.0表示没有边界带裁剪，而其他值表示进行了裁剪调整。垂直边界带裁剪可以用于在渲染中定义屏幕上的裁剪区域，超出该区域的图元将被裁剪掉。

具体数值的设置可以根据具体的渲染需求和场景进行调整，以获得所需的裁剪效果和渲染质量。


```mermaid
graph TD
 si_emit_guardband --> radeon_opt_set_context_reg4

```

####  PA_CL_NANINF_CNTL

PA:PA_CL_NANINF_CNTL 是一个可读写寄存器，占据32位，用于处理NaN（Not a Number）和无穷大值的控制。

该寄存器的 GpuF0MMReg 地址为 0x28820。

具体字段如下：

| 字段名称                  | 位     | 默认值 | 描述                                                   |
|------------------------|--------|---------|------------------------------------------------------|
| VTE_XY_INF_DISCARD      | 0      | none    | 若为1，顶点着色器计算的XY分量为无穷大时将被丢弃                |
| VTE_Z_INF_DISCARD       | 1      | none    | 若为1，顶点着色器计算的Z分量为无穷大时将被丢弃                 |
| VTE_W_INF_DISCARD       | 2      | none    | 若为1，顶点着色器计算的W分量为无穷大时将被丢弃                 |
| VTE_0XNANINF_IS_0       | 3      | none    | 若为1，顶点着色器计算结果中的0、无穷大和NaN将被替换为0      |
| VTE_XY_NAN_RETAIN       | 4      | none    | 若为1，顶点着色器计算的XY分量为NaN时将被保留                 |
| VTE_Z_NAN_RETAIN        | 5      | none    | 若为1，顶点着色器计算的Z分量为NaN时将被保留                  |
| VTE_W_NAN_RETAIN        | 6      | none    | 若为1，顶点着色器计算的W分量为NaN时将被保留                  |
| VTE_W_RECIP_NAN_IS_0    | 7      | none    | 若为1，当顶点着色器计算W的倒数为NaN时将被替换为0           |
| VS_XY_NAN_TO_INF        | 8      | none    | 若为1，顶点着色器计算的XY分量为NaN时将被替换为无穷大      |
| VS_XY_INF_RETAIN        | 9      | none    | 若为1，顶点着色器计算的XY分量为无穷大时将被保留              |
| VS_Z_NAN_TO_INF         | 10     | none    | 若为1，顶点着色器计算的Z分量为NaN时将被替换为无穷大       |
| VS_Z_INF_RETAIN         | 11     | none    | 若为1，顶点着色器计算的Z分量为无穷大时将被保留              |
| VS_W_NAN_TO_INF         | 12     | none    | 若为1，顶点着色器计算的W分量为NaN时将被替换为无穷大       |
| VS_W_INF_RETAIN         | 13     | none    | 若为1，顶点着色器计算的W分量为无穷大时将被保留              |
| VS_CLIP_DIST_INF_DISCARD| 14     | none    | 若为1，当顶点着色器计算的裁剪距离为无穷大时将被丢弃    |
| VTE_NO_OUTPUT_NEG_0     | 20     | none    | 若为1，顶点着色器计算结果中的负数将被替换为0              |



这些字段用于控制在图形管线中遇到NaN和无穷大值时的处理方式，可以通过设置不同的标志位来指定具体的处理行为。这些处理行为可能包括裁剪、替换或保留等操作，以确保在图形渲染过程中不会出现异常或非法的数值。根据应用场景和需求，可以根据需要来设置这些标志位，以实现所需的数值处理策略。

流程同同VGT_ES_PER_GS


#### PA_CL_VTE_CNTL

PA:PA_CL_VS_OUT_CNTL 是一个可读写寄存器，占据32位，用于控制顶点着色器输出的设置。

该寄存器的 GpuF0MMReg 地址为 0x2881c。

具体字段如下：

| 字段名称                        | 位     | 默认值 | 描述                                                             |
|-------------------------------|--------|---------|----------------------------------------------------------------|
| CLIP_DIST_ENA_0               | 0      | none    | 启用ClipDistance#用于用户定义的裁剪。需要设置VS_OUT_CCDIST#_ENA     |
| CLIP_DIST_ENA_1               | 1      | none    | 启用ClipDistance#用于用户定义的裁剪。需要设置VS_OUT_CCDIST#_ENA     |
| CLIP_DIST_ENA_2               | 2      | none    | 启用ClipDistance#用于用户定义的裁剪。需要设置VS_OUT_CCDIST#_ENA     |
| CLIP_DIST_ENA_3               | 3      | none    | 启用ClipDistance#用于用户定义的裁剪。需要设置VS_OUT_CCDIST#_ENA     |
| CLIP_DIST_ENA_4               | 4      | none    | 启用ClipDistance#用于用户定义的裁剪。需要设置VS_OUT_CCDIST#_ENA     |
| CLIP_DIST_ENA_5               | 5      | none    | 启用ClipDistance#用于用户定义的裁剪。需要设置VS_OUT_CCDIST#_ENA     |
| CLIP_DIST_ENA_6               | 6      | none    | 启用ClipDistance#用于用户定义的裁剪。需要设置VS_OUT_CCDIST#_ENA     |
| CLIP_DIST_ENA_7               | 7      | none    | 启用ClipDistance#用于用户定义的裁剪。需要设置VS_OUT_CCDIST#_ENA     |
| CULL_DIST_ENA_0               | 8      | none    | 启用CullDistance#用于用户定义的裁剪丢弃。需要设置VS_OUT_CCDIST#_ENA |
| CULL_DIST_ENA_1               | 9      | none    | 启用CullDistance#用于用户定义的裁剪丢弃。需要设置VS_OUT_CCDIST#_ENA |
| CULL_DIST_ENA_2               | 10     | none    | 启用CullDistance#用于用户定义的裁剪丢弃。需要设置VS_OUT_CCDIST#_ENA |
| CULL_DIST_ENA_3               | 11     | none    | 启用CullDistance#用于用户定义的裁剪丢弃。需要设置VS_OUT_CCDIST#_ENA |
| CULL_DIST_ENA_4               | 12     | none    | 启用CullDistance#用于用户定义的裁剪丢弃。需要设置VS_OUT_CCDIST#_ENA |
| CULL_DIST_ENA_5               | 13     | none    | 启用CullDistance#用于用户定义的裁剪丢弃。需要设置VS_OUT_CCDIST#_ENA |
| CULL_DIST_ENA_6               | 14     | none    | 启用CullDistance#用于用户定义的裁剪丢弃。需要设置VS_OUT_CCDIST#_ENA |
| CULL_DIST_ENA_7               | 15     | none    | 启用CullDistance#用于用户定义的裁剪丢弃。需要设置VS_OUT_CCDIST#_ENA |
| USE_VTX_POINT_SIZE            | 16     | none    | 使用来自VS的PointSize输出（在VS_OUT_MISC_VEC的x通道）                |
| USE_VTX_EDGE_FLAG             | 17     | none    | 使用来自VS的EdgeFlag输出（在VS_OUT_MISC_VEC的y通道）                |
| USE_VTX_RENDER_TARGET_INDX    | 18     | none    | 使用来自VS的RenderTargetArrayIndx输出（在VS_OUT_MISC_VEC的z通道）  |
| USE_VTX_VIEWPORT_INDX         | 19     | none    | 使用来自VS的ViewportArrayIndx输出（在VS_OUT_MISC_VEC的w通道）       |
| USE_VTX_KILL_FLAG             | 20     | none    | 使用来自VS的KillFlag输出（在VS_OUT_MISC_VEC的z通道）                |
| VS_OUT_MISC_VEC_ENA           | 21     | none    | 输出VS的杂项向量（从VS（SX）到PA（原

语组装器））。如果要使用任何字段，应设置 |
| VS_OUT_CCDIST0_VEC_ENA        | 22     | none    | 输出VS的ccdist0向量（从VS（SX）到PA（原语组装器））。如果要使用任何字段，应设置 |
| VS_OUT_CCDIST1_VEC_ENA        | 23     | none    | 输出VS的ccdist1向量（从VS（SX）到PA（原语组装器））。如果要使用任何字段，应设置 |
| VS_OUT_MISC_SIDE_BUS_ENA      | 24     | none    |                                                                     |
| USE_VTX_GS_CUT_FLAG           | 25     | none    |                                                                     |

这些字段用于控制顶点着色器的输出设置，可以启用/禁用剪裁和舍弃、设置顶点大小、边缘标志等功能，以及将顶点着色器输出传递到图元组装阶段。可以根据图形渲染需求来设置这些标志位，以实现所需的顶点着色器输出设置。



流程同PA_CL_CLIP_CNTL

####  PA_SC_AA_MASK_X0Y0_X1Y0

PA_SC_AA_MASK_X0Y0_X1Y0是一个可读写的寄存器，它占据32位，用于多样本反锯齿掩码。

该寄存器的GpuF0MMReg地址为0x28c38。

具体字段如下：

| 字段名称         | 位     | 默认值 | 描述                                                                                      |
|----------------|--------|--------|-----------------------------------------------------------------------------------------|
| AA_MASK_X0Y0   | 15:0   | none   | 16位掩码应用于像素X0、Y0(左上角)。LSB是Sample0，MSB是Sample15。                         |
| AA_MASK_X1Y0   | 31:16  | none   | 16位掩码应用于像素X1、Y0(右上角)。LSB是Sample0，MSB是Sample15。                         |

这些字段用于设置多样本反锯齿掩码，对像素X0、Y0（左上角）和X1、Y0（右上角）的样本进行掩码，每个样本对应一个比特位。掩码决定了像素的多样本覆盖情况，如果全部为1，则开启全覆盖的优化。根据所需的样本覆盖情况，可以设置这些掩码来实现反锯齿效果。

#### PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0

PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0是一个可读写的寄存器，用于设置多样本像素X0、Y0（左上角）的程序化采样位置。

该寄存器的GpuF0MMReg地址为0x28bf8。

具体字段如下：

| 字段名称   | 位      | 默认值 | 描述                                        |
|------------|---------|--------|-----------------------------------------------|
| S0_X       | 3:0     | none   | 从像素中心的偏移，4位有符号数。范围-8/16至7/16。 |
| S0_Y       | 7:4     | none   | 从像素中心的偏移，4位有符号数。范围-8/16至7/16。 |
| S1_X       | 11:8    | none   | 从像素中心的偏移，4位有符号数。范围-8/16至7/16。 |
| S1_Y       | 15:12   | none   | 从像素中心的偏移，4位有符号数。范围-8/16至7/16。 |
| S2_X       | 19:16   | none   | 从像素中心的偏移，4位有符号数。范围-8/16至7/16。 |
| S2_Y       | 23:20   | none   | 从像素中心的偏移，4位有符号数。范围-8/16至7/16。 |
| S3_X       | 27:24   | none   | 从像素中心的偏移，4位有符号数。范围-8/16至7/16。 |
| S3_Y       | 31:28   | none   | 从像素中心的偏移，4位有符号数。范围-8/16至7/16。 |

这些字段用于设置多样本像素X0、Y0（左上角）的采样位置。每个样本对应一个4位的有符号偏移，表示相对于像素中心的偏移量。范围是-8/16至7/16。通过设置这些偏移量，可以调整多样本像素采样的位置，从而实现更精确的抗锯齿效果。




#### PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_0

#### PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_0


### General Shader Registers 



### Shader Buffer Resource Descriptor

#### SQ_BUF_RSRC_WORD0

SQ_BUF_RSRC_WORD0是一个可读写的寄存器，用于设置缓冲区资源的一些参数。该寄存器的GpuF0MMReg地址为0x8f00。

具体字段如下：

| 字段名称        | 位     | 默认值 | 描述                                 |
|----------------|--------|--------|----------------------------------------|
| BASE_ADDRESS   | 31:0   | 0x0    | 缓冲区的基地址，位31-0表示以字节为单位的基地址。 |

这个字段用于设置缓冲区的基地址，它表示缓冲区在内存中的起始位置。在32位的字段中，可以指定2^32个不同的内存位置作为缓冲区的基地址。这个寄存器是一个非常重要的设置，因为它决定了缓冲区在内存中的位置，从而影响了程序的正确执行。要正确使用缓冲区，必须将合适的基地址加载到该寄存器中。



### SQ_BUF_RSRC_WORD1

SQ_BUF_RSRC_WORD1是一个可读写的寄存器，用于设置缓冲区资源的一些参数。该寄存器的GpuF0MMReg地址为0x8f04。

具体字段如下：

| 字段名称          | 位      | 默认值 | 描述                                                        |
|------------------|---------|--------|---------------------------------------------------------------|
| BASE_ADDRESS_HI  | 15:0    | 0x0    | 缓冲区的基地址的高16位，位47-32表示以字节为单位的高位基地址。 |
| STRIDE           | 29:16   | 0x0    | 缓冲区中相邻元素之间的跨度，以字节为单位，取值范围为[0..2048]。  |
| CACHE_SWIZZLE    | 30      | 0x0    | 缓存重排标志。可以选择对TC L1缓存进行bank级别的重排。              |
| SWIZZLE_ENABLE   | 31      | 0x0    | 缓存重排使能标志。根据跨度（stride）、索引跨度（index_stride）和元素大小（element_size），对缓存进行阵列结构的重排，否则为线性。   |

这些字段用于配置缓冲区资源的各个参数，包括高16位的基地址、跨度（Stride）、缓存重排（Cache Swizzle）和缓存重排使能（Swizzle Enable）。这些参数的正确设置对于缓冲区的访问和数据处理非常重要。例如，跨度决定了在缓冲区中相邻元素之间的字节偏移量，而缓存重排可以优化缓存的访问性能。


###  SQ_BUF_RSRC_WORD2


SQ_BUF_RSRC_WORD2是一个可读写的寄存器，用于设置缓冲区资源的一些参数。该寄存器的GpuF0MMReg地址为0x8f08。

具体字段如下：

| 字段名称       | 位     | 默认值 | 描述                                  |
|---------------|--------|--------|---------------------------------------|
| NUM_RECORDS   | 31:0   | 0x0    | 缓冲区中的记录数量。每个记录的大小为STRIDE字节。 |

这个寄存器用于设置缓冲区中的记录数量。每个记录的大小由STRIDE字段指定，因此通过设置NUM_RECORDS和STRIDE字段，可以确定缓冲区的总大小。

请注意，记录的大小和数量是关键参数，它们决定了缓冲区可以存储的数据量。确保将这些参数设置为正确的值，以满足应用程序的需求。


### SQ_BUF_RSRC_WORD3




## Shader Image Resource Descriptor 

TODO

## Shader Image Resource Sampler Descriptor
TODO

|
# Shader Program Registers


在GCN 硬件阶段可分为LS,HS, ES,GS,VS,PS
如下图

下面寄存器名中**HI**代表高位[63:32], **LO**代表低位[31:0]，所有下面的寄存器都是32 位。

### Shader Program 基址相关

在 AMD 的显卡中，着色器程序被分为两个部分：`SPI_SHADER_PGM_LO` 和 `SPI_SHADER_PGM_HI`。

具体来说：

- `SPI_SHADER_PGM_LO_VS` 是用于顶点着色器程序的低位地址部分。
- `SPI_SHADER_PGM_HI_VS` 是用于顶点着色器程序的高位地址部分。

这两个寄存器结合在一起，形成了顶点着色器程序的完整地址，指向存储在显存中的顶点着色器代码。

在显卡的驱动程序或硬件中，通过将顶点着色器程序加载到这些寄存器中，显卡就可以在渲染过程中使用顶点着色器来处理传入的顶点数据，并执行必要的变换、光照等计算，以生成最终的图形显示。

这些寄存器通常是由驱动程序在渲染任务开始之前设置好的，以确保着色器程序正确加载并准备执行。它们是图形渲染流水线中非常重要的部分，有助于实现图形渲染的高性能和效率。

* SPI_SHADER_PGM_HI_ES
* SPI_SHADER_PGM_HI_GS
* SPI_SHADER_PGM_HI_HS
* SPI_SHADER_PGM_HI_LS
* SPI_SHADER_PGM_HI_PS
* SPI_SHADER_PGM_HI_VS
* SPI_SHADER_PGM_LO_ES
* SPI_SHADER_PGM_LO_GS
* SPI_SHADER_PGM_LO_HS
* SPI_SHADER_PGM_LO_LS
* SPI_SHADER_PGM_LO_PS
* SPI_SHADER_PGM_LO_VS


### Shader Program设置 相关


下面这些寄存器主要是shader program的配置。

* SPI_SHADER_PGM_RSRC1_ES
* SPI_SHADER_PGM_RSRC1_GS
* SPI_SHADER_PGM_RSRC1_HS
* SPI_SHADER_PGM_RSRC1_LS
* SPI_SHADER_PGM_RSRC1_PS
* SPI_SHADER_PGM_RSRC1_VS
* 
* SPI_SHADER_PGM_RSRC2_ES
* SPI_SHADER_PGM_RSRC2_GS
* SPI_SHADER_PGM_RSRC2_HS
* SPI_SHADER_PGM_RSRC2_LS
* SPI_SHADER_PGM_RSRC2_PS
* SPI_SHADER_PGM_RSRC2_VS


以 SPI_SHADER_PGM_RSRC1_VS (0xb128) 为例， 下表给出了各个字段的含义 :

这些字段描述了SPI_SHADER_PGM_RSRC1_VS寄存器中的不同位段的作用和含义。这个寄存器用于配置Vertex Shader（VS）程序的设置，比如VGPRs数量、SGPRs数量、浮点模式、优先级等。这些配置设置将影响VS程序的运行行为和性能。

| 字段名称       | 位数   | 默认值 | 描述                                               |
| -------------- | ------ | ------ | -------------------------------------------------- |
| VGPRS          | 5:0    | 0x0    | VGPRs的数量，以4为粒度，范围为0-63，分配方式为4,8,12 ... 256 |
| SGPRS          | 9:6    | 0x0    | SGPRs的数量，以8为粒度，范围为0-15，分配方式为8,16,24 ... 128 |
| PRIORITY       | 11:10  | 0x0    | 驱动spi_sq newWave指令中的spi_priority |
| FLOAT_MODE     | 19:12  | 0x0    | 驱动spi_sq newWave指令中的float_mode |
| PRIV           | 20     | 0x0    | 驱动spi_sq newWave指令中的priv |
| DX10_CLAMP     | 21     | 0x0    | 驱动spi_sq newWave指令中的dx10_clamp |
| DEBUG_MODE     | 22     | 0x0    | 驱动spi_sq newWave指令中的debug |
| IEEE_MODE      | 23     | 0x0    | 驱动spi_sq newWave指令中的ieee |
| VGPR_COMP_CNT  | 25:24  | 0x0    | 告诉SPI要加载多少VGPR组件 |
| CU_GROUP_ENABLE| 26     | 0x0    | 当设置为1时，VS优先发送一个wave给CU中的每个SIMD；当设置为0时，VS只发送一个wave给一个CU中的一个SIMD |


* SPI_SHADER_PGM_RSRC2_VS 


| **字段名称**  | **位数** | **默认值** | **描述**                                              |
|-------------|--------|---------|---------------------------------------------------|
| SCRATCH_EN  | 0      | 0x0     | 该波（Wave）是否使用寄存器临时空间进行寄存器溢出。       |
| USER_SGPR   | 5-1    | 0x0     | SPI 应初始化的 USER_DATA 寄存器数量。范围：0-16。      |
| TRAP_PRESENT| 6      | 0x0     | 是否启用陷阱处理。设置该位会使 SPI 分配额外的 16 个 SGPR，并将 TBA/TMA 值写入 SGPR 中。|
| OC_LDS_EN   | 7      | 0x0     | 是否启用将与 offchip 相关的信息加载到 SGPR 中。        |
| SO_BASE0_EN | 8      | 0x0     | 是否启用将 Stream Out Base 0 加载到 SGPR 中。           |
| SO_BASE1_EN | 9      | 0x0     | 是否启用将 Stream Out Base 1 加载到 SGPR 中。           |
| SO_BASE2_EN | 10     | 0x0     | 是否启用将 Stream Out Base 2 加载到 SGPR 中。           |
| SO_BASE3_EN | 11     | 0x0     | 是否启用将 Stream Out Base 3 加载到 SGPR 中。           |
| SO_EN       | 12     | 0x0     | 是否启用将 Stream Out 缓冲区配置加载到 SGPR 中。        |
| EXCP_EN     | 19-13  | 0x0     | 在 spi_sq newWave 命令中设置 excp 位的驱动。                |

### TBA 相关

**陷阱处理程序基地址TBA (Trap Base Address)**
TBA 是一个 64 位的寄存器，它用于保存指向当前陷阱（trap）处理程序的指针。在计算机图形学中，陷阱是一种异常或中断，用于处理特定的条件或情况。着色器程序可以使用 TBA 寄存器来存储当前陷阱处理程序的地址，以便在需要时跳转到相应的处理程序。


* SPI_SHADER_TBA_HI_ES
* SPI_SHADER_TBA_HI_GS
* SPI_SHADER_TBA_HI_HS
* SPI_SHADER_TBA_HI_LS
* SPI_SHADER_TBA_HI_PS
* SPI_SHADER_TBA_HI_VS

* SPI_SHADER_TBA_LO_ES
* SPI_SHADER_TBA_LO_GS
* SPI_SHADER_TBA_LO_HS
* SPI_SHADER_TBA_LO_LS
* SPI_SHADER_TBA_LO_PS
* SPI_SHADER_TBA_LO_VS


### TMA 相关

**指向陷阱处理程序在内存中使用的数据的指针TMA (Trap Memory Address)**:
TMA 是另一个 64 位的寄存器，它用于在着色器操作中作为临时寄存器。着色器是在图形渲染管线中运行的小型程序，用于控制图形元素的渲染过程。TMA 寄存器用于暂存某些中间结果或计算过程中的数据，以便在需要时进行临时存储和处理。

* SPI_SHADER_TMA_HI_ES
* SPI_SHADER_TMA_HI_GS
* SPI_SHADER_TMA_HI_HS
* SPI_SHADER_TMA_HI_LS
* SPI_SHADER_TMA_HI_PS
* SPI_SHADER_TMA_HI_VS

* SPI_SHADER_TMA_LO_ES
* SPI_SHADER_TMA_LO_GS
* SPI_SHADER_TMA_LO_HS
* SPI_SHADER_TMA_LO_LS
* SPI_SHADER_TMA_LO_PS
* SPI_SHADER_TMA_LO_VS

### USER_DATA

USER_DATA相关寄存器是SGPR Initialization用到的寄存器。

SGPR（Scalar General Purpose Register）初始化是指在图形着色器中加载不会在每个线程中变化的着色器输入。这个过程也会根据着色器类型而有所不同。

在着色器中，SGPR可以用来存储通用目的的数据。在初始化过程中，SGPR主要用来加载一些与着色器相关的常量或数据，这些数据在整个着色器执行过程中保持不变。

在AMD的图形处理架构中，有一种称为 "USER_DATA" 的机制，用于将数据加载到SGPR中。通过使用 "SPI_SHADER_USER_DATA_xx_0-15" 硬件寄存器，驱动程序可以通过PM4 SET命令写入数据到这些SGPR中。用户数据用于向着色器传递一些关键信息，其中最重要的是视频内存中资源常量的内存地址。


* SPI_SHADER_USER_DATA_ES_[0-15]
* SPI_SHADER_USER_DATA_GS_[0-15]
* SPI_SHADER_USER_DATA_HS_[0-15]
* SPI_SHADER_USER_DATA_LS_[0-15]
* SPI_SHADER_USER_DATA_PS_[0-15]
* SPI_SHADER_USER_DATA_VS_[0-15]


TMA和TBA都用在Scalar ALU Operations 中。



##  SPI Registers


### SPI:SPI_ARB_CYCLES_0 · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x90f4

**DESCRIPTION:** 这个寄存器用于控制时间分片（Timeslice）的时长，其粒度为 16 个时钟周期。它允许在1GHz时钟频率下设置时间分片的持续时间，范围从16纳秒到1毫秒。应以广播方式写入，并分别存储在每个SE中。

以下是字段的定义：

| 字段名称       | 位范围  | 默认值 | 描述                                   |
|---------------|---------|--------|----------------------------------------|
| TS0_DURATION  | 15:0    | 0x0    | Timeslice 0 的持续时间（时钟周期）。   |
| TS1_DURATION  | 31:16   | 0x0    | Timeslice 1 的持续时间（时钟周期）。   |

这些字段允许配置两个不同时间分片的持续时间，以便在GPU操作中进行精确的时间控制。

### SPI:SPI_ARB_CYCLES_1 · [R/W] · 16 bits · Access: 16 · GpuF0MMReg:0x90f8

**DESCRIPTION:** 该寄存器用于控制时间分片（Timeslice）的时长，其粒度为 16 个时钟周期。它允许在1GHz时钟频率下设置时间分片的持续时间，范围从16纳秒到1毫秒。应以广播方式写入，并分别存储在每个SE（Shader Engine）中。

以下是字段的定义：

| 字段名称       | 位范围  | 默认值 | 描述                                   |
|---------------|---------|--------|----------------------------------------|
| TS2_DURATION  | 15:0    | 0x0    | Timeslice 2 的持续时间（时钟周期）。   |

这个字段允许配置第三个不同时间分片的持续时间，以便在GPU操作中进行更精确的时间控制。


### SPI:SPI_ARB_PRIORITY · [R/W] · 16 bits · Access: 16 · GpuF0MMReg:0x90f0

**DESCRIPTION:** 该寄存器用于配置在三个时间分片（Timeslice）持续时间内，三个环（Ring）的优先级级别。应以广播方式写入，并分别存储在每个SE（Shader Engine）中。

以下是字段的定义：

| 字段名称          | 位范围   | 默认值 | 描述                                   |
|------------------|----------|--------|----------------------------------------|
| RING_ORDER_TS0   | 2:0      | 0x0    | 在 Timeslice 0 期间的环优先级顺序设置。 |
| RING_ORDER_TS1   | 5:3      | 0x0    | 在 Timeslice 1 期间的环优先级顺序设置。 |
| RING_ORDER_TS2   | 8:6      | 0x0    | 在 Timeslice 2 期间的环优先级顺序设置。 |

这些字段允许配置每个时间分片内三个环的优先级顺序。优先级的不同顺序可能会影响GPU操作的性能和效率，具体取决于应用程序的要求和硬件配置。


### SPI:SPI_BARYC_CNTL · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x286e0

**DESCRIPTION:** 此寄存器用于配置BCI（Barycentric Interpolation）中的重心插值控制。BCI是一种用于图形渲染中的插值技术，用于计算像素内的属性值。以下是字段的定义和说明：

| 字段名称                | 位范围     | 默认值 | 描述                                                 |
|------------------------|------------|--------|------------------------------------------------------|
| PERSP_CENTER_CNTL       | 0          | 0x0    | 透视中心控制。可能的值：                           |
|                        |            |        | 00 - 在中心处启用                                    |
|                        |            |        | 01 - 在质心处启用                                    |
| PERSP_CENTROID_CNTL     | 4          | 0x0    | 透视质心控制。可能的值：                           |
|                        |            |        | 00 - 在质心处启用                                    |
|                        |            |        | 01 - 在中心处启用                                    |
| LINEAR_CENTER_CNTL      | 8          | 0x0    | 线性中心控制。可能的值：                            |
|                        |            |        | 00 - 在中心处启用                                    |
|                        |            |        | 01 - 在质心处启用                                    |
| LINEAR_CENTROID_CNTL    | 12         | 0x0    | 线性质心控制。可能的值：                            |
|                        |            |        | 00 - 在质心处启用                                    |
|                        |            |        | 01 - 在中心处启用                                    |
| POS_FLOAT_LOCATION      | 17:16      | 0x0    | 位置浮点数位置控制。可能的值：                      |
|                        |            |        | 00 - 在像素中心计算每像素浮点位置                   |
|                        |            |        | 01 - 在像素质心计算每像素浮点位置                   |
|                        |            |        | 02 - 在迭代样本编号处计算每像素浮点位置               |
|                        |            |        | 03 - 未定义                                          |
| POS_FLOAT_ULC           | 20         | 0x0    | 强制浮点位置到像素的左上角（X.0，Y.0）。              |
| FRONT_FACE_ALL_BITS     | 24         | 0x0    | 前向面所有位控制。可能的值：                        |
|                        |            |        | 00 - 符号位表示isFF（dx9，-1.0f == 背面，+1.0f == 正面） |
|                        |            |        | 01 - 用isFF替换整个32位值（WGF，1 == 正面，0 == 背面）  |

这些字段允许配置BCI中的不同插值参数，以满足图形渲染的需求。根据这些参数的设置，插值的方式和位置可能会发生变化，从而影响像素的属性计算。

### SPI:SPI_CONFIG_CNTL_1 · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x913c

### SPI:SPI_DYN_GPR_LOCK_EN · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x90dc
### SPI:SPI_INTERP_CONTROL_0 · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x286d4
### SPI:SPI_PS_INPUT_ADDR · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x286d0
### SPI:SPI_PS_INPUT_CNTL_[0-31] · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x28644-0x286c0
### SPI:SPI_PS_INPUT_ENA · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x286cc
### SPI:SPI_PS_IN_CONTROL · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x286d8
### SPI:SPI_PS_MAX_WAVE_ID · [R/W] · 16 bits · Access: 16 · GpuF0MMReg:0x90ec
### SPI:SPI_RESOURCE_RESERVE_CU_AB_[0-7] · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x936c0x9388
### SPI:SPI_SHADER_COL_FORMAT · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x28714
### SPI:SPI_SHADER_POS_FORMAT · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x2870c
### SPI:SPI_SHADER_Z_FORMAT · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x28710
### SPI:SPI_STATIC_THREAD_MGMT_1 · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x90e0
### SPI:SPI_STATIC_THREAD_MGMT_2 · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x90e4
### SPI:SPI_STATIC_THREAD_MGMT_3 · [R/W] · 16 bits · Access: 16 · GpuF0MMReg:0x90e8
### SPI:SPI_TMPRING_SIZE · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x286e8
## SPI:SPI_VS_OUT_CONFIG · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x286c4
### SPI:SPI_WAVE_MGMT_1 · [R/W] · 32 bits · Access: 32 · GpuF0MMReg:0x28704
## SPI:SPI_WAVE_MGMT_2 · [R/W] · 16 bits · Access: 16 · GpuF0MMReg:0x28708


# 10. Compute Registers


### COMP:COMPUTE_DIM_X · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb804 


### COMP:COMPUTE_DIM_Y · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb808


### COMP:COMPUTE_DIM_Z · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb80c  


### COMP:COMPUTE_DISPATCH_INITIATOR · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb800 

### COMP:COMPUTE_MAX_WAVE_ID · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb82c 

### COMP:COMPUTE_NUM_THREAD_X · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb81c  

### COMP:COMPUTE_NUM_THREAD_Y · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb820 

### COMP:COMPUTE_NUM_THREAD_Z · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb824 


### COMP:COMPUTE_PGM_HI · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb834 


### COMP:COMPUTE_PGM_LO · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb830 


### COMP:COMPUTE_PGM_RSRC1 · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb848 


### COMP:COMPUTE_PGM_RSRC2 · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb84c 


### COMP:COMPUTE_RESOURCE_LIMITS · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb854  

### COMP:COMPUTE_START_X · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb810 

### COMP:COMPUTE_START_Y · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb814

### COMP:COMPUTE_START_Z · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb818 

### COMP:COMPUTE_STATIC_THREAD_MGMT_SE0 · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb858

### COMP:COMPUTE_STATIC_THREAD_MGMT_SE1 · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb85c


### COMP:COMPUTE_TBA_HI · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb83c 


### COMP:COMPUTE_TMA_HI · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb844 

### COMP:COMPUTE_TMA_LO · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb840 


### COMP:COMPUTE_TMPRING_SIZE · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb860 


### COMP:COMPUTE_USER_DATA_[0-15] · [W] · 32 bits · Access: 32 · GpuF0MMReg:0xb900-0xb93c 



# 11. Tiling Registers

### GB_TILE_MODE[0-31]

### 

# 12. Surface Synchronization Registers

### CP_COHER_BASE

### CP_COHER_CNTL


### CP_COHER_SIZE



# 13. Texture Pipe Registers


### TA_BC_BASE_ADDR

### TA_CS_BC_BASE_ADDR



# 14. Depth Buffer Registers
###  DB_ALPHA_TO_MASK

DB:DB_ALPHA_TO_MASK 寄存器是一个32位寄存器，用于控制与 alpha 值相关的蒙版操作。该寄存器的 GPU 寄存器地址为 0x28b70。

**字段解析**

| Field Name            | 位范围       | 默认值 | 描述                                                                                   |
|-----------------------|--------------|--------|------------------------------------------------------------------------------------------|
| ALPHA_TO_MASK_ENABLE  | 0            | none   | 如果启用，样本蒙版将与从 alpha 值产生的蒙版进行逻辑与操作。可以通过设置 DB_SHADER_CONTROL.ALPHA_TO_MASK_DISABLE 来覆盖此字段。 |
| ALPHA_TO_MASK_OFFSET0 | 9:8          | none   | 如果启用 alpha 到蒙版，这是每个四方块中像素 (0,0) 的抖动阈值。设置为 2 表示非抖动，或者设置为 0-3 中的唯一值表示抖动。        |
| ALPHA_TO_MASK_OFFSET1 | 11:10        | none   | 如果启用 alpha 到蒙版，这是每个四方块中像素 (0,1) 的抖动阈值。设置为 2 表示非抖动，或者设置为 0-3 中的唯一值表示抖动。      |
| ALPHA_TO_MASK_OFFSET2 | 13:12        | none   | 如果启用 alpha 到蒙版，这是每个四方块中像素 (1,0) 的抖动阈值。设置为 2 表示非抖动，或者设置为 0-3 中的唯一值表示抖动。      |
| ALPHA_TO_MASK_OFFSET3 | 15:14        | none   | 如果启用 alpha 到蒙版，这是每个四方块中像素 (1,1) 的抖动阈值。设置为 2 表示非抖动，或者设置为 0-3 中的唯一值表示抖动。      |
| OFFSET_ROUND          | 16           | none   | 抖动阈值舍入。设置为 0 表示无抖动外观，设置为 1 表示有抖动外观。                              |

这个寄存器用于配置与 alpha 值相关的蒙版操作，以影响像素的采样过程。


### DB_COUNT_CONTROL

DB:DB_COUNT_CONTROL 寄存器是一个32位寄存器，用于配置深度缓冲区计数相关的控制参数。该寄存器的 GPU 寄存器地址为 0x28004。

**字段解析**

| Field Name            | 位范围       | 默认值 | 描述                                                                                                     |
|-----------------------|--------------|--------|------------------------------------------------------------------------------------------------------------|
| ZPASS_INCREMENT_DISABLE| 0            | none   | 禁止增加此上下文的 ZPass 计数。                                                                           |
| PERFECT_ZPASS_COUNTS  | 1            | none   | 强制 ZPass 计数准确，通过关闭跳过光栅化可能导致不正确的 ZPass 计数的无操作剔除优化（部分覆盖的瓦片）。 |
| SAMPLE_RATE           | 6:4          | 0x0    | 设置每像素计数的样本数。无论实际每像素有多少样本，面积计数都是准确的。                                     |

DB:DB_COUNT_CONTROL 寄存器用于控制深度缓冲区计数的行为，包括是否启用增加 ZPass 计数、是否强制准确的 ZPass 计数，以及每像素计数的样本数。这些配置可用于优化图形渲染流程。



### DB_DEPTH_BOUNDS_MAX

DB:DB_DEPTH_BOUNDS_MAX 寄存器是一个32位寄存器，用于配置深度范围测试的最大深度值。该寄存器的 GPU 寄存器地址为 0x28024。

**字段解析**

| Field Name    | 位范围 | 默认值 | 描述                                      |
|---------------|--------|--------|-------------------------------------------|
| MAX           | 31:0   | none   | 深度范围测试的最大深度值。               |

DB:DB_DEPTH_BOUNDS_MAX 寄存器用于设置深度范围测试的最大深度值。深度范围测试是一种用于确定像素是否在深度范围内的测试，该测试可以用于剔除不需要的像素，以提高图形渲染的性能。这个寄存器的 MAX 字段指定了允许通过深度范围测试的最大深度值。


### DB_DEPTH_BOUNDS_MIN

DB:DB_DEPTH_BOUNDS_MIN 寄存器是一个32位寄存器，用于配置深度范围测试的最小深度值。该寄存器的 GPU 寄存器地址为 0x28020。

**字段解析**

| Field Name    | 位范围 | 默认值 | 描述                                      |
|---------------|--------|--------|-------------------------------------------|
| MIN           | 31:0   | none   | 深度范围测试的最小深度值。               |

DB:DB_DEPTH_BOUNDS_MIN 寄存器用于设置深度范围测试的最小深度值。深度范围测试是一种用于确定像素是否在深度范围内的测试，该测试可以用于剔除不需要的像素，以提高图形渲染的性能。这个寄存器的 MIN 字段指定了允许通过深度范围测试的最小深度值。



### DB_DEPTH_CLEAR

DB:DB_DEPTH_CLEAR 寄存器是一个32位寄存器，用于配置当 ZMASK 等于0时表示瓦片已经被清除为背景深度的深度值。该寄存器的 GPU 寄存器地址为 0x2802c。

**字段解析**

| Field Name    | 位范围 | 默认值 | 描述                                      |
|---------------|--------|--------|-------------------------------------------|
| DEPTH_CLEAR   | 31:0   | none   | 当 ZMASK==0 时表示瓦片已经被清除为背景深度的深度值。这个寄存器保存一个32位浮点值。这个值必须在0.0到1.0的范围内。               |

DB:DB_DEPTH_CLEAR 寄存器用于配置深度缓冲区中的清除深度值。当 ZMASK 等于0时，表示瓦片已经被清除到背景深度值。这个寄存器的 DEPTH_CLEAR 字段指定了清除深度值，它必须是一个32位浮点值，并且必须在0.0到1.0的范围内。这个值通常用于初始化或重置深度缓冲区。

###  DB_DEPTH_CONTROL

DB:DB_DEPTH_CONTROL 寄存器是一个32位寄存器，用于控制深度和模板测试的行为。该寄存器的 GPU 寄存器地址为 0x28800。

**字段解析**

| Field Name                      | 位范围 | 默认值 | 描述                                                                                     |
|---------------------------------|--------|--------|--------------------------------------------------------------------------------------------|
| STENCIL_ENABLE                  | 0      | none   | 启用模板测试。如果禁用，所有像素都会通过模板测试。如果没有模板缓冲区，将视为禁用。           |
| Z_ENABLE                        | 1      | none   | 启用深度测试。如果禁用，所有像素都会通过深度测试。如果没有深度缓冲区，将视为禁用。         |
| Z_WRITE_ENABLE                  | 2      | none   | 如果深度测试通过，启用对深度缓冲区的写入。                                            |
| DEPTH_BOUNDS_ENABLE             | 3      | none   | 启用深度范围测试。如果禁用，所有样本都会通过深度范围测试。如果没有深度缓冲区，将视为禁用。 |
| ZFUNC                           | 6:4    | none   | 指定在片段中每个样本与相应样本点的目标深度进行比较的函数。                                    |
| BACKFACE_ENABLE                 | 7      | none   | 如果为 false，强制将所有四方块视为正面四方块进行模板测试。                                |
| STENCILFUNC                     | 10:8   | none   | 指定与正面四方块的目标模板值进行比较的函数。模板测试通过如果 ref OP dest 为 true。               |
| STENCILFUNC_BF                  | 22:20  | none   | 指定与背面四方块的目标模板值进行比较的函数。模板测试通过如果 ref OP dest 为 true。             |
| ENABLE_COLOR_WRITES_ON_DEPTH_FAIL | 30    | none   | 如果 z 或模板测试失败，则启用对颜色缓冲区的写入。                                          |
| DISABLE_COLOR_WRITES_ON_DEPTH_PASS | 31    | none   | 如果 z 和模板测试通过，则禁用对颜色缓冲区的写入。                                        |

DB:DB_DEPTH_CONTROL 寄存器用于配置深度和模板测试的各种行为，包括启用/禁用测试、深度写入、颜色写入等操作，以控制图形渲染流程中的深度和模板处理。这些配置可用于调整渲染管线以实现不同的图形效果。

### DB_DEPTH_INFO

DB:DB_DEPTH_INFO 寄存器是一个32位寄存器，用于配置深度缓冲区信息，特别是与数据存储有关的配置。该寄存器的 GPU 寄存器地址为 0x2803c。

**字段解析**

| Field Name          | 位范围 | 默认值 | 描述                                                                                                         |
|---------------------|--------|--------|------------------------------------------------------------------------------------------------------------|
| ADDR5_SWIZZLE_MASK  | 3:0    | none   | 对于32B瓦片，指示数据应存储在64B字的上半部分还是下半部分。如果ADDR5_SWIZZLE_MASK与{TILE_Y[1:0], TILE_X[1:0]}的异或减少设置，使用上半部分，否则使用下半部分。最可能的最佳值是0x1。|

DB:DB_DEPTH_INFO 寄存器用于配置深度缓冲区数据的存储方式。特别地，ADDR5_SWIZZLE_MASK 字段用于指示在32B瓦片中数据应存储在64B字的哪一半，根据一些特定条件进行选择。这个寄存器的配置可以影响深度缓冲区数据的组织和存储方式。

### DB_DEPTH_SIZE

DB:DB_DEPTH_SIZE 寄存器是一个32位寄存器，用于配置深度缓冲区的大小。该寄存器的 GPU 寄存器地址为 0x28058。

**字段解析**

| Field Name       | 位范围   | 默认值 | 描述                                                         |
|------------------|----------|--------|------------------------------------------------------------|
| PITCH_TILE_MAX   | 10:0     | none   | 以8x8像素瓦片为单位的宽度。实际宽度为 (Pitch/8 - 1)。 |
| HEIGHT_TILE_MAX  | 21:11    | none   | 以8x8像素瓦片为单位的深度缓冲区高度。实际高度为 (height/8 - 1)。 |

DB:DB_DEPTH_SIZE 寄存器用于配置深度缓冲区的大小，特别是指定深度缓冲区的宽度和高度。PITCH_TILE_MAX 字段表示深度缓冲区的宽度（以8x8像素瓦片为单位），而 HEIGHT_TILE_MAX 字段表示深度缓冲区的高度（同样以8x8像素瓦片为单位）。这些字段的配置决定了深度缓冲区的存储尺寸。


### DB_DEPTH_SLICE

DB:DB_DEPTH_SLICE 寄存器是一个32位寄存器，用于配置深度缓冲区切片的信息。该寄存器的 GPU 寄存器地址为 0x2805c。

**字段解析**

| Field Name      | 位范围 | 默认值 | 描述                                                                                                              |
|-----------------|--------|--------|-----------------------------------------------------------------------------------------------------------------|
| SLICE_TILE_MAX  | 21:0   | none   | 到下一个切片的8x8像素瓦片数加上一些小数以能够旋转瓦片模式。 (pitch*height/64 - 1)。|

DB:DB_DEPTH_SLICE 寄存器用于配置深度缓冲区切片的信息，特别是指定了在下一个切片之前有多少个8x8像素瓦片。这个值被计算为 (pitch*height/64 - 1) 加上一些小数，以便能够旋转瓦片模式。深度缓冲区切片的配置通常与深度缓冲区的大小和布局有关，影响图形渲染中深度信息的存储方式。

### DB_DEPTH_VIEW

DB:DB_DEPTH_VIEW 寄存器是一个32位寄存器，用于选择渲染目标0的切片索引范围。该寄存器的 GPU 寄存器地址为 0x28008。

**字段解析**

| Field Name        | 位范围 | 默认值 | 描述                                                                                         |
|-------------------|--------|--------|--------------------------------------------------------------------------------------------|
| SLICE_START       | 10:0   | none   | 指定此视图的起始切片编号。此字段将添加到 RenderTargetArrayIndex 以计算要渲染的切片。SLICE_START 必须小于或等于 SLICE_MAX。 |
| SLICE_MAX         | 23:13  | none   | 指定此资源的最大允许的 Z 切片索引，即总切片数减一。                                     |
| Z_READ_ONLY       | 24     | none   | 只读 Z 缓冲区，即禁止写入 Z 缓冲区。                                                  |
| STENCIL_READ_ONLY | 25     | none   | 只读模板缓冲区，即禁止写入模板缓冲区。                                              |

DB:DB_DEPTH_VIEW 寄存器用于选择渲染目标0的切片索引范围，并可以配置是否只读的 Z 缓冲区和模板缓冲区。SLICE_START 和 SLICE_MAX 字段用于指定切片范围，Z_READ_ONLY 和 STENCIL_READ_ONLY 字段用于配置只读属性。这些配置影响深度和模板测试的行为。


### DB_EQAA

DB:DB_EQAA 寄存器是一个32位寄存器，用于控制深度缓冲区的等效抗锯齿 (EQAA) 相关设置。EQAA 是一种用于提高图形渲染质量的技术，通过对多个样本进行采样和插值来实现更平滑的图像效果。该寄存器的 GPU 寄存器地址为 0x28804。

**字段解析**

| Field Name                  | 位范围  | 默认值 | 描述                                                                                                                                                                                                                                    |
|-----------------------------|---------|--------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| MAX_ANCHOR_SAMPLES           | 2:0     | none   | 设置CB允许使用的锚定样本数的最大值。将其设置为DB表面分配的最小样本数以限制CB在创建非锚定片段后可能需要丢弃它们。                                                                                                                         |
| PS_ITER_SAMPLES             | 6:4     | none   | 指定PS_ITER_SAMPLE设置时要迭代的样本数量，从而设置超采样量。通常这是应用程序公开的样本数量。不支持大于深度表面样本数的值。                                                                                                                  |
| MASK_EXPORT_NUM_SAMPLES     | 10:8    | none   | 指定用于着色器掩码导出的样本数量。                                                                                                                                                                                                    |
| ALPHA_TO_MASK_NUM_SAMPLES   | 14:12   | none   | 为A2M生成的质量样本数。将其设置在应用程序公开的样本数和较高EQAA样本数之间以进行速度/质量权衡。如果ALPHA_TO_MASK_EQAA_DISABLE=1，则必须将其设置为应用程序公开的样本数。                                              |
| HIGH_QUALITY_INTERSECTIONS   | 16      | none   | 如果未设置，所有完全覆盖的瓦片将以瓦片速率通过详细步进器运行，仅在深度测试结果未知时才以DB的表面速率减慢，或者在着色器执行时以像素速率减慢。如果设置，只会加速已知Z测试结果的完全覆盖瓦片，但仍允许可能存在Z交点的瓦片以详细速率运行，从而获得抗锯齿的交点。最好与INTERPOLATE_COMP_Z一起使用以获得最佳质量。  |
| INCOHERENT_EQAA_READS        | 17      | none   | 禁用对共享锚定样本但不共享详细样本的相邻三角形的一致性检查，对于不存在数据转发的相邻条带来说很重要。可能会引入依赖于延迟的结果，因此除了单元测试外，应强制为0。                                                       |
| INTERPOLATE_COMP_Z           | 18      | none   | 允许未锚定的样本从压缩的Z平面插值出唯一的Z值。在像素的第一个交点上创建漂亮的抗锯齿交点。引入依赖于延迟的结果，因此除了单元定向测试可能需要可视检查外，应将其强制为0。                                                         |
| INTERPOLATE_SRC_Z            | 19      | none   | 强制未锚定样本在目标Z未压缩时插值出唯一的源Z，以获得更平滑的交点，即使在未压缩的Z上也会引发ZFUNC==EQUALS的混合失败。除非进行实验，否则可能永远不会设置。                                                                                                 |
| STATIC_ANCHOR_ASSOCIATIONS   | 20      | none   | 强制复制的目标数据始终来自静态关联的锚定样本，而不是尝试从最靠近的锚定样本拉取目标数据，该样本位于原始图元内。当设置时，可能会导致额外的一致性停顿并且可能会降低相邻三角形的质量。                       |
| ALPHA_TO_MASK_EQAA_DISABLE   | 21      | none   | 使Alpha to Mask设置的样本与先前的GPU完全相同。只有在需要先前一代的行为时才应设置，否则新行为针对EQAA进行了优化，可以在混合AA模式和即使在没有AA时也可以提高质量。                                      |
| OVERRASTERIZATION_AMOUNT     | 26:24   | none   | 超采样中的样本掩码的OR减少次数的对数。                                                                                                                                                                                               |
| ENABLE_POSTZ_OVERRASTERIZATION | 27      | none   | 启用PostZ中的超采样（即，在着色器之后）。                                                                                                                                                                                        |

DB:DB_EQAA 寄存器用于控制深度缓冲区的等效抗锯齿 (EQAA) 相关设置。EQAA 技术可以提高图形渲染的质量，通过对多个样本进行采样和插值来实现更平滑的图像效果。这些字段允许配置 EQAA 的不同方面，以满足特定的渲染需求和质量要求。

###

以下是关于这两个寄存器的描述和字段解析：

**DB:DB_HTILE_DATA_BASE 寄存器**

- 寄存器描述: 用于指定 HTileData 表面的基地址。
- 寄存器地址: 0x28014
- 字段解析：

  | Field Name | 位范围  | 默认值 | 描述                                                                                              |
  |------------|---------|--------|-------------------------------------------------------------------------------------------------|
  | BASE_256B  | 31:0    | none   | HTileData 表面在设备地址空间中第一个字节的位置，必须按256字节对齐。40位地址的高32位。该表面包含 HiZ 数据。 |

**DB:DB_HTILE_SURFACE 寄存器**

- 寄存器描述: 用于配置 HTile 表面的属性。
- 寄存器地址: 0x28abc
- 字段解析：

  | Field Name           | 位范围  | 默认值 | 描述                                                                                                            |
  |----------------------|---------|--------|-------------------------------------------------------------------------------------------------------------------|
  | LINEAR               | 0       | none   | 表面以每次8个 htiles 高的方式线性存储，直到表面完成。                                                              |
  | FULL_CACHE           | 1       | none   | 此 htile 缓冲区使用整个 htile 缓存。如果设置为0且 htile 表面不适合缓存的一半，那么 SC 的部分向量死锁计时器也必须启用。 |
  | HTILE_USES_PRELOAD_WIN | 2       | none   | 如果设置，htile 表面的尺寸将与预加载窗口的尺寸相同；否则，将与深度缓冲区的尺寸相同。                         |
  | PRELOAD              | 3       | none   | 一旦在上下文中看到 VGT_DRAW_INITIATOR，只要有空间可用，就预加载所有适合的数据。                             |
  | PREFETCH_WIDTH       | 9:4     | none   | 预取窗口的宽度（以64像素增量为单位）。预取器尝试始终在缓存中保持此窗口周围最后一个光栅化的 htile。              |
  | PREFETCH_HEIGHT      | 15:10   | none   | 预取窗口的高度（以64像素增量为单位）。预取器尝试始终在缓存中保持此窗口周围最后一个光栅化的 htile。             |
  | DST_OUTSIDE_ZERO_TO_ONE | 16  | none   | 告诉 HiZ 逻辑不要假设深度范围的最小值是精确的0.0或深度范围的最大值是精确的1.0。                            |

这两个寄存器用于配置与深度缓冲区 HiZ 数据相关的设置。DB:DB_HTILE_DATA_BASE 寄存器指定了 HiZ 数据的基地址，而 DB:DB_HTILE_SURFACE 寄存器用于配置 HiZ 数据表面的属性，包括如何存储、预取和其他行为。这些配置可以影响深度测试和图形渲染的性能和质量。


### DB_PRELOAD_CONTROL

DB:DB_PRELOAD_CONTROL 寄存器是一个可读写的 32 位寄存器，用于配置预加载窗口的位置和大小，以 64 像素为增量。该寄存器的地址是 0x28ac8。

以下是字段的定义：

| 字段名称  | 位范围  | 默认值 | 描述                                      |
|-------------|---------|--------|-----------------------------------------|
| START_X    | 7:0     | none   | 预加载窗口的起始 X 位置，以 64 像素为增量。   |
| START_Y    | 15:8    | none   | 预加载窗口的起始 Y 位置，以 64 像素为增量。   |
| MAX_X      | 23:16   | none   | 预加载窗口的结束 X 位置，以 64 像素为增量。   |
| MAX_Y      | 31:24   | none   | 预加载窗口的结束 Y 位置，以 64 像素为增量。   |

DB:DB_PRELOAD_CONTROL 寄存器用于配置在图形渲染过程中哪些像素数据需要被预先加载。可以通过设置这些字段的值来控制预加载窗口的范围，以满足特定的渲染需求。


### DB_RENDER_CONTROL

DB:DB_RENDER_CONTROL 寄存器是一个可读写的 32 位寄存器，用于控制深度和模板测试等渲染操作。该寄存器的地址是 0x28000。

以下是字段的定义：

| 字段名称              | 位范围 | 默认值 | 描述                                                                                           |
|----------------------|--------|--------|--------------------------------------------------------------------------------------------------|
| DEPTH_CLEAR_ENABLE    | 0      | none   | 启用深度缓冲区的清除操作，将深度值清除为指定的 Clear Value。                                       |
| STENCIL_CLEAR_ENABLE  | 1      | none   | 启用模板缓冲区的清除操作，将模板值清除为指定的 Clear Value。                                      |
| DEPTH_COPY            | 2      | none   | 启用深度值的扩展，将其复制到颜色渲染目标 0。必须在颜色缓冲区（CB）中设置所需的目标格式。             |
| STENCIL_COPY          | 3      | none   | 启用模板值的扩展，将其复制到颜色渲染目标 0。必须在颜色缓冲区（CB）中设置所需的目标格式。          |
| RESUMMARIZE_ENABLE    | 4      | none   | 如果设置，所有受影响的瓦片将更新 HTILE 表面信息。                                                 |
| STENCIL_COMPRESS_DISABLE | 5   | none   | 强制在任何渲染的瓦片上禁用层次剔除的模板值解压缩。                                          |
| DEPTH_COMPRESS_DISABLE   | 6   | none   | 强制在任何渲染的瓦片上禁用层次剔除的深度值解压缩。                                          |
| COPY_CENTROID          | 7      | none   | 如果设置，从像素内的第一个亮点样本开始复制，从 COPY_SAMPLE 开始（循环回到较低的样本）。如果 COPY_CENTROID==0 并且启用了深度或模板写入（在生产驱动程序中不会发生），则必须设置 DB_RENDER_OVERRIDE.FORCE_QC_SMASK_CONFLICT。此外，在执行深度或模板复制并且启用 ps_iter 时，必须将 COPY_CENTROID 设置为 1。 |
| COPY_SAMPLE            | 11:8   | none   | 如果 COPY_CENTROID 为真，则从此样本编号开始复制第一个亮点样本。否则，不管是否亮点样本都复制此样本。       |

DB:DB_RENDER_CONTROL 寄存器用于配置深度和模板测试等渲染操作的行为。可以通过设置这些字段的值来控制渲染过程中的不同操作。


### DB_RENDER_OVERRIDE

DB:DB_RENDER_OVERRIDE 寄存器是一个可读写的 32 位寄存器，用于控制深度和模板测试等渲染操作的高级设置。该寄存器的地址是 0x2800c。

以下是字段的定义：

| 字段名称                | 位范围  | 默认值 | 描述                                                                                      |
|------------------------|---------|--------|---------------------------------------------------------------------------------------------|
| FORCE_HIZ_ENABLE       | 1:0     | none   | 强制启用或禁用层次深度剔除，忽略 DB_SHADER_CONTROL 和其他所有渲染状态。                      |
| FORCE_HIS_ENABLE0      | 3:2     | none   | 强制启用或禁用用于比较状态 0 的层次模板剔除，忽略 DB_SHADER_CONTROL 和其他所有渲染状态。       |
| FORCE_HIS_ENABLE1      | 5:4     | none   | 强制启用或禁用用于比较状态 1 的层次模板剔除，忽略 DB_SHADER_CONTROL 和其他所有渲染状态。       |
| FORCE_SHADER_Z_ORDER  | 6       | none   | 强制使用 DB_SHADER_CONTROL.Z_ORDER 中指定的设置进行早期/晚期 Z 和 S 测试。                 |
| FAST_Z_DISABLE         | 7       | none   | 禁用加速 Z 清除或写操作。防止在需要深度操作时在详细栅格化之前终止四分之一。                  |
| FAST_STENCIL_DISABLE   | 8       | none   | 禁用加速模板清除或写操作。防止在需要模板操作时在详细栅格化之前终止四分之一。            |
| NOOP_CULL_DISABLE      | 9       | none   | 防止层次剔除杀死那些通过 Z 和模板测试，但不写入 Z、模板或颜色的四分之一。              |
| FORCE_COLOR_KILL       | 10      | none   | 强制 DB 假定着色器结果不需要，杀死所有样本在颜色操作之前。                                  |
| FORCE_Z_READ           | 11      | none   | 即使不需要，也读取一个瓦片的所有 Z 数据。用于重新汇总 blt。                                   |
| FORCE_STENCIL_READ     | 12      | none   | 即使不需要，也读取一个瓦片的所有模板数据。用于重新汇总 blt。                                   |
| FORCE_FULL_Z_RANGE     | 14:13   | none   | 强制层次深度将每个基元视为其范围为 0.0 -> 1.0f 或不是。如果禁用，它会从 DB_SHADER_CONTROL.Z_EXPORT_ENABLE 和其他启用寄存器中隐式派生。可以用于重置 Z 范围为 0-1。 |
| FORCE_QC_SMASK_CONFLICT | 15      | none   | 强制 Quad 一致性将具有匹配的 dtileid、x 和 y 作为冲突标记，并即使样本掩码不重叠也会停滞。       |
| DISABLE_VIEWPORT_CLAMP | 16      | none   | 禁用视口夹取，允许 Z 数据保持不变。                                                        |
| IGNORE_SC_ZRANGE       | 17      | none   | 在 HiZ 中忽略 SC 的 minZ/maxZ 的顶点边界对瓦片的影响。                                      |
| DISABLE_FULLY_COVERED  | 18      | none   | 禁用进入 DB 的全覆盖瓦片位，关闭所有全覆盖的优化。                                          |
| FORCE_Z_LIMIT_SUMM     | 20:19   | none   | 强制汇总 minz 或 maxz 或两者。                                                            |
| MAX_TILES_IN_DTT       | 25:21   | 0x0    | 在引起停滞之前，在 dtt 块中允许的最大瓦片数。如果 DB_DEBUG.NEVER_FREE_Z_ONLY 设置为，MAX_TILES_IN_DTT 必须小于或等于以下值

，具体取决于 Z 缓冲区中的样本数：1xaa: 21 2xaa: 11 4xaa: 5 8xaa: 2。生产驱动程序预计会将此寄存器保留为默认值 0，以满足约束。   |
| DISABLE_TILE_RATE_TILES | 26      | 0x0    | 禁用允许某些全覆盖的 8x8 以瓦片速率运行的优化。                                          |
| FORCE_Z_DIRTY          | 27      | none   | 即使没有更改，也强制写入 Z 数据。可以用于将 Z 数据复制到备用表面。                             |
| FORCE_STENCIL_DIRTY    | 28      | none   | 即使没有更改，也强制写入模板数据。可以用于将模板数据复制到备用表面。                             |
| FORCE_Z_VALID          | 29      | none   | 强制读取 Z 数据，除非它正在被覆盖。可以用于将 Z 数据复制到备用表面。                             |
| FORCE_STENCIL_VALID    | 30      | none   | 强制读取模板数据，除非它正在被覆盖。可以用于将模板数据复制到备用表面。                             |
| PRESERVE_COMPRESSION   | 31      | none   | 在解压缩到备用表面时可以使用，以防止 htile 的压缩状态不小心标记为扩展。停止写入 htile 缓冲区的 zmask 和 smem 字段的所有数据。 |

DB:DB_RENDER_OVERRIDE 寄存器允许高级配置深度和模板测试等渲染操作的行为，以满足特定需求。通过设置这些字段的值，可以控制各种渲染优化和高级操作。


### DB_RENDER_OVERRIDE2

DB:DB_RENDER_OVERRIDE2 寄存器是一个可读写的 32 位寄存器，用于控制深度和模板测试等高级渲染设置。该寄存器的地址为 0x28010。

以下是字段的定义：

| 字段名称                           | 位范围  | 默认值 | 描述                                                                                           |
|-----------------------------------|---------|--------|------------------------------------------------------------------------------------------------|
| PARTIAL_SQUAD_LAUNCH_CONTROL       | 1:0     | none   | 设置如何启动部分分片。                                                                           |
|                                   |         |        | 可能的值：                                                                                     |
|                                   |         |        | 00 - PSLC_AUTO：让 DB 自动控制部分分片启动。                                                   |
|                                   |         |        | 01 - PSLC_ON_HANG_ONLY：仅在检测到故障时启动部分分片。                                          |
|                                   |         |        | 02 - PSLC_ASAP：启用部分启动的倒计时。PSLC_COUNTDOWN 值为 7 表示立即启动。                             |
|                                   |         |        | 03 - PSLC_COUNTDOWN：启用部分启动的倒计时。PSLC_COUNTDOWN 值为 7 表示不启动部分分片。                         |
| PARTIAL_SQUAD_LAUNCH_COUNTDOWN      | 4:2     | none   | 设置部分分片启动的倒计时，以 (1 << N) 表示。值为 7 表示禁用倒计时。                                     |
| DISABLE_ZMASK_EXPCLEAR_OPTIMIZATION | 5       | none   | 仅在 DB_Z_INFO.ALLOW_EXPCLEAR=1 时有效。用于在未初始化表面上的第一次清除时，当无法信任 zmask 时使用。                |
| DISABLE_SMEM_EXPCLEAR_OPTIMIZATION  | 6       | none   | 仅在 DB_STENCIL_INFO.ALLOW_EXPCLEAR=1 时有效。用于在未初始化表面上的第一次清除时，当无法信任模板内存格式时使用。     |
| DISABLE_COLOR_ON_VALIDATION         | 7       | none   | 禁用 DB 查看 CB_COLOR_INFO、CB_SHADER_MASK 和 CB_TARGET_MASK 以确定颜色是否开启。                              |
| DECOMPRESS_Z_ON_FLUSH              | 8       | none   | 0：在管道内通过分配缓存空间和在管道内解压来执行 Z 解压缩。在更高的 AA 模式下会增加缓存压力。                       |
|                                   |         |        | 1：在刷新到内存时执行 Z 解压缩，不分配缓存空间，但会在每个瓦片的刷新中产生启动延迟。应设置为 0（1xAA 和 2xAA）或 1（4xAA 和 8xAA）。 |
| DISABLE_REG_SNOOP                   | 9       | none   | 禁用 DB 对非 DB 寄存器的窥探。只应在非常特殊的情况下设置，并在开始绘制或复制上下文之前清除。                        |
| DEPTH_BOUNDS_HIER_DEPTH_DISABLE     | 10      | none   | 禁用 hiZ 深度边界测试。hiZ 将无法确定深度边界测试是否通过或失败。                                           |

DB:DB_RENDER_OVERRIDE2 寄存器允许高级配置深度和模板测试等渲染操作的行为，以满足特定需求。通过设置这些字段的值，可以控制各种渲染优化和高级操作。


###  DB_SHADER_CONTROL

DB:DB_SHADER_CONTROL 寄存器是一个可读写的 32 位寄存器，用于控制深度和模板测试以及着色器的行为。该寄存器的地址为 0x2880c。

以下是字段的定义：

| 字段名称                       | 位范围 | 默认值 | 描述                                                                                                   |
|-------------------------------|--------|--------|--------------------------------------------------------------------------------------------------------|
| Z_EXPORT_ENABLE               | 0      | none   | 使用 DB Shader Export 的红色通道作为 Z 值，而不是插值的 Z 值。                                      |
|                               |        |        | SPI_SHADER_Z_FORMAT.Z_EXPORT_FORMAT 必须具有浮点32红色通道（32_ABGR，32_R，32_GR或32_AR）。                    |
| STENCIL_TEST_VAL_EXPORT_ENABLE | 1      | none   | 使用 DB Shader Export 的绿色[7:0]作为模板测试值。Z_EXPORT_FORMAT 必须具有绿色通道（不为 ZERO，32_R 或 32_AR）。 |
| STENCIL_OP_VAL_EXPORT_ENABLE   | 2      | none   | 使用 DB Shader Export 的绿色[15:8]作为模板操作值。Z_EXPORT_FORMAT 必须具有绿色通道（不为 ZERO，32_R 或 32_AR）。  |
| Z_ORDER                       | 5:4    | none   | 指示着色器首选哪种类型的 Z 测试。如果无法使用 EARLY_Z，则 EARLY_Z_THEN_LATE_Z；如果无法使用 RE_Z，则 LATE_Z。     |
|                               |        |        | 可能的值：                                                                                             |
|                               |        |        | 00 - LATE_Z                                                                                            |
|                               |        |        | 01 - EARLY_Z_THEN_LATE_Z                                                                               |
|                               |        |        | 02 - RE_Z                                                                                              |
|                               |        |        | 03 - EARLY_Z_THEN_RE_Z                                                                                |
| KILL_ENABLE                   | 6      | none   | 着色器可以通过 texkill 杀死像素。                                                                    |
| COVERAGE_TO_MASK_ENABLE       | 7      | none   | 使用 DB Shader Export 的 Alpha 通道作为独立的 Alpha to Mask 操作。                                      |
|                               |        |        | Z_EXPORT_FORMAT 必须具有非整数 alpha 通道（32_AR、FP16_ABGR、UNORM16_ABGR、SNORM_ABGR 或 32_ABGR）。                |
| MASK_EXPORT_ENABLE            | 8      | none   | 使用 DB Shader Export 的蓝色通道作为像素的采样掩码。使用最低 NUM_SAMPLES 位。                                 |
|                               |        |        | Z_EXPORT_FORMAT 必须包含一个蓝色通道，始终被解释为采样掩码（*_ABGR）。                                       |
| EXEC_ON_HIER_FAIL             | 9      | none   | 即使在分层 Z 或模板会杀死四分之一时，也会执行着色器。                                                 |
|                               |        |        | 启用时，如果像素着色器具有对失败或通过样本（当 DEPTH_BEFORE_SHADER=0 时）的任何样本有所需的副作用（注意，EarlyZ 和 ReZ 杀死仍然会阻止着色器运行）。 |
| EXEC_ON_NOOP                  | 10     | none   | 即使没有东西使用着色器的颜色或深度输出，也会执行着色器。                                               |
|                               |        |        | 启用时，如果像素着色器具有仅针对通过像素产生的副作用而不是由上述标志引起的副作用，则应启用。                        |
| ALPHA_TO_MASK_DISABLE         | 11     | none   | 如果设置，将禁用 alpha 到掩码，覆盖 DB_ALPHA_TO_MASK.ALPHA_TO_MASK_ENABLE。                              |
| DEPTH_BEFORE_SHADER           | 12     | none   | 着色器被定义为在深度之后运行，这将阻止着色器杀死样本和/或像素（alpha 测试、alpha 到覆盖、覆盖到掩码、掩码导出、Z/模板导出）影响深度操作，

因此不允许这些操作禁止 EarlyZ。此外，ZPass 计数被定义为在 Z 测试之后计数，因此此模式使得着色器和基于 alpha 的剔除不再减少 ZPass 计数。  |
| CONSERVATIVE_Z_EXPORT         | 14:13  | none   | 强制 z 导出为小于或大于源 z 值。                                                                       |
|                               |        |        | 可能的值：                                                                                             |
|                               |        |        | 00 - EXPORT_ANY_Z：导出的 Z 可以是任何值                                                            |
|                               |        |        | 01 - EXPORT_LESS_THAN_Z：导出的 Z 将被假定小于源 z 值                                                  |
|                               |        |        | 02 - EXPORT_GREATER_THAN_Z：导出的 Z 将被假定大于源 z 值                                                |
|                               |        |        | 03 - EXPORT_RESERVED：保留                                                                              |

DB:DB_SHADER_CONTROL 寄存器允许控制深度和模板测试以及着色器行为的高级设置。这些字段的配置可以根据需求进行调整，以实现特定的渲染和深度测试效果。

### DB_SRESULTS_COMPARE_STATE0

DB:DB_SRESULTS_COMPARE_STATE0 寄存器是一个可读写的 32 位寄存器，用于配置与层次模板测试相关的比较状态。该寄存器的地址为 0x28ac0。

以下是字段的定义：

| 字段名称        | 位范围   | 默认值 | 描述                                                                                     |
|----------------|----------|--------|------------------------------------------------------------------------------------------|
| COMPAREFUNC0   | 2:0      | none   | 用于确定在层次模板测试期间 MayPass 和 MayFail smask 位的含义。NEVER 或 ALWAYS 会使 HTile 缓冲区中的 SResults 无效。可能的值： 00 - REF_NEVER: 永不通过 01 - REF_LESS: 如果 left < right 则通过 02 - REF_EQUAL: 如果 left = right 则通过 03 - REF_LEQUAL: 如果 left <= right 则通过 04 - REF_GREATER: 如果 left > right 则通过 05 - REF_NOTEQUAL: 如果 left != right 则通过 06 - REF_GEQUAL: 如果 left >= right 则通过 07 - REF_ALWAYS: 总是通过 |
| COMPAREVALUE0  | 11:4     | none   | 在层次模板测试期间与模板参考值比较的模板值。                                          |
| COMPAREMASK0   | 19:12    | none   | 该值与 SResults 比较值进行 AND 运算。如果掩码为 0，则使 HTile 缓冲区中的 SResults 无效。             |
| ENABLE0        | 24       | none   | 如果设置，使用 HiS 测试中的 SResults。当比较状态已知时设置，当进行重新总结时清除。                      |

DB:DB_SRESULTS_COMPARE_STATE0 寄存器允许配置用于层次模板测试的比较状态，以及在比较期间使用的模板值和掩码。这些设置可用于优化模板测试和深度测试的性能和准确性。

### DB_SRESULTS_COMPARE_STATE1

DB:DB_SRESULTS_COMPARE_STATE1 寄存器是一个可读写的 32 位寄存器，用于控制在分层模板测试期间用于比较的模板测试参数。该寄存器的地址为 0x28ac4。

以下是字段的定义：

| 字段名称         | 位范围   | 默认值 | 描述                                                                                   |
|-----------------|----------|--------|----------------------------------------------------------------------------------------|
| COMPAREFUNC1    | 2:0      | none   | 用于确定在分层模板测试期间用于比较的模板测试参数的含义。                             |
|                 |          |        | NEVER 或 ALWAYS 会使 HTile 缓冲区中的 SResults 无效。                               |
|                 |          |        | 可能的值：                                                                           |
|                 |          |        | 00 - REF_NEVER: 从不通过                                                             |
|                 |          |        | 01 - REF_LESS: 如果左侧 < 右侧通过                                                    |
|                 |          |        | 02 - REF_EQUAL: 如果左侧 = 右侧通过                                                    |
|                 |          |        | 03 - REF_LEQUAL: 如果左侧 <= 右侧通过                                                  |
|                 |          |        | 04 - REF_GREATER: 如果左侧 > 右侧通过                                                  |
|                 |          |        | 05 - REF_NOTEQUAL: 如果左侧 != 右侧通过                                                |
|                 |          |        | 06 - REF_GEQUAL: 如果左侧 >= 右侧通过                                                  |
|                 |          |        | 07 - REF_ALWAYS: 总是通过                                                           |
| COMPAREVALUE1   | 11:4     | none   | 分层模板测试期间与模板参考值进行比较的模板值。                                      |
| COMPAREMASK1    | 19:12    | none   | 此值与 SResults 比较值进行 AND 运算。如果掩码为 0，则使 HTile 缓冲区中的 SResults 无效。 |
| ENABLE1         | 24       | none   | 如果设置，使用 HiS 测试中的 SResults。在已知比较状态时设置，进行重新汇总时清除。         |

这些字段允许配置在模板测试期间如何处理模板比较，以及在分层模板测试期间如何使用 SResults。模板测试用于确定像素是否通过了模板测试，而 SResults 是一个包含关于测试的结果的缓冲区。



### DB_STENCIL_CONTROL 

DB:DB_STENCIL_CONTROL 寄存器是一个可读写的 32 位寄存器，用于控制模板测试的操作，包括前向面和后向面的情况。该寄存器的地址为 0x2842c。

以下是字段的定义：

| 字段名称          | 位范围    | 默认值 | 描述                                                                                             |
|------------------|-----------|--------|--------------------------------------------------------------------------------------------------|
| STENCILFAIL      | 3:0       | none   | 指定模板测试失败时用于前向面四边形的模板操作。                                                 |
|                  |           |        | 可能的值：                                                                                     |
|                  |           |        | 00 - STENCIL_KEEP: 新值 = 旧值                                                                 |
|                  |           |        | 01 - STENCIL_ZERO: 新值 = 0                                                                   |
|                  |           |        | 02 - STENCIL_ONES: 新值 = 8'hff                                                               |
|                  |           |        | 03 - STENCIL_REPLACE_TEST: 新值 = STENCIL_TEST_VAL                                             |
|                  |           |        | 04 - STENCIL_REPLACE_OP: 新值 = STENCIL_OP_VAL                                                 |
|                  |           |        | 05 - STENCIL_ADD_CLAMP: 新值 = 旧值 + STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 06 - STENCIL_SUB_CLAMP: 新值 = 旧值 - STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 07 - STENCIL_INVERT: 新值 = 〜旧值                                                            |
|                  |           |        | 08 - STENCIL_ADD_WRAP: 新值 = 旧值 + STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 09 - STENCIL_SUB_WRAP: 新值 = 旧值 - STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 10 - STENCIL_AND: 新值 = 旧值 & STENCIL_OP_VAL                                                |
|                  |           |        | 11 - STENCIL_OR: 新值 = 旧值 | STENCIL_OP_VAL                                                 |
|                  |           |        | 12 - STENCIL_XOR: 新值 = 旧值 ^ STENCIL_OP_VAL                                                 |
|                  |           |        | 13 - STENCIL_NAND: 新值 = 〜（旧值 & STENCIL_OP_VAL）                                           |
|                  |           |        | 14 - STENCIL_NOR: 新值 = 〜（旧值 | STENCIL_OP_VAL）                                           |
|                  |           |        | 15 - STENCIL_XNOR: 新值 = 〜（旧值 ^ STENCIL_OP_VAL）                                          |
| STENCILZPASS     | 7:4       | none   | 指定模板和深度函数都通过时用于前向面四边形的模板操作。                                         |
|                  |           |        | 可能的值：                                                                                     |
|                  |           |        | 00 - STENCIL_KEEP: 新值 = 旧值                                                                 |
|                  |           |        | 01 - STENCIL_ZERO: 新值 = 0                                                                   |
|                  |           |        | 02 - STENCIL_ONES: 新值 = 8'hff                                                               |
|                  |           |        | 03 - STENCIL_REPLACE_TEST: 新值 = STENCIL_TEST_VAL                                             |
|                  |           |        | 04 - STENCIL_REPLACE_OP: 新值 = STENCIL_OP_VAL                                                 |
|                  |           |        | 05 - STENCIL_ADD_CLAMP: 新值 = 旧值 + STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 06 - STENCIL_SUB_CLAMP: 新值 = 旧值 - STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 07 - STENCIL_INVERT: 新值 = 〜旧值                                                            |
|                  |           |        | 08 - STENCIL_ADD_WRAP: 新值 = 旧值 + STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 09 - STENCIL_SUB_WRAP: 新值 = 旧值 - STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 10 - STENCIL_AND: 新值 = 旧值 & STENCIL_OP_VAL                                                |
|                  |           |        | 11 - STENCIL_OR: 新值 = 旧值 | STENCIL_OP_VAL                                                 |
|                  |           |        | 12 - STENCIL_XOR: 新值 = 旧值 ^ STENCIL_OP_VAL                                                 |
|                  |           |        | 13 - STENCIL_NAND: 新值 = 〜（旧值 & STENCIL_OP_VAL）                                           |
|                  |           |        | 14 - STENCIL_NOR: 新值 = 〜（旧值 | STENCIL_OP_VAL）                                           |
|                  |           |        | 15 - STENCIL_XNOR: 新值 = 〜（旧

值 ^ STENCIL_OP_VAL）                                          |
| STENCILZFAIL     | 11:8      | none   | 指定模板函数通过但深度函数失败时用于前向面四边形的模板操作。                                   |
|                  |           |        | 可能的值：                                                                                     |
|                  |           |        | 00 - STENCIL_KEEP: 新值 = 旧值                                                                 |
|                  |           |        | 01 - STENCIL_ZERO: 新值 = 0                                                                   |
|                  |           |        | 02 - STENCIL_ONES: 新值 = 8'hff                                                               |
|                  |           |        | 03 - STENCIL_REPLACE_TEST: 新值 = STENCIL_TEST_VAL                                             |
|                  |           |        | 04 - STENCIL_REPLACE_OP: 新值 = STENCIL_OP_VAL                                                 |
|                  |           |        | 05 - STENCIL_ADD_CLAMP: 新值 = 旧值 + STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 06 - STENCIL_SUB_CLAMP: 新值 = 旧值 - STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 07 - STENCIL_INVERT: 新值 = 〜旧值                                                            |
|                  |           |        | 08 - STENCIL_ADD_WRAP: 新值 = 旧值 + STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 09 - STENCIL_SUB_WRAP: 新值 = 旧值 - STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 10 - STENCIL_AND: 新值 = 旧值 & STENCIL_OP_VAL                                                |
|                  |           |        | 11 - STENCIL_OR: 新值 = 旧值 | STENCIL_OP_VAL                                                 |
|                  |           |        | 12 - STENCIL_XOR: 新值 = 旧值 ^ STENCIL_OP_VAL                                                 |
|                  |           |        | 13 - STENCIL_NAND: 新值 = 〜（旧值 & STENCIL_OP_VAL）                                           |
|                  |           |        | 14 - STENCIL_NOR: 新值 = 〜（旧值 | STENCIL_OP_VAL）                                           |
|                  |           |        | 15 - STENCIL_XNOR: 新值 = 〜（旧值 ^ STENCIL_OP_VAL）                                          |
| STENCILFAIL_BF   | 15:12     | none   | 指定模板函数失败时用于后向面四边形的模板操作。                                                 |
|                  |           |        | 可能的值：                                                                                     |
|                  |           |        | 00 - STENCIL_KEEP: 新值 = 旧值                                                                 |
|                  |           |        | 01 - STENCIL_ZERO: 新值 = 0                                                                   |
|                  |           |        | 02 - STENCIL_ONES: 新值 = 8'hff                                                               |
|                  |           |        | 03 - STENCIL_REPLACE_TEST: 新值 = STENCIL_TEST_VAL                                             |
|                  |           |        | 04 - STENCIL_REPLACE_OP: 新值 = STENCIL_OP_VAL                                                 |
|                  |           |        | 05 - STENCIL_ADD_CLAMP: 新值 = 旧值 + STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 06 - STENCIL_SUB_CLAMP: 新值 = 旧值 - STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 07 - STENCIL_INVERT: 新值 = 〜旧值                                                            |
|                  |           |        | 08 - STENCIL_ADD_WRAP: 新值 = 旧值 + STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 09 - STENCIL_SUB_WRAP: 新值 = 旧值 - STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 10 - STENCIL_AND: 新值 = 旧值 & STENCIL_OP_VAL                                                |
|                  |           |        | 11 - STENCIL_OR: 新值 = 旧值 | STENCIL_OP_VAL                                                 |
|                  |           |        | 12 - STENCIL_XOR: 新值 = 旧值 ^ STENCIL_OP_VAL                                                 |
|                  |           |        | 13 - STENCIL_NAND: 新值 = 〜（旧值 & STENCIL_OP_VAL）                                           |
|                  |           |        | 14 - STENCIL_NOR: 新值 = 〜（旧值 | STENCIL_OP_VAL）                                           |
|                  |           |        | 15 - STENCIL_XNOR: 新值 = 〜（旧值 ^ STENCIL_OP_VAL）                                          |
| STENCILZPASS_BF  | 19:16     | none   | 指定模板和深度函数都通过时用于后向面四边形的模板操作。                                       |
|                  |           |        | 可能的值

：                                                                                     |
|                  |           |        | 00 - STENCIL_KEEP: 新值 = 旧值                                                                 |
|                  |           |        | 01 - STENCIL_ZERO: 新值 = 0                                                                   |
|                  |           |        | 02 - STENCIL_ONES: 新值 = 8'hff                                                               |
|                  |           |        | 03 - STENCIL_REPLACE_TEST: 新值 = STENCIL_TEST_VAL                                             |
|                  |           |        | 04 - STENCIL_REPLACE_OP: 新值 = STENCIL_OP_VAL                                                 |
|                  |           |        | 05 - STENCIL_ADD_CLAMP: 新值 = 旧值 + STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 06 - STENCIL_SUB_CLAMP: 新值 = 旧值 - STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 07 - STENCIL_INVERT: 新值 = 〜旧值                                                            |
|                  |           |        | 08 - STENCIL_ADD_WRAP: 新值 = 旧值 + STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 09 - STENCIL_SUB_WRAP: 新值 = 旧值 - STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 10 - STENCIL_AND: 新值 = 旧值 & STENCIL_OP_VAL                                                |
|                  |           |        | 11 - STENCIL_OR: 新值 = 旧值 | STENCIL_OP_VAL                                                 |
|                  |           |        | 12 - STENCIL_XOR: 新值 = 旧值 ^ STENCIL_OP_VAL                                                 |
|                  |           |        | 13 - STENCIL_NAND: 新值 = 〜（旧值 & STENCIL_OP_VAL）                                           |
|                  |           |        | 14 - STENCIL_NOR: 新值 = 〜（旧值 | STENCIL_OP_VAL）                                           |
|                  |           |        | 15 - STENCIL_XNOR: 新值 = 〜（旧值 ^ STENCIL_OP_VAL）                                          |
| STENCILZFAIL_BF  | 23:20     | none   | 指定模板函数通过但深度函数失败时用于后向面四边形的模板操作。                                   |
|                  |           |        | 可能的值：                                                                                     |
|                  |           |        | 00 - STENCIL_KEEP: 新值 = 旧值                                                                 |
|                  |           |        | 01 - STENCIL_ZERO: 新值 = 0                                                                   |
|                  |           |        | 02 - STENCIL_ONES: 新值 = 8'hff                                                               |
|                  |           |        | 03 - STENCIL_REPLACE_TEST: 新值 = STENCIL_TEST_VAL                                             |
|                  |           |        | 04 - STENCIL_REPLACE_OP: 新值 = STENCIL_OP_VAL                                                 |
|                  |           |        | 05 - STENCIL_ADD_CLAMP: 新值 = 旧值 + STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 06 - STENCIL_SUB_CLAMP: 新值 = 旧值 - STENCIL_OP_VAL（夹紧）                                     |
|                  |           |        | 07 - STENCIL_INVERT: 新值 = 〜旧值                                                            |
|                  |           |        | 08 - STENCIL_ADD_WRAP: 新值 = 旧值 + STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 09 - STENCIL_SUB_WRAP: 新值 = 旧值 - STENCIL_OP_VAL（环绕）                                     |
|                  |           |        | 10 - STENCIL_AND: 新值 = 旧值 & STENCIL_OP_VAL                                                |
|                  |           |        | 11 - STENCIL_OR: 新值 = 旧值 | STENCIL_OP_VAL                                                 |
|                  |           |        | 12 - STENCIL_XOR: 新值 = 旧值 ^ STENCIL_OP_VAL                                                 |
|                  |           |        | 13 - STENCIL_NAND: 新值 = 〜（旧值 & STENCIL_OP_VAL）                                           |
|                  |           |        | 14 - STENCIL_NOR: 新值 = 〜（旧值 | STENCIL_OP_VAL）                                           |
|                  |           |        | 15 - STENCIL_XNOR: 新值 = 〜（旧值 ^ STENCIL_OP_VAL）                                          |

这些字段控制模板测试在前向面和后向面四边形上的操作。根据模板测试的结果和深度测试的结果，可以使用不同的操作来更新模板缓冲区的值。您可以根据需要配置这些字段以实现所需的模板测试行为。


### DB_STENCIL_INFO


DB:DB_STENCIL_INFO 寄存器是一个可读写的 32 位寄存器，用于配置模板测试过程中的模板缓冲区的属性。该寄存器的地址为 0x28044。

以下是字段的定义：

| 字段名称            | 位范围    | 默认值 | 描述                                                                                     |
|--------------------|-----------|--------|------------------------------------------------------------------------------------------|
| FORMAT             | 0         | none   | 指定模板组件的大小。                                                                      |
|                    |           |        | 可能的值：                                                                               |
|                    |           |        | 00 - STENCIL_INVALID: 无效的模板表面。                                                   |
|                    |           |        | 01 - STENCIL_8: 8 位整数模板表面。                                                        |
| TILE_MODE_INDEX    | 22:20     | none   | 该表面将用于 tile_split 的 GB_TILE_MODEn 寄存器的索引。所有其他字段将来自 DB_Z_INFO 寄存器。 |
| ALLOW_EXPCLEAR     | 27        | none   | 允许模板内存格式跟踪扩展和清除。                                                           |
| TILE_STENCIL_DISABLE | 29      | none   | 表示 htile 缓冲区没有模板元数据。这提高了 hiz 的精度，但取消了模板压缩或 HiStencil 优化的使用。 |

### DB_STENCIL_READ_BASE

DB:DB_STENCIL_READ_BASE 寄存器是一个可读写的 32 位寄存器，用于配置模板读取过程中的模板缓冲区的地址。该寄存器的地址为 0x2804c。

以下是字段的定义：

| 字段名称      | 位范围    | 默认值 | 描述                                             |
|--------------|-----------|--------|--------------------------------------------------|
| BASE_256B    | 31:0      | none   | 模板读取操作中的模板表面的首字节位置，必须是 256 字节对齐的。40 位地址的高 32 位。 |

这个字段允许指定用于读取模板的模板缓冲区的起始地址，以便在渲染过程中读取模板数据。


###  DB_Z_INFO

DB:DB_Z_INFO 寄存器是一个可读写的 32 位寄存器，用于控制深度缓冲区（Z 缓冲区）的属性和行为。该寄存器的地址为 0x28040。

以下是字段的定义：

| 字段名称           | 位范围 | 默认值 | 描述                                                                                                   |
|-------------------|--------|--------|--------------------------------------------------------------------------------------------------------|
| FORMAT            | 1:0    | none   | 指定深度分量的大小以及深度是否为浮点数。                                                            |
|                   |        |        | 可能的值：                                                                                             |
|                   |        |        | 00 - Z_INVALID：无效的深度表面。                                                                      |
|                   |        |        | 01 - Z_16：16 位 UNORM 深度表面。                                                                    |
|                   |        |        | 02 - Z_24：（已弃用：请改用 Z_32_FLOAT）24 位 UNORM 深度表面。                                           |
|                   |        |        | 03 - Z_32_FLOAT：32 位浮点深度表面。                                                                |
| NUM_SAMPLES       | 3:2    | none   | 指定 Z 表面的多重采样（MSAA）表面占用。                                                            |
| TILE_MODE_INDEX   | 22:20  | none   | 指示此表面将使用的 GB_TILE_MODEn 寄存器的索引。                                                        |
| ALLOW_EXPCLEAR    | 27     | none   | 允许 ZMask 跟踪扩展和清除。                                                                         |
| READ_SIZE         | 28     | none   | 设置读取的最小大小为 512 位。如果该表面位于具有小于 512 位访问的内存池中，则设置此选项。              |
|                   |        |        | 可能的值：                                                                                             |
|                   |        |        | 00 - READ_256_BITS                                                                                     |
|                   |        |        | 01 - READ_512_BITS                                                                                     |
| TILE_SURFACE_ENABLE | 29   | none   | 启用读写 htile 数据。如果关闭，HiZ+S 也将关闭。                                                        |
| ZRANGE_PRECISION  | 31     | none   | 0 = ZMin 是基础值，通常在执行 Z > 测试时设置。                                                             |
|                   |        |        | 1 = ZMax 是基础值，通常在执行 Z < 测试时设置。                                                             |
|                   |        |        | 基础值具有完整的 14 位精度。通过将基础值设置为 Max，在 < 测试中剔除时误差较小。                             |
|                   |        |        | 仅可在完全表面清除后更改此字段。如果 TILE_Z_ONLY == 0，则此字段才具有意义。                                      |

DB:DB_Z_INFO 寄存器允许您配置深度缓冲区的格式、多重采样属性以及其他深度缓冲区相关的选项。这些设置可以影响深度测试和渲染的行为。



### 


### CB_BLEND[0-7]_CONTROL

CB:CB_BLEND[0-7]_CONTROL 寄存器是一个可读写的 32 位寄存器，用于控制混合（Blending）操作的设置。这些设置适用于渲染目标 RT0，而 RT1-7 的设置类似。该寄存器的地址范围为 0x28780 到 0x2879c。

以下是字段的定义：

**颜色混合（Color Blend）设置：**

| 字段名称            | 位范围 | 默认值 | 描述                                                                                   |
|--------------------|--------|--------|----------------------------------------------------------------------------------------|
| COLOR_SRCBLEND     | 4:0    | none   | RGB 分量的源混合函数。BLEND_X 对应于 GL_X 混合函数。                                 |
|                    |        |        | 可能的值：                                                                           |
|                    |        |        | 00 - BLEND_ZERO：(d3d_zero)                                                        |
|                    |        |        | 01 - BLEND_ONE：(d3d_one)                                                          |
|                    |        |        | 02 - BLEND_SRC_COLOR：(d3d_srccolor)                                                |
|                    |        |        | 03 - BLEND_ONE_MINUS_SRC_COLOR：(d3d_invsrccolor)                                    |
|                    |        |        | 04 - BLEND_SRC_ALPHA：(d3d_srcalpha)                                                |
|                    |        |        | 05 - BLEND_ONE_MINUS_SRC_ALPHA：(d3d_invsrcalpha)                                    |
|                    |        |        | 06 - BLEND_DST_ALPHA：(d3d_destalpha)                                                |
|                    |        |        | 07 - BLEND_ONE_MINUS_DST_ALPHA：(d3d_invdestalpha)                                    |
|                    |        |        | 08 - BLEND_DST_COLOR：(d3d_destcolor)                                                |
|                    |        |        | 09 - BLEND_ONE_MINUS_DST_COLOR：(d3d_invdestcolor)                                    |
|                    |        |        | 10 - BLEND_SRC_ALPHA_SATURATE：(d3d_srcalphasat)                                    |
|                    |        |        | 11 - 保留                                                                              |
|                    |        |        | 12 - 保留                                                                              |
|                    |        |        | 13 - BLEND_CONSTANT_COLOR：(d3d_blendfactor，使用相应的 RB_BLEND 分量)               |
|                    |        |        | 14 - BLEND_ONE_MINUS_CONSTANT_COLOR：(d3d_invblendfactor)                             |
|                    |        |        | 15 - BLEND_SRC1_COLOR：DX10 双源模式                                                 |
|                    |        |        | 16 - BLEND_INV_SRC1_COLOR：DX10 双源模式                                             |
|                    |        |        | 17 - BLEND_SRC1_ALPHA：DX10 双源模式                                                 |
|                    |        |        | 18 - BLEND_INV_SRC1_ALPHA：DX10 双源模式                                             |
|                    |        |        | 19 - BLEND_CONSTANT_ALPHA：(使用 RB_BLEND_ALPHA)                                   |
|                    |        |        | 20 - BLEND_ONE_MINUS_CONSTANT_ALPHA：                                                  |
| COLOR_COMB_FCN     | 7:5    | none   | RGB 分量的源/目标组合函数。结果被夹紧到可表示的范围。                           |
|                    |        |        | 可能的值：                                                                           |
|                    |        |        | 00 - COMB_DST_PLUS_SRC：(ADD)：Source*SRCBLEND + Dest*DSTBLEND                       |
|                    |        |        | 01 - COMB_SRC_MINUS_DST：(SUBTRACT)：Source*SRCBLEND - Dest*DSTBLEND                  |
|                    |        |        | 02 - COMB_MIN_DST_SRC：(MIN)：min(Source*SRCBLEND, Dest*DSTBLEND)                     |
|                    |        |        | 03 - COMB_MAX_DST_SRC：(MAX)：max(Source*SRCBLEND, Dest*DSTBLEND)                     |
|                    |        |        | 04 - COMB_DST_MINUS_SRC：(REVSUBTRACT)：Dest*DSTBLEND - Source*SRCBLEND               |

**颜色混合（Color Blend）设置（续）：**

| 字段名称            | 位范围 | 默认值 | 描述                                                                                    |
|--------------------|--------|--------|-----------------------------------------------------------------------------------------|
| COLOR_DESTBLEND    | 12:8   | none   | RGB 分量的目标混合函数。BLEND_X 对应于 GL_X 混合函数。                                |
|                    |        |        | 可能的值：                                                                            |
|                    |        |        | 00 - BLEND_ZERO：(d3d_zero)                                                         |
|                    |        |        | 01 - BLEND_ONE：(d3d_one)                                                           |
|                    |        |        | 02 - BLEND_SRC_COLOR：(d3d_srccolor)                                                 |
|                    |        |        | 03 - BLEND_ONE_MINUS_SRC_COLOR：(d3d_invsrccolor)                                     |
|                    |        |        | 04 - BLEND_SRC_ALPHA：(d3d_srcalpha)                                                 |
|                    |        |        | 05 - BLEND_ONE_MINUS_SRC_ALPHA：(d3d_invsrcalpha)                                     |
|                    |        |        | 06 - BLEND_DST_ALPHA：(d3d_destalpha)                                                 |
|                    |        |        | 07 - BLEND_ONE_MINUS_DST_ALPHA：(d3d_invdestalpha)                                     |
|                    |        |        | 08 - BLEND_DST_COLOR：(d3d_destcolor)                                                 |
|                    |        |        | 09 - BLEND_ONE_MINUS_DST_COLOR：(d3d_invdestcolor)                                     |
|                    |        |        | 10 - BLEND_SRC_ALPHA_SATURATE：(d3d_srcalphasat)                                     |
|                    |        |        | 11 - 保留                                                                               |
|                    |        |        | 12 - 保留                                                                               |
|                    |        |        | 13 - BLEND_CONSTANT_COLOR：(d3d_blendfactor，使用相应的 RB_BLEND 分量)                |
|                    |        |        | 14 - BLEND_ONE_MINUS_CONSTANT_COLOR：(d3d_invblendfactor)                              |
|                    |        |        | 15 - BLEND_SRC1_COLOR：DX10 双源模式                                                    |
|                    |        |        | 16 - BLEND_INV_SRC1_COLOR：DX10

 双源模式                                                |
|                    |        |        | 17 - BLEND_SRC1_ALPHA：DX10 双源模式                                                    |
|                    |        |        | 18 - BLEND_INV_SRC1_ALPHA：DX10 双源模式                                                |
|                    |        |        | 19 - BLEND_CONSTANT_ALPHA：(使用 RB_BLEND_ALPHA)                                      |
|                    |        |        | 20 - BLEND_ONE_MINUS_CONSTANT_ALPHA：                                                   |
| ALPHA_SRCBLEND     | 20:16  | none   | Alpha 分量的源混合函数。BLEND_X 对应于 GL_X 混合函数。                                |
|                    |        |        | 可能的值：                                                                            |
|                    |        |        | 00 - BLEND_ZERO：(d3d_zero)                                                         |
|                    |        |        | 01 - BLEND_ONE：(d3d_one)                                                           |
|                    |        |        | 02 - BLEND_SRC_COLOR：(d3d_srccolor)                                                 |
|                    |        |        | 03 - BLEND_ONE_MINUS_SRC_COLOR：(d3d_invsrccolor)                                     |
|                    |        |        | 04 - BLEND_SRC_ALPHA：(d3d_srcalpha)                                                 |
|                    |        |        | 05 - BLEND_ONE_MINUS_SRC_ALPHA：(d3d_invsrcalpha)                                     |
|                    |        |        | 06 - BLEND_DST_ALPHA：(d3d_destalpha)                                                 |
|                    |        |        | 07 - BLEND_ONE_MINUS_DST_ALPHA：(d3d_invdestalpha)                                     |
|                    |        |        | 08 - BLEND_DST_COLOR：(d3d_destcolor)                                                 |
|                    |        |        | 09 - BLEND_ONE_MINUS_DST_COLOR：(d3d_invdestcolor)                                     |
|                    |        |        | 10 - BLEND_SRC_ALPHA_SATURATE：(d3d_srcalphasat)                                     |
|                    |        |        | 11 - 保留                                                                               |
|                    |        |        | 12 - 保留                                                                               |
|                    |        |        | 13 - BLEND_CONSTANT_COLOR：(d3d_blendfactor，使用相应的 RB_BLEND 分量)                |
|                    |        |        | 14 - BLEND_ONE_MINUS_CONSTANT_COLOR：(d3d_invblendfactor)                              |
|                    |        |        | 15 - BLEND_SRC1_COLOR：DX10 双源模式                                                    |
|                    |        |        | 16 - BLEND_INV_SRC1_COLOR：DX10 双源模式                                                |
|                    |        |        | 17 - BLEND_SRC1_ALPHA：DX10 双源模式                                                    |
|                    |        |        | 18 - BLEND_INV_SRC1_ALPHA：DX10 双源模式                                                |
|                    |        |        | 19 - BLEND_CONSTANT_ALPHA：(使用 RB_BLEND_ALPHA)                                      |
|                    |        |        | 20 - BLEND_ONE_MINUS_CONSTANT_ALPHA：                                                   |
| ALPHA_COMB_FCN     | 23:21  | none   | Alpha 分量的源/目标组合函数。结果被夹紧到可表示的范围。                                |
|                    |        |        | 注意，Min 和 Max 不会强制源和目标混合函数为 ONE。                                    |
|                    |        |        | 可能的值：                                                                            |
|                    |        |        | 00 - COMB_DST_PLUS_SRC：(ADD)：Source*SRCBLEND + Dest*DSTBLEND                        |
|                    |        |        | 01 - COMB_SRC_MINUS_DST：(SUBTRACT)：Source*SRCBLEND - Dest*DSTBLEND                   |
|                    |        |        | 02 - COMB_MIN_DST_SRC：(MIN)：min(Source*SRCBLEND, Dest*DSTBLEND)                      |
|                    |        |        | 03 - COMB_MAX_DST_SRC：(MAX)：max(Source*SRCBLEND, Dest*DSTBLEND)                      |
|                    |        |        | 04 - COMB_DST_MINUS_SRC：(REVSUBTRACT)：Dest*DSTBLEND - Source*SRCBLEND                |
|                    |        |        |                                                                                       |
| ALPHA_DESTBLEND    | 28:24  | none   | Alpha 分量的目标混合函数。BLEND_X 对应于 GL_X 混合函数。                                |
|                    |        |        | 可能的值：                                                                            |
|                    |        |        | 00 - BLEND_ZERO：(d3d_zero)                                                         |
|                    |        |        | 01

 - BLEND_ONE：(d3d_one)                                                           |
|                    |        |        | 02 - BLEND_SRC_COLOR：(d3d_srccolor)                                                 |
|                    |        |        | 03 - BLEND_ONE_MINUS_SRC_COLOR：(d3d_invsrccolor)                                     |
|                    |        |        | 04 - BLEND_SRC_ALPHA：(d3d_srcalpha)                                                 |
|                    |        |        | 05 - BLEND_ONE_MINUS_SRC_ALPHA：(d3d_invsrcalpha)                                     |
|                    |        |        | 06 - BLEND_DST_ALPHA：(d3d_destalpha)                                                 |
|                    |        |        | 07 - BLEND_ONE_MINUS_DST_ALPHA：(d3d_invdestalpha)                                     |
|                    |        |        | 08 - BLEND_DST_COLOR：(d3d_destcolor)                                                 |
|                    |        |        | 09 - BLEND_ONE_MINUS_DST_COLOR：(d3d_invdestcolor)                                     |
|                    |        |        | 10 - BLEND_SRC_ALPHA_SATURATE：(d3d_srcalphasat)                                     |
|                    |        |        | 11 - 保留                                                                               |
|                    |        |        | 12 - 保留                                                                               |
|                    |        |        | 13 - BLEND_CONSTANT_COLOR：(d3d_blendfactor，使用相应的 RB_BLEND 分量)                |
|                    |        |        | 14 - BLEND_ONE_MINUS_CONSTANT_COLOR：(d3d_invblendfactor)                              |
|                    |        |        | 15 - BLEND_SRC1_COLOR：DX10 双源模式                                                    |
|                    |        |        | 16 - BLEND_INV_SRC1_COLOR：DX10 双源模式                                                |
|                    |        |        | 17 - BLEND_SRC1_ALPHA：DX10 双源模式                                                    |
|                    |        |        | 18 - BLEND_INV_SRC1_ALPHA：DX10 双源模式                                                |
|                    |        |        | 19 - BLEND_CONSTANT_ALPHA：(使用 RB_BLEND_ALPHA)                                      |
|                    |        |        | 20 - BLEND_ONE_MINUS_CONSTANT_ALPHA：                                                   |
| SEPARATE_ALPHA_BLEND | 29     | none   | 如果为 false，则使用颜色混合模式来混合 alpha 通道。如果为 true，则使用 ALPHA_ 字段来控制混合到 alpha 通道。 |
| ENABLE             | 30     | none   | 1=启用此 MRT 的混合，0=禁用此 MRT 的混合。如果启用混合，则它会覆盖并禁用 ROP3。                      |
| DISABLE_ROP3       | 31     | 0x0    | (默认) 0=启用此 MRT 的 ROP3，1=禁用此 MRT 的 ROP3。如果启用混合，则 ROP3 会被覆盖并禁用。                    |

这些设置允许您控制颜色和 alpha 通道的混合方式，从而实现复杂的图形效果。您可以选择不同的混合函数和混合因子，以控制颜色和 alpha 通道的混合方式，从而创建所需的视觉效果。通过启用或禁用混合和 ROP3，您可以更精细地控制渲染目标的输出。




### CB_BLEND_ALPHA

### CB_BLEND_BLUE

### CB_BLEND_GREEN


### CB_BLEND_RED

### CB_COLOR[0-7]_ATTRIB

CB:CB_COLOR[0-7]_ATTRIB 是一组可读写的 32 位寄存器，用于配置颜色缓冲区（COLOR surface）和相关属性的信息，其中 RT0 对应 CB:CB_COLOR0_ATTRIB，RT1 对应 CB:CB_COLOR1_ATTRIB，以此类推。这些寄存器的地址范围从 0x28c74 到 0x28e18。

以下是字段的定义：

| 字段名称               | 位范围    | 默认值 | 描述                                                                                                                                                  |
|------------------------|-----------|--------|-------------------------------------------------------------------------------------------------------------------------------------------------------|
| TILE_MODE_INDEX        | 4:0       | none   | 用于查找 GB_TILE_MODEn 寄存器以配置 COLOR 和 CMASK surface 的平铺设置的索引。                                                                     |
| FMASK_TILE_MODE_INDEX  | 9:5       | none   | 用于查找 GB_TILE_MODEn 寄存器以配置 FMASK surface 的平铺设置的索引。                                                                              |
| NUM_SAMPLES            | 14:12     | none   | 指定样本数量的对数。这个值不能大于 4（即 16 个样本）。                                                                                                   |
| NUM_FRAGMENTS          | 16:15     | none   | 指定片段数量的对数。这个值不能大于 MIN(NUM_SAMPLES, 3)，因为 log2(3) == 8 片段。                                                                    |
| FORCE_DST_ALPHA_1      | 17        | none   | 如果设置了此标志，将强制 DST_ALPHA=1.0f。适用于不具有 alpha 分量的格式。                                                                             |

CB_COLOR[0-7]_ATTRIB 寄存器用于配置与颜色缓冲区相关的属性，包括平铺模式、样本数量和片段数量等。这些属性的配置可以影响渲染的质量和性能。通过调整这些属性，可以满足不同渲染需求的要求。


### CB_COLOR[0-7]_BASE

CB:CB_COLOR[0-7]_BASE 是一组可读写的 32 位寄存器，用于配置颜色缓冲区（COLOR surface）的基地址，其中 RT0 对应 CB:CB_COLOR0_BASE，RT1 对应 CB:CB_COLOR1_BASE，以此类推。这些寄存器的地址范围从 0x28c60 到 0x28e04。

以下是字段的定义：

| 字段名称   | 位范围 | 默认值 | 描述                                                                                                                                                                                             |
|------------|--------|--------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| BASE_256B  | 31:0   | none   | 该字段的 8 位到 39 位（共 32 位）指定了设备地址空间中 COLOR surface 的起始字节地址的高位部分。                                                                                                                                                                       |
| PIPE_SWZ   | p-1:0  |        | 如果使用了管道（pipe） swizzle，则这些位（p-1:0）指定了管道 swizzle。p 的值等于 log2(numPipes)，其中 numPipes 是管道数量。                                                                                                                                        |
| BANK_SWZ   | p+b-1:p |        | 如果使用了银行（bank） swizzle，则这些位（p+b-1:p）指定了银行 swizzle。b 的值等于 log2(numBanks)，其中 numBanks 是银行数量。                                                                                                                                  |

BASE_256B 寄存器的主要作用是配置 COLOR surface 的起始地址，以指示 GPU 在设备内存中查找渲染目标的颜色数据。这些寄存器的配置可以影响渲染目标的读写行为，并可以使用 swizzle 选项进行微调，以便更好地优化内存访问。

### CB_COLOR[0-7]_CLEAR_WORD0

CB:CB_COLOR[0-7]_CLEAR_WORD0 是一组可读写的 32 位寄存器，用于配置清除颜色的低位（Bits [31:0]）数据。每个渲染目标都有对应的 CLEAR_WORD0 寄存器，其中 RT0 对应 CB:CB_COLOR0_CLEAR_WORD0，RT1 对应 CB:CB_COLOR1_CLEAR_WORD0，以此类推。这些寄存器的地址范围从 0x28c8c 到 0x28e30。

以下是字段的定义：

| 字段名称    | 位范围 | 默认值 | 描述                             |
|-------------|--------|--------|----------------------------------|
| CLEAR_WORD0 | 31:0   | none   | 清除颜色的低位数据（Bits [31:0]）。 |

CLEAR_WORD0 寄存器的作用是配置清除颜色的低位数据，这些数据用于在执行清除操作时覆盖渲染目标的颜色数据。这些寄存器的配置可以影响渲染目标的清除行为。


### CB_COLOR[0-7]_CLEAR_WORD1

CB:CB_COLOR[0-7]_CLEAR_WORD1 是一组可读写的 32 位寄存器，用于配置清除颜色的高位（Bits [63:32]）数据。每个渲染目标都有对应的 CLEAR_WORD1 寄存器，其中 RT0 对应 CB:CB_COLOR0_CLEAR_WORD1，RT1 对应 CB:CB_COLOR1_CLEAR_WORD1，以此类推。这些寄存器的地址范围从 0x28c90 到 0x28e34。

以下是字段的定义：

| 字段名称    | 位范围 | 默认值 | 描述                             |
|-------------|--------|--------|----------------------------------|
| CLEAR_WORD1 | 31:0   | none   | 清除颜色的高位数据（Bits [63:32]）。 |

CLEAR_WORD1 寄存器的作用是配置清除颜色的高位数据，这些数据用于在执行清除操作时覆盖渲染目标的颜色数据。这些寄存器的配置可以影响渲染目标的清除行为。



### CB_COLOR[0-7]_CMASK

CB:CB_COLOR[0-7]_CMASK 寄存器是一组可读写的 32 位寄存器，用于配置 CMASK（Color Mask）表面的基地址，其中 RT0 对应 CB:CB_COLOR0_CMASK，RT1 对应 CB:CB_COLOR1_CMASK，以此类推。这些寄存器的地址范围从 0x28c7c 到 0x28e20。

以下是字段的定义：

| 字段名称  | 位范围 | 默认值 | 描述                                                                                   |
|-----------|--------|--------|----------------------------------------------------------------------------------------|
| BASE_256B | 31:0   | none   | 指定设备地址空间中每个瓦片 CMASK 数据的起始字节地址的位 [39:8]。 |

CB:CB_COLOR[0-7]_CMASK 寄存器用于配置 CMASK 表面的基地址，CMASK 是一个用于渲染中的颜色蒙版数据的缓冲区。这些寄存器允许您定义每个渲染目标的 CMASK 表面的基地址，以便在渲染过程中使用 CMASK 数据来控制像素的写入。


### CB_COLOR[0-7]_CMASK_SLICE


CB:CB_COLOR[0-7]_CMASK_SLICE 寄存器是一组可读写的 32 位寄存器，用于配置 CMASK（Color Mask）表面的切片大小，其中 RT0 对应 CB:CB_COLOR0_CMASK_SLICE，RT1 对应 CB:CB_COLOR1_CMASK_SLICE，以此类推。这些寄存器的地址范围从 0x28c80 到 0x28e24。

以下是字段的定义：

| 字段名称  | 位范围 | 默认值 | 描述                                                                                   |
|-----------|--------|--------|----------------------------------------------------------------------------------------|
| TILE_MAX  | 13:0   | none   | 编码了切片的大小。该字段等于每个切片的 CMASK 数据的 128x128 块（16x16 块）的数量减 1。 |

CB:CB_COLOR[0-7]_CMASK_SLICE 寄存器用于配置 CMASK 表面的切片大小。CMASK 是一个用于渲染中的颜色蒙版数据的缓冲区。这些寄存器允许您定义每个渲染目标的 CMASK 表面的切片大小，以满足特定的渲染需求。


### CB_COLOR[0-7]_FMASK 


CB:CB_COLOR[0-7]_FMASK 寄存器是一组可读写的 32 位寄存器，用于配置 FMASK（Fragment Mask）表面的基地址，其中 RT0 对应 CB:CB_COLOR0_FMASK，RT1 对应 CB:CB_COLOR1_FMASK，以此类推。这些寄存器的地址范围从 0x28c84 到 0x28e28。

以下是字段的定义：

| 字段名称 | 位范围 | 默认值 | 描述                                                         |
|----------|--------|--------|--------------------------------------------------------------|
| BASE_256B | 31:0   | none   | 这指定了资源在设备地址空间中起始位置的字节地址的[39:8]位。您可以在此处指定管道和银行的分布。具体来说： |
|          |        |        | - 该字段的低位 [p-1:0]，其中 p = log2(numPipes)，指定了管道分布。        |
|          |        |        | - 该字段的 [p+b-1:p] 位，其中 b = log2(numBanks)，指定了银行分布。      |

CB:CB_COLOR[0-7]_FMASK 寄存器用于配置 FMASK 表面的基地址。FMASK 是一个用于遮挡和光栅化的辅助缓冲区，通常用于多重采样渲染中。这些寄存器允许您定义每个渲染目标的 FMASK 表面的基地址，以满足特定的渲染需求。其中包括管道和银行的分布设置。


### CB_COLOR[0-7]_FMASK_SLICE

CB:CB_COLOR[0-7]_FMASK_SLICE 寄存器是一组可读写的 32 位寄存器，用于配置 FMASK（Fragment Mask）表面切片的大小。类似地，对于每个渲染目标，例如 CB:CB_COLOR0_FMASK_SLICE 对应 RT0，CB:CB_COLOR1_FMASK_SLICE 对应 RT1，以此类推。这些寄存器的地址范围从 0x28c88 到 0x28e2c。

以下是字段的定义：

| 字段名称   | 位范围 | 默认值 | 描述                                                         |
|------------|--------|--------|--------------------------------------------------------------|
| TILE_MAX   | 21:0   | none   | 编码了切片的大小。该字段等于每个切片的 8x8 瓦片的数量减一。 |

CB:CB_COLOR[0-7]_FMASK_SLICE 寄存器用于配置 FMASK 表面切片的大小。FMASK 是一个用于遮挡和光栅化的辅助缓冲区，通常用于多重采样渲染中。这些寄存器允许您定义每个渲染目标的 FMASK 表面切片的大小，以满足特定的渲染需求。

### CB_COLOR_[0-7]_INFO

CB:CB_COLOR[0-7]_INFO 寄存器用于描述颜色渲染目标 (RT) 0 的表面格式信息，类似地，RT1-7 也有相似的寄存器。这些寄存器的地址范围从 0x28c70 到 0x28e14。

以下是字段的定义：

| 字段名称              | 位范围 | 默认值 | 描述                                                                                                  |
|----------------------|--------|--------|-------------------------------------------------------------------------------------------------------|
| ENDIAN               | 1:0    | none   | 指定在不同大小端模式下是否执行字节交换，字节交换相当于计算 dest[A] = src[A XOR N]，其中 A 是字节地址，N 是下面列出的 XOR 值。 |
|                      |        |        | 可能的值：                                                                                            |
|                      |        |        | 00 - ENDIAN_NONE: 无字节交换 (XOR 0)                                                                |
|                      |        |        | 01 - ENDIAN_8IN16: 在 16 位字内部进行 8 位交换 (XOR 1): 0xAABBCCDD -> 0xBBAADDCC                      |
|                      |        |        | 02 - ENDIAN_8IN32: 在 32 位字内部进行 8 位交换 (XOR 3): 0xAABBCCDD -> 0xDDCCBBAA                      |
|                      |        |        | 03 - ENDIAN_8IN64: 在 64 位字内部进行 8 位交换 (XOR 7): 0xaabbccddeeffgghh -> 0xhhggffeeddccbbaa         |
| FORMAT               | 6:2    | none   | 指定颜色分量的大小，在某些情况下还指定数字格式。请参阅下面的 COMP_SWAP 字段，了解将 RGBA（XYZW）着色器管线结果映射到像素格式中的颜色分量位置。从表面读取时，格式中缺失的分量将被默认值替代：RGB 为 0.0 或 Alpha 为 1.0。 |
|                      |        |        | 可能的值：                                                                                            |
|                      |        |        | 00 - COLOR_INVALID: 此资源已禁用                                                                      |
|                      |        |        | 01 - COLOR_8: 标准化、整数                                                                              |
|                      |        |        | 02 - COLOR_16: 标准化、整数、浮点                                                                        |
|                      |        |        | 03 - COLOR_8_8: 标准化、整数                                                                            |
|                      |        |        | 04 - COLOR_32: 整数、浮点                                                                               |
|                      |        |        | 05 - COLOR_16_16: 标准化、整数、浮点                                                                    |
|                      |        |        | 06 - COLOR_10_11_11: 仅浮点                                                                             |
|                      |        |        | 07 - COLOR_11_11_10: 仅浮点                                                                             |
|                      |        |        | 08 - COLOR_10_10_10_2: 标准化、整数                                                                    |
|                      |        |        | 09 - COLOR_2_10_10_10: 标准化、整数                                                                    |
|                      |        |        | 10 - COLOR_8_8_8_8: 标准化、整数、sRGB                                                                 |
|                      |        |        | 11 - COLOR_32_32: 整数、浮点                                                                           |
|                      |        |        | 12 - COLOR_16_16_16_16: 标准化、整数、浮点                                                               |
|                      |        |        | 13 - RESERVED                                                                                         |
|                      |        |        | 14 - COLOR_32_32_32_32: 整数、浮点                                                                     |
|                      |        |        | 15 - RESERVED                                                                                         |
|                      |        |        | 16 - COLOR_5_6_5: 仅标准化                                                                            |
|                      |        |        | 17 - COLOR_1_5_5_5: 仅标准化，1 位组件始终是 unorm                                                      |
|                      |        |        | 18 - COLOR_5_5_5_1: 仅标准化，1 位组件始终是 unorm                                                      |
|                      |        |        |

19 - COLOR_4_4_4_4: 仅标准化                                                                          |
|                      |        |        | 20 - COLOR_8_24: unorm 深度，uint 模板                                                                   |
|                      |        |        | 21 - COLOR_24_8: unorm 深度，uint 模板                                                                   |
|                      |        |        | 22 - COLOR_X24_8_32_FLOAT: 浮点深度，uint 模板                                                           |
|                      |        |        | 23 - RESERVED                                                                                         |
| LINEAR_GENERAL        | 7      | none   | 1: 覆盖 ARRAY_MODE 为 ARRAY_LINEAR_GENERAL，忽略 CB_COLOR0_ATTRIB.TILE_MODE_INDEX 设置。                |
|                      |        |        | 0: ARRAY_MODE=GB_TILE_MODE[CB_COLOR0_ATTRIB.TILE_MODE_INDEX].ARRAY_MODE                                |
| NUMBER_TYPE           | 10:8   | none   | 指定颜色分量的数字类型。                                                                               |
|                      |        |        | 可能的值：                                                                                            |
|                      |        |        | 00 - NUMBER_UNORM: 无符号重复小数 (urf)：范围 [0..1]，尺度因子 (2^n)-1                                  |
|                      |        |        | 01 - NUMBER_SNORM: Microsoft 风格的有符号小数 (rf)：范围 [-1..1]，尺度因子 (2^(n-1))-1                   |
|                      |        |        | 02 - 保留                                                                                            |
|                      |        |        | 03 - 保留                                                                                            |
|                      |        |        | 04 - NUMBER_UINT: 零扩展位域，着色器中为 int：不可混合或可过滤                                            |
|                      |        |        | 05 - NUMBER_SINT: 符号扩展位域，着色器中为 int：不可混合或可过滤                                          |
|                      |        |        | 06 - NUMBER_SRGB: 伽马校正，范围 [0..1]（仅支持 COLOR_8_8_8_8 格式；总是将颜色通道四舍五入）             |
|                      |        |        | 07 - NUMBER_FLOAT: 浮点数：32 位：IEEE 浮点，SE8M23，偏置 127，范围 (-2^129..2^129)；16 位：Short 浮点 SE5M10，偏置 15，范围 (-2^17..2^17)；11 位：打包浮点，E5M6 偏置 15，范围 [0..2^17)；10 位：打包浮点，E5M5 偏置 15，范围 [0..2^17) |
| COMP_SWAP             | 12:11  | none   | 指定如何将着色器中的红、绿、蓝和 Alpha 分量映射到渲染目标像素格式中的组件位置（0、1、2、3，其中 0 最不显著，3 最显著）。 |
|                      |        |        | 使用一个分量时，这将选择映射到单个渲染目标分量 (STD: R=>0; ALT: G=>0; STD_REV: B=>0; ALT_REV: A=>0)。 对于 2-4 个分量，SWAP_STD 始终将着色器分量映射到从 R=>0 开始的可用分量数量（分量 R=>0、G=>1、B=>2、A=>3）。 对于 2-3 个分量，SWAP_ALT 与 SWAP_STD 类似，除了来自着色器的 Alpha 总是发送到最后的渲染目标分量（2 个分量：R=>0、A=>1；3 个分量：R=>0、G=>1、A=>2）。 对于 4 个分量，SWAP_ALT 选择一种替代顺序（B=>0、G=>1、R=>2、A=>3）。 对于 2-4 个分量，SWAP_STD_REV 和 SWAP_ALT_REV 反转分量

顺序。 |
|                      |        |        | 可能的值：                                                                                            |
|                      |        |        | 00 - SWAP_STD: 标准小端组件顺序                                                                       |
|                      |        |        | 01 - SWAP_ALT: 替代组件或顺序                                                                         |
|                      |        |        | 02 - SWAP_STD_REV: 反转 SWAP_STD 顺序                                                                  |
|                      |        |        | 03 - SWAP_ALT_REV: 反转 SWAP_ALT 顺序                                                                  |
| FAST_CLEAR            | 13     | none   | 启用快速清除。如果设置，CB 识别 cmask 中的快速清除编码，并将相应的瓦片区域视为已快速清除。                       |
| COMPRESSION           | 14     | none   | 启用颜色压缩。                                                                                        |
| BLEND_CLAMP           | 15     | none   | 指定是否在混合之前将源数据夹紧到格式范围内，以及混合后夹紧。此位必须在 BLEND_BYPASS 设置为 true 时清除。否则，它必须在任何分量为 SINT/UINT（NUMBER_TYPE = SINT、UINT，或 FORMAT = COLOR_8_24、COLOR_24_8、COLOR_X24_8_32_FLOAT） 时设置。 |
| BLEND_BYPASS          | 16     | none   | 如果为 false，则该 MRT 的混合器根据 CB_BLENDn_CONTROL.ENABLE 中的指定启用/禁用。如果为 true，则禁用混合。当且仅当任何分量为 SINT/UINT（NUMBER_TYPE = SINT、UINT，或 FORMAT = COLOR_8_24、COLOR_24_8、COLOR_X24_8_32_FLOAT） 时，应设置此位。 |
| SIMPLE_FLOAT          | 17     | 0x0    | 如果设置，通过忽略特殊值（如 NaN、+/-Inf 和 -0.0f）来简化浮点处理，以使 DESTBLEND*DST=0.0f 如果 DESTBLEND==0.0f 以及 SRCBLEND*SRC=0.0f 如果 SRCBLEND==0.0f。如果为 false，则浮点处理遵循特殊值（如 NaN、+/-Inf 和 -0.0f）的完整 IEEE 规则。对于浮点表面，设置此字段可以帮助启用以下混合优化：BLEND_OPT_DONT_RD_DST、BLEND_OPT_BYPASS 和 BLEND_OPT_DISCARD_PIXEL。对于其他组件格式，将忽略此位。 |
| ROUND_MODE            | 18     | none   | 此字段选择将混合器结果转换为帧缓冲组件时的截断（浮点数的标准方式）和四舍五入之间的方式。如果任何分量为 UNORM、SNORM 或 SRGB，应将其设置为 ROUND_BY_HALF（此字段对于 COLOR_8_24 和 COLOR_24_8 被忽略）。 |
|                      |        |        | 可能的值：                                                                                            |
|                      |        |        | 00 - ROUND_BY_HALF: 加上 1/2 最低有效位，然后截断                                                      |
|                      |        |        | 01 - ROUND_TRUNCATE: 截断到浮点数零值                                                                    |
| CMASK_IS_LINEAR       | 19     | none   | 如果设置，Cmask 表面存储为线性存储。这可以减少 cmask 表面上的填充限制。                                |
| BLEND_OPT_DONT_RD_DST | 22:20  | 0x0    | 不读取 DST 的混合优化：如果混合函数计算为 SRCBLEND*SRC +/- 0*DST 并且 SRBBLEND 也不需要 DST，那么不要读取 DST。 |
|                      |        |        | 可能的值：                                                                                            |
|                      |        |        | 00 - FORCE_OPT_AUTO: （默认）硬件自动检测并启用此优化                                                     |
|                      |        |        | 01 - FORCE_OPT_DISABLE: 禁用此 RT 的优化                                                                |
|                      |        |        | 02 -

 FORCE_OPT_ENABLE_IF_SRC_A_0: 仅在 Src Alpha 为 0.0f 时启用优化                                      |
|                      |        |        | 03 - FORCE_OPT_ENABLE_IF_SRC_RGB_0: 仅在 Src 颜色分量 (RGB) 都为 0.0f 时启用优化                           |
|                      |        |        | 04 - FORCE_OPT_ENABLE_IF_SRC_ARGB_0: 仅在 Src 颜色分量 (RGB) 和 Alpha 都为 0.0f 时启用优化                   |
|                      |        |        | 05 - FORCE_OPT_ENABLE_IF_SRC_A_1: 仅在 Src Alpha 为 1.0f 时启用优化                                      |
|                      |        |        | 06 - FORCE_OPT_ENABLE_IF_SRC_RGB_1: 仅在 Src 颜色分量 (RGB) 都为 1.0f 时启用优化                           |
|                      |        |        | 07 - FORCE_OPT_ENABLE_IF_SRC_ARGB_1: 仅在 Src 颜色分量 (RGB) 和 Alpha 都为 1.0f 时启用优化                   |
| BLEND_OPT_DISCARD_PIXEL| 25:23  | 0x0    | 丢弃像素的混合优化：如果混合函数计算为 0*SRC +/- 1*DST，则这将成为 NOP。                                 |
|                      |        |        | 可能的值：                                                                                            |
|                      |        |        | 00 - FORCE_OPT_AUTO: （默认）硬件自动检测并启用此优化                                                     |
|                      |        |        | 01 - FORCE_OPT_DISABLE: 禁用此 RT 的优化                                                                |
|                      |        |        | 02 - FORCE_OPT_ENABLE_IF_SRC_A_0: 仅在 Src Alpha 为 0.0f 时启用优化                                      |
|                      |        |        | 03 - FORCE_OPT_ENABLE_IF_SRC_RGB_0: 仅在 Src 颜色分量 (RGB) 都为 0.0f 时启用优化                           |
|                      |        |        | 04 - FORCE_OPT_ENABLE_IF_SRC_ARGB_0: 仅在 Src 颜色分量 (RGB) 和 Alpha 都为 0.0f 时启用优化                   |
|                      |        |        | 05 - FORCE_OPT_ENABLE_IF_SRC_A_1: 仅在 Src Alpha 为 1.0f 时启用优化                                      |
|                      |        |        | 06 - FORCE_OPT_ENABLE_IF_SRC_RGB_1: 仅在 Src 颜色分量 (RGB) 都为 1.0f 时启用优化                           |
|                      |        |        | 07 - FORCE_OPT_ENABLE_IF_SRC_ARGB_1: 仅在 Src 颜色分量 (RGB) 和 Alpha 都为 1.0f 时启用优化                   |

这些字段允许配置颜色渲染目标的各种特性，包括颜色格式、字节顺序、数字类型、混合和优化选项等。这些设置可根据渲染需求和硬件支持进行调整，以获得最佳性能和效果。

### CB_COLOR[0-7]_PITCH

CB:CB_COLOR[0-7]_PITCH 寄存器是一组可读写的 32 位寄存器，用于控制渲染目标颜色缓冲区的扫描线间距（Pitch）。每个寄存器对应一个渲染目标，例如，CB:CB_COLOR0_PITCH 对应 RT0，CB:CB_COLOR1_PITCH 对应 RT1，以此类推。这些寄存器的地址范围从 0x28c64 到 0x28e08。

以下是字段的定义：

| 字段名称   | 位范围 | 默认值 | 描述                                                                                                    |
|------------|--------|--------|---------------------------------------------------------------------------------------------------------|
| TILE_MAX   | 10:0   | none   | 编码了扫描线的间距（Pitch）；如果 Pitch 是每个扫描线的数据元素数，那么这个字段是 (Pitch / 8) - 1，表示X维度上允许的最大 8x8 瓦片编号。 |

CB:CB_COLOR[0-7]_PITCH 寄存器用于配置渲染目标颜色缓冲区的扫描线间距，以便在渲染过程中正确处理像素数据的存储。不同的渲染目标可以具有不同的扫描线间距，以满足渲染需求。



### CB_COLOR[0-7]_SLICE

CB:CB_COLOR[0-7]_SLICE 寄存器是一组可读写的 32 位寄存器，用于控制渲染目标颜色缓冲区的切片大小（Slice）。每个寄存器对应一个渲染目标，例如，CB:CB_COLOR0_SLICE 对应 RT0，CB:CB_COLOR1_SLICE 对应 RT1，以此类推。这些寄存器的地址范围从 0x28c68 到 0x28e0c。

以下是字段的定义：

| 字段名称   | 位范围 | 默认值 | 描述                                                                                                    |
|------------|--------|--------|---------------------------------------------------------------------------------------------------------|
| TILE_MAX   | 21:0   | none   | 编码了切片的大小。如果 SliceTiles 是一个切片中的最大瓦片数（等于 Pitch * Height / 64），那么这个字段是 SliceTiles - 1，表示允许的最大切片中的瓦片编号。 |

CB:CB_COLOR[0-7]_SLICE 寄存器用于配置渲染目标颜色缓冲区的切片大小，以便在渲染过程中正确管理渲染目标的数据存储。不同的渲染目标可以具有不同的切片大小，以满足渲染需求。


### CB_COLOR[0-7]_VIEW


CB:CB_COLOR[0-7]_VIEW 寄存器是一组可读写的 32 位寄存器，用于选择渲染目标的切片索引范围。每个寄存器对应一个渲染目标，例如，CB:CB_COLOR0_VIEW 对应 RT0，CB:CB_COLOR1_VIEW 对应 RT1，以此类推。这些寄存器的地址范围从 0x28c6c 到 0x28e10。

以下是字段的定义：

| 字段名称     | 位范围 | 默认值 | 描述                                                                                                                                                                               |
|--------------|--------|--------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| SLICE_START  | 10:0   | none   | 对于 ARRAY_LINEAR_GENERAL：此字段的[7:0]位指定资源的字节地址的[7:0]位。与 CB_COLOR*_BASE.BASE_256B 一起使用，指定 40 位起始地址。地址必须是元素对齐的。使用 ARRAY_LINEAR_GENERAL 时，由于 SLICE_START 没有实际值，因此在进行 rtindex（切片）夹紧时，假定 SLICE_START 值为零。对于所有其他表面，此字段指定了此视图的起始切片编号：此字段添加到 rtindex 以计算要呈现的切片。 |
| SLICE_MAX    | 23:13  | none   | 指定此资源的允许的最大渲染目标切片索引（rtindex），这比总切片数少一个。如果超过此值，rtindex 将被夹紧到 SLICE_START。                                                                                                                  |

CB:CB_COLOR[0-7]_VIEW 寄存器用于选择要呈现的渲染目标的切片索引范围。这些寄存器允许配置渲染目标的子区域以实现高级渲染技术，例如多视图渲染或渲染到纹理数组的特定切片。




### CB:CB_COLOR_CONTROL 


CB:CB_COLOR_CONTROL 是一个可读写的 32 位寄存器，用于控制通用的颜色缓冲（CB）行为，适用于所有多渲染目标（MRTs）。以下是字段的定义：

| 字段名称         | 位范围 | 默认值 | 描述                                                                                   |
|------------------|--------|--------|----------------------------------------------------------------------------------------|
| DEGAMMA_ENABLE   | 3      | none   | 如果为真，则每个 UNORM 格式的 COLOR_8_8_8_8 或 COLOR_8 MRT 将被视为 SRGB 格式。这会影响正常绘制和解析操作。此位存在是为了与旧架构兼容，旧架构不具有 SRGB 数字类型。 |
| MODE             | 6:4    | none   | 该字段选择标准颜色处理或几种主要操作模式。                                      |
|                  |        |        | 可能的值：                                                                           |
|                  |        |        | 00 - CB_DISABLE: 禁用对颜色缓冲的绘制。导致 DB 不会将瓦片/四块发送到 CB。CB 本身会忽略此字段。               |
|                  |        |        | 01 - CB_NORMAL: 正常渲染模式。DB 应该发送用于像素导出的瓦片和四块。                                   |
|                  |        |        | 02 - CB_ELIMINATE_FAST_CLEAR: 使用清除颜色填充已经快速清除的颜色表面位置。DB 应只发送瓦片。                   |
|                  |        |        | 03 - CB_RESOLVE: 从 MRT0 读取，对所有样本求平均，并写入 MRT1，它是单样本的。DB 应该只发送瓦片。               |
|                  |        |        | 04 - 保留                                                                                 |
|                  |        |        | 05 - CB_FMASK_DECOMPRESS: 解压 FMASK 缓冲区以获得可读取的纹理格式。在执行此操作之前不需要 CB_ELIMINATE_FAST_CLEAR 通道。DB 应只发送瓦片。 |
| ROP3             | 23:16  | none   | 此字段支持 28 个布尔操作，将源和目标或刷子和目标组合在一起，其中刷子由着色器提供，以替换源。代码 0xCC (11001100) 将源复制到目标，从而禁用 ROP 功能。如果任何 MRT 启用混合，必须禁用 ROP。 |
|                  |        |        | 可能的值：                                                                           |
|                  |        |        | 00 - 0x00: BLACKNESS                                                                     |
|                  |        |        | 05 - 0x05                                                                                |
|                  |        |        | 10 - 0x0A                                                                                |
|                  |        |        | 15 - 0x0F                                                                                |
|                  |        |        | 17 - 0x11: NOTSRCERASE                                                                   |
|                  |        |        | 34 - 0x22                                                                                |
|                  |        |        | 51 - 0x33: NOTSRCCOPY                                                                    |
|                  |        |        | 68 - 0x44: SRCERASE                                                                      |
|                  |        |        | 80 - 0x50                                                                                |
|                  |        |        | 85 - 0x55: DSTINVERT                                                                     |
|                  |        |        | 90 - 0x5A: PATINVERT                                                                     |
|                  |        |        | 95 - 0x5F                                                                                |
|                  |        |        | 102 - 0x66: SRCINVERT                                                                    |
|                  |        |        | 119 - 0x77                                                                               |
|                  |        |        | 136 - 0x88: SRCAND                                                                       |
|                  |        |        | 153 - 0x99                                                                               |
|                  |        |        | 160 - 0xA0                                                                               |
|                  |        |        | 165 - 0xA5                                                                               |
|                  |        |        | 170 - 0xAA                                                                               |
|                  |        |        | 175 - 0xAF                                                                               |
|                  |        |        | 187 - 0xBB: MERGEPAINT                                                                    |
|                  |        |        | 204 - 0xCC: SRCCOPY                                                                      |
|                  |        |        | 221 - 0xDD                                                                               |
|                  |        |        | 238 - 0xEE: SRCPAINT                                                                     |
|                  |        |        | 240 - 0xF0: PATCOPY                                                                      |
|                  |        |        | 245 - 0xF5                                                                               |
|                  |        |        |

250 - 0xFA                                                                               |
|                  |        |        | 255 - 0xFF: WHITENESS                                                                    |

CB:CB_COLOR_CONTROL 寄存器用于控制颜色缓冲的行为，包括颜色处理模式、混合操作和其他相关设置。可以根据应用程序的需要配置这些字段，以影响渲染的结果。

### CB_SHADER_MASK


### CB_TARGET_MASK

CB:CB_TARGET_MASK 寄存器是一个可读写的 32 位寄存器，用于控制写入多渲染目标 (MRTs) 的颜色组件掩码。该寄存器的地址为 0x28238。

以下是字段的定义：

| 字段名称         | 位范围 | 默认值 | 描述                                                                                                                |
|-----------------|--------|--------|---------------------------------------------------------------------------------------------------------------------|
| TARGET0_ENABLE  | 3:0    | none   | 启用对 RT 0 组件的写入。低位对应红色通道。0 位禁止对该通道的写入，1 位启用对该通道的写入。                          |
| TARGET1_ENABLE  | 7:4    | none   | 启用对 RT 1 组件的写入。                                                                                           |
| TARGET2_ENABLE  | 11:8   | none   | 启用对 RT 2 组件的写入。                                                                                           |
| TARGET3_ENABLE  | 15:12  | none   | 启用对 RT 3 组件的写入。                                                                                           |
| TARGET4_ENABLE  | 19:16  | none   | 启用对 RT 4 组件的写入。                                                                                           |
| TARGET5_ENABLE  | 23:20  | none   | 启用对 RT 5 组件的写入。                                                                                           |
| TARGET6_ENABLE  | 27:24  | none   | 启用对 RT 6 组件的写入。                                                                                           |
| TARGET7_ENABLE  | 31:28  | none   | 启用对 RT 7 组件的写入。                                                                                           |

CB:CB_TARGET_MASK 寄存器允许控制多渲染目标的颜色组件写入，通过配置每个组件的使能位来选择是否写入对应的渲染目标。这些掩码字段使您可以选择哪些颜色通道会被写入到每个渲染目标中。




## 分析case中的寄存器配置

从IB 列表可以看出 ## 首先编译完成shader之后关于shader的状态如下图,今天主要分析了产生过程。

```
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

```
## 流程

```mermaid
graph TD
si_compiler_shader --> si_init_shader_ctx --> si_llvm_context_set_tgsi 
--> si_compiler_tgsi_main --> si_compile_llvm 
si_compile_llvm --> si_shader_binary_read_config

```
## 主要函数

```c
void ac_shader_binary_read_config(struct ac_shader_binary *binary,
                                  struct ac_shader_config *conf,
                                  unsigned symbol_offset,
                                  bool supports_spill)
{
    unsigned i;
    const unsigned char *config =
        ac_shader_binary_config_start(binary, symbol_offset);
    bool really_needs_scratch = false;
    uint32_t wavesize = 0;

    // 检查是否真的需要scratch缓冲区，根据是否支持spill来判断
    if (supports_spill) {
        really_needs_scratch = true;
    } else {
        // 如果不支持spill，则检查是否有关于scratch资源的符号重定位信息
        for (i = 0; i < binary->reloc_count; i++) {
            const struct ac_shader_reloc *reloc = &binary->relocs[i];

            // 通过比较符号的名字来确定是否需要scratch缓冲区
            if (!strcmp(scratch_rsrc_dword0_symbol, reloc->name) ||
                !strcmp(scratch_rsrc_dword1_symbol, reloc->name)) {
                really_needs_scratch = true;
                break;
            }
        }
    }

    // 从二进制的配置数据中读取寄存器值并填充到ac_shader_config结构体中
    for (i = 0; i < binary->config_size_per_symbol; i += 8) {
        unsigned reg = util_le32_to_cpu(*(uint32_t*)(config + i));
        unsigned value = util_le32_to_cpu(*(uint32_t*)(config + i + 4));
        switch (reg) {
        case R_00B028_SPI_SHADER_PGM_RSRC1_PS:
        case R_00B128_SPI_SHADER_PGM_RSRC1_VS:
        case R_00B228_SPI_SHADER_PGM_RSRC1_GS:
        case R_00B848_COMPUTE_PGM_RSRC1:
        case R_00B428_SPI_SHADER_PGM_RSRC1_HS:
            // 读取SGPRs和VGPRs数量，并存储到ac_shader_config结构体中
            conf->num_sgprs = MAX2(conf->num_sgprs, (G_00B028_SGPRS(value) + 1) * 8);
            conf->num_vgprs = MAX2(conf->num_vgprs, (G_00B028_VGPRS(value) + 1) * 4);
            conf->float_mode =  G_00B028_FLOAT_MODE(value);
            break;
        case R_00B02C_SPI_SHADER_PGM_RSRC2_PS:
            // 读取LDS（Local Data Share）大小，并存储到ac_shader_config结构体中
            conf->lds_size = MAX2(conf->lds_size, G_00B02C_EXTRA_LDS_SIZE(value));
            break;
        case R_00B84C_COMPUTE_PGM_RSRC2:
            // 读取LDS大小（Compute模式），并存储到ac_shader_config结构体中
            conf->lds_size = MAX2(conf->lds_size, G_00B84C_LDS_SIZE(value));
            break;
        case R_0286CC_SPI_PS_INPUT_ENA:
            // 读取SPI_PS_INPUT_ENA寄存器值，并存储到ac_shader_config结构体中
            conf->spi_ps_input_ena = value;
            break;
        case R_0286D0_SPI_PS_INPUT_ADDR:
            // 读取SPI_PS_INPUT_ADDR寄存器值，并存储到ac_shader_config结构体中
            conf->spi_ps_input_addr = value;
            break;
        case R_0286E8_SPI_TMPRING_SIZE:
        case R_00B860_COMPUTE_TMPRING_SIZE:
            // 读取WAVESIZE，并存储到ac_shader_config结构体中
            // WAVESIZE是以256个DWORD为单位的
            wavesize = value;
            break;
        case SPILLED_SGPRS:
            // 读取被spill的SGPRs数量，并存储到ac_shader_config结构体中
            conf->spilled_sgprs = value;
            break;
        case SPILLED_VGPRS:
            // 读取被spill的VGPRs数量，并存储到ac_shader_config结构体中
            conf->spilled_vgprs = value;
            break;
        default:
            // 处理未知的配置寄存器，输出警告信息
            {
                static bool printed;

                if (!printed) {
                    fprintf(stderr, "Warning: LLVM emitted unknown "
                        "config register: 0x%x\n", reg);
                    printed = true;
                }
            }
            break;
        }

        // 设置SPI_PS_INPUT_ADDR为SPI_PS_INPUT_ENA（如果未设置）
        if (!conf->spi_ps_input_addr)
            conf->spi_ps_input_addr = conf->spi_ps_input_ena;
    }

    // 如果真的需要scratch缓冲区，则计算每个wave所需的scratch缓冲区大小，并存储到ac_shader_config结构体中
    if (really_needs_scratch) {
        // WAVESIZE以256个DWORD为单位，计算scratch_bytes_per_wave
        conf->scratch_bytes_per_wave = G_00B860_WAVESIZE(wavesize) * 256 * 4;
    }
}



```

## RadeonSI中未使用的寄存器

### VGT

* VGT_HOS_REUSE_DEPTH
* VGT_IMMED_DATA
* VGT_OUTPUT_PATH_CNTL
* VGT_STRMOUT_BUFFER_FILLED_SIZE_0
* VGT_STRMOUT_BUFFER_FILLED_SIZE_1
* VGT_STRMOUT_BUFFER_FILLED_SIZE_2
* VGT_STRMOUT_BUFFER_FILLED_SIZE_3

* VGT_STRMOUT_BUFFER_OFFSET_0
* VGT_STRMOUT_BUFFER_OFFSET_1
* VGT_STRMOUT_BUFFER_OFFSET_2
* VGT_STRMOUT_BUFFER_OFFSET_3

* VGT_STRMOUT_VTX_STRIDE_0
* VGT_STRMOUT_VTX_STRIDE_1
* VGT_STRMOUT_VTX_STRIDE_2
* VGT_STRMOUT_VTX_STRIDE_2

* VGT_VTX_VECT_EJECT_REG

### PA 

* PA_CL_GB_HORZ_CLIP_ADJ
* PA_CL_GB_VERT_DISC_ADJ

* PA_CL_POINT_CULL_RAD
* PA_CL_POINT_SIZE
* PA_CL_POINT_X_RAD
* PA_CL_POINT_Y_RAD
* PA_CL_UCP_[0-5]_W
* PA_CL_UCP_[0-5]_X 
* PA_CL_UCP_[0-5]_Y
* PA_CL_UCP_[0-5]_Z

* PA_CL_VPORT_XOFFSET_[0-15]
* PA_CL_VPORT_XSCALE_[0-15]
* PA_CL_VPORT_YOFFSET_[0-15] 
* PA_CL_VPORT_YSCALE_[0-15]
* PA_CL_VPORT_ZOFFSET_[0-15]
* PA_CL_VPORT_ZSCALE_[0-15]

* PA_SC_AA_CONFIG
* PA_SC_AA_MASK_X0Y1_X1Y1

* PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_1
* PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_2
* PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_3

* PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_1
* PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_2
* PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_3




## Reference 

* [southern-islands-instruction-set-architecture.pdf](https://www.amd.com/system/files/TechDocs/southern-islands-instruction-set-architecture.pdf)

* [R6xx_R7xx_3D.pdf](https://www.x.org/docs/AMD/old/R6xx_R7xx_3D.pdf)

* [si_programming_guide_v2.pdf](https://www.x.org/docs/AMD/old/si_programming_guide_v2.pdf)

* [SI_3D_registers.pdf](https://www.x.org/docs/AMD/old/SI_3D_registers.pdf)

