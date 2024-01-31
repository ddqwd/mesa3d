
# OpenCL 资料

https://www.nersc.gov/assets/pubs_presos/MattsonTutorialSC14.pdf

https://registry.khronos.org/OpenCL/specs/opencl-1.1.pdf


# OpenCL 1.1 规范

## 1. 介绍

现代处理器架构已经将并行性视为提高性能的重要途径。面对在固定功耗范围内提高时钟速度的技术挑战，中央处理单元（CPU）通过增加多个核心来改善性能。图形处理单元（GPU）也从固定功能的渲染设备演变为可编程的并行处理器。由于当今计算机系统通常包括高度并行的CPU、GPU和其他类型的处理器，使软件开发人员充分利用这些异构处理平台变得非常重要。
为异构并行处理平台创建应用程序具有挑战性，因为用于多核CPU和GPU的传统编程方法非常不同。基于CPU的并行编程模型通常基于标准，但通常假定共享地址空间并且不包括矢量操作。通用GPU编程模型涉及复杂的内存层次结构和矢量操作，但通常是面向平台、供应商或硬件特定的。这些限制使得开发人员难以从单一的、多平台的源代码库中访问异构CPU、GPU和其他处理器的计算能力。现在比以往任何时候都更需要使软件开发人员能够有效地充分利用包括异构CPU、GPU和其他处理器（如DSP和Cell/B.E.处理器）在内的高性能计算服务器、桌面计算机系统和手持设备等多样化的并行处理平台。
OpenCL（开放计算语言）是一个面向CPU、GPU和其他处理器的通用并行编程的开放免费标准，为软件开发人员提供了在这些异构处理平台上获得可移植和高效访问的能力。
OpenCL支持各种应用程序，从嵌入式和消费者软件到高性能计算解决方案，通过一个低级、高性能、可移植的抽象。通过创建一个高效、接近硬件的编程接口，OpenCL将成为一个平台独立的工具、中间件和应用程序的并行计算生态系统的基础层。
OpenCL特别适用于在新兴的交互式图形应用程序中发挥越来越重要的作用，这些应用程序将通用并行计算算法与图形渲染流水线相结合。
OpenCL包括一个用于协调异构处理器之间的并行计算的API；以及一个具有明确定义计算环境的跨平台编程语言。OpenCL标准：
- 支持基于数据和任务的并行编程模型
- 利用 ISO C99 的子集，带有并行扩展
- 定义基于 IEEE 754 的一致数值要求
- 定义了面向手持设备和嵌入式设备的配置文件
- 与OpenGL、OpenGL ES和其他图形API高效互操作
- 最后修订日期：2011年6月1日，第13页
本文档首先概述了基本概念和OpenCL的架构，然后详细描述了其执行模型、内存模型和同步支持。然后讨论了OpenCL平台和运行时API，并详细介绍了OpenCL C编程语言。提供了一些示例，描述了示例计算用例及其在OpenCL中的编写方式。该规范分为核心规范，任何符合OpenCL的实现必须支持；手持/嵌入式配置文件，放宽了面向手持设备和嵌入式设备的OpenCL兼容要求；以及一组可能在后续OpenCL规范的修订中进入核心规范的可选扩展。

## 2. 术语表

**Application（应用程序）:** 主机上运行的程序与OpenCL设备的组合。

**Blocking and Non-Blocking Enqueue API calls（阻塞和非阻塞入队API调用）:** 非阻塞的入队API调用将命令放置在命令队列上并立即返回到主机。阻塞模式的入队API调用在命令完成之前不会返回到主机。

**Barrier（屏障）:** 有两种类型的屏障 - 命令队列屏障和工作组屏障。OpenCL API提供了一个函数，用于入队命令队列屏障命令。此屏障命令确保在命令队列中排队的所有先前入队的命令在可以开始执行命令队列中排队的任何后续命令之前已经执行完成。在OpenCL C编程语言中，有一个内置的工作组屏障函数，用于在设备上执行的内核中执行工作组内的工作项之间的同步。

**Buffer Object（缓冲对象）:** 存储线性字节集合的内存对象，通过在设备上执行的内核中的指针可以访问。可以使用OpenCL API调用由主机操作的缓冲区对象。缓冲区对象封装了大小（字节数）、描述用法信息和要从中分配的区域的属性以及缓冲区数据。

**Command（命令）:** 提交到命令队列以执行的OpenCL操作。例如，OpenCL命令可用于在计算设备上执行内核、操作内存对象等。

**Command-queue（命令队列）:** 保存将在特定设备上执行的命令的对象。命令队列在上下文中在特定设备上创建。命令队列中的命令按顺序排队，但可以按顺序或无序执行。有关“顺序执行”和“无序执行”，请参阅相应部分。

**Command-queue Barrier（命令队列屏障）:** 参见“屏障”。

**Compute Device Memory（计算设备内存）:** 指连接到计算设备的一个或多个内存。

**Compute Unit（计算单元）:** 一个OpenCL设备有一个或多个计算单元。一个工作组在单个计算单元上执行。计算单元由一个或多个处理元素和本地内存组成。计算单元还可能包括专用的纹理过滤单元，可以由其处理元素访问。

**Concurrency（并发性）:** 系统中一组任务可以保持活动并同时取得进展的属性。要在运行程序时利用并发执行，程序员必须在其问题中识别并发性，在源代码中暴露它，然后使用支持并发性的符号进行利用。

**Constant Memory（常量内存）:** 内核执行期间保持不变的全局内存区域。主机分配并初始化放入常量内存的内存对象。

**Context（上下文）:** 内核执行的环境以及其中定义同步和内存管理的域。上下文包括一组设备，这些设备可以访问的内存，相应的内存属性以及用于调度内核或对内存对象执行操作的一个或多个命令队列。

**Data Parallel Programming Model（数据并行编程模型）:** 在OpenCL中，泛指一种模型，其中单个程序的一组指令被并发地应用于抽象域内的每个点。

**Device（设备）:** 由计算单元组成的设备。命令队列用于将命令排队到设备。OpenCL设备通常对应于GPU、多核CPU以及其他处理器，如DSP和Cell/B.E.处理器。

**Event Object（事件对象）:** 事件对象封装了操作（如命令）的状态。它可用于同步上下文中的操作。

**Event Wait List（事件等待列表）:** 事件等待列表是一个事件对象列表，可用于控制特定命令何时开始执行。

**Framework（框架）:** 包含用于支持软件开发和执行的组件集的软件系统。框架通常包括库、API、运行时系统、编译器等。

**Global ID（全局ID）:** 用于唯一标识一个工作项的全局ID，源自在执行内核时指定的全局工作项数。全局ID是一个N维值，从（0, 0, … 0）开始。另见“Local ID”。

**Global Memory（全局内存）:** 上下文中所有工作项都可以访问的内存区域。它可通过命令，如读取、写入和映射，由主机访问。

**GL Share Group（GL共享组）:** GL共享组对象管理共享的OpenGL或OpenGL ES资源，如纹理、缓冲区、帧缓冲区和渲染缓冲区，并与一个或多个GL上下文对象相关联。GL共享组通常是一个不可见的对象，不能直接访问。

**Handle（句柄）:** 引用由OpenCL分配的对象的不透明类型。任何对对象的操作都是通过引用该对象的句柄进行的。

**Host（主机）:** 主机通过OpenCL API与上下文进行交互。

**Host Pointer（主机指针）:** 指向主机虚拟地址空间中的内存的指针。

**Illegal（非法）:** 系统明确不允许的行为，当在OpenCL中遇到时将报告为错误。

**Image Object（图像对象）:** 存储二

维或三维结构化数组的内存对象。只能使用读取和写入函数访问图像数据。读取函数使用采样器。图像对象封装了尺寸、描述图像中每个元素的属性、描述用法信息和要从中分配的区域的属性以及图像数据。

**Implementation Defined（实现定义）:** 明确允许在OpenCL符合实现之间变化的行为。OpenCL实施者必须记录实现定义的行为。

**In-order Execution（顺序执行）:** 在OpenCL中的执行模型，其中命令队列中的命令按照提交的顺序依次执行，每个命令在下一个命令开始之前执行完成。参见“Out-of-order Execution”。

**Kernel（内核）:** 在程序中声明并在OpenCL设备上执行的函数。内核通过应用于程序中定义的任何函数的__kernel限定符来标识。

**Kernel Object（内核对象）:** 内核对象封装了程序中声明的特定__kernel函数以及在执行此__kernel函数时要使用的参数值。

**Local ID（本地ID）:** 在执行内核的工作组内唯一的工作项ID。本地ID是一个N维值，从（0, 0, … 0）开始。另见“Global ID”。

**Local Memory (本地内存):** 与工作组相关联的内存区域，仅由属于该工作组的工作项访问。

**Marker (标记):** 在命令队列中排队的命令，可用于标记在标记之前排队的所有命令。标记命令返回一个事件，应用程序可以使用该事件排队对标记事件的等待，即等待在标记命令之前排队的所有命令完成。

**Memory Objects (内存对象):** 内存对象是对全局内存的引用计数区域的句柄。参见Buffer Object（缓冲对象）和Image Object（图像对象）。

**Memory Regions (or Pools) (内存区域（或池）):** 在OpenCL中的不同地址空间。内存区域可能在物理内存中重叠，尽管OpenCL将其视为逻辑上不同的。内存区域被表示为private（私有）、local（局部）、constant（常量）和global（全局）。

**Object (对象):** 对资源的抽象表示，可通过OpenCL API操作。示例包括程序对象、内核对象和内存对象。

**Out-of-Order Execution (无序执行):** 在该执行模型中，放置在工作队列中的命令可以按照与事件等待列表和命令队列屏障强加的约束一致的任何顺序开始和完成执行。参见In-order Execution（顺序执行）。

**Platform (平台):** 主机加上由OpenCL框架管理的一组设备，允许应用程序共享资源并在平台上的设备上执行内核。

**Private Memory (私有内存):** 对工作项私有的内存区域。在一个工作项的私有内存中定义的变量对另一个工作项不可见。

**Processing Element (处理元素):** 一个虚拟标量处理器。一个工作项可能在一个或多个处理元素上执行。

**Program (程序):** 一个OpenCL程序由一组内核组成。程序还可以包含由__kernel函数调用的辅助函数和常量数据。

**Program Object (程序对象):** 程序对象封装了以下信息：
- 与关联上下文的引用。
- 程序源代码或二进制代码。
- 最近成功构建的程序可执行文件、为其构建的设备列表、使用的构建选项以及构建日志。
- 当前附加的内核对象数。

**Reference Count (引用计数):** OpenCL对象的生命周期由其引用计数确定 - 对该对象的引用数量的内部计数。创建OpenCL对象时，其引用计数设置为1。对适当的retain API的后续调用（如clRetainContext、clRetainCommandQueue）会增加引用计数。对适当的release API的调用（如clReleaseContext、clReleaseCommandQueue）会减少引用计数。引用计数达到零后，OpenCL将释放对象的资源。

**Relaxed Consistency (松散一致性):** 一种内存一致性模型，在不同工作项或命令可见的内存内容在屏障或其他显式同步点除外可能是不同的。

**Resource (资源):** 由OpenCL定义的对象类别。资源的一个实例是一个对象。最常见的资源包括上下文、命令队列、程序对象、内核对象和内存对象。计算资源是参与推进程序计数器的硬件元素。示例包括主机、设备、计算单元和处理元素。

**Retain, Release (保留，释放):** 使用OpenCL对象时增加（保留）和减少（释放）引用计数的操作。这是一种簿记功能，以确保在所有使用该对象的实例完成之前，系统不会删除该对象。参见Reference Count（引用计数）。

**Sampler (采样器):** 描述在内核中读取图像时如何对图像进行采样的对象。图像读取函数将采样器作为参数。采样器指定图像寻址模式，即如何处理超出范围的图像坐标，滤波模式以及输入图像坐标是规格化还是非规格化值。

**SIMD (单指令多数据):** 一种编程模型，其中一个内核在多个处理元素上同时执行，每个处理元素都有自己的数据和共享的程序计数器。所有处理元素执行严格相同的一组指令。

**SPMD (单程序多数据):** 一种编程模型，其中一个内核在多个处理元素上同时执行，每个处理元素都有自己的数据和自己的程序计数器。因此，虽然所有计算资源运行相同的内核，但它们保持自己的指令计数器，并且由于内核中的分支，实际的指令序列在处理元素集合中可能相当不同。

**Task Parallel Programming Model (任务并行编程模型):** 一种编程模型，其中计算以多个并发任务的形式表达，其中任务是在单个大小为一的工作组中执行的内核。并发任务可以运行不同的内核。

**Thread-safe (线程安全):** 如果由OpenCL管理的内部状态在同时由多个主机线程调用时保持一致，那么OpenCL API调用被认为是线程安全的。OpenCL API调用是线程安全的，允许应用程序在多个主机线程中调用这些函数而无需在这些主机线程之间实施互斥，即它们也是可重入安全的。

**Undefined (未定义):** OpenCL API调用、内核中使用的内置函数或内核的执行行为明确未被OpenCL定义的情况。符合规范的实现无需规定在OpenCL中遇到未定义构造时会发生什么。

**Work-group (工作组):** 一组相关的工作项，它们在单个计算单元上执行。组中的工作项执行相同的内核，并共享本地内存和工作组屏障。

**Work-group Barrier (工作组屏障). See Barrier (屏障).**

**Work-item (工作项):** 由命令在设备上调用的内核的一组并行执行之一。一个工作项由一个或多个处理元素执行，作为在计算单元上执行的工作组的一部分。工作项通过其全局ID和本地ID与集合中的其他执行区分开。

### **2.1 OpenCL 类图**

图2.1使用统一建模语言1（UML）符号描述了OpenCL规范的类图。该图显示了节点和边，它们分别是类和它们之间的关系。为简化起见，它只显示了类，没有显示属性或操作。抽象类用“{abstract}”进行注释。至于关系，它显示了聚合（用实心菱形注释）、关联（无注释）和继承（用开放箭头注释）。关系的基数显示在每个末端。关系的基数“*”表示“多”，基数“1”表示“仅一个”，基数“0..1”表示“可选一个”，基数“1..*”表示“一个或多个”。关系的可导航性使用常规箭头表示。

## **3. OpenCL 架构**

OpenCL是一种用于编程异构的CPU、GPU和其他离散计算设备组成的单一平台的开放行业标准。它不仅仅是一种语言，还是一个用于并行编程的框架，包括一种语言、API、库和运行时系统以支持软件开发。例如，使用OpenCL，程序员可以编写在GPU上执行的通用程序，而无需将其算法映射到3D图形API，如OpenGL或DirectX。

OpenCL的目标是希望编写可移植且高效代码的专业程序员。这包括库编写者、中间件供应商和性能导向的应用程序程序员。因此，OpenCL提供了低级硬件抽象以及支持编程和底层硬件许多细节的框架。

为了描述OpenCL背后的核心思想，我们将使用一系列模型：
1. **平台模型**
2. **内存模型**
3. **执行模型**
4. **编程模型**

### **3.1 平台模型**

OpenCL的平台模型定义在图3.1中。该模型由一个主机连接到一个或多个OpenCL设备组成。一个OpenCL设备分为一个或多个计算单元（CUs），这些计算单元进一步分为一个或多个处理单元（PEs）。设备上的计算发生在处理单元内。

一个OpenCL应用程序在主机上运行，遵循主机平台本地的模型。OpenCL应用程序从主机向设备内的处理单元提交命令以执行计算。计算单元内的处理单元可以作为SIMD单元（以单一指令流的方式同步执行）或作为SPMD单元（每个PE维护自己的程序计数器）执行单一指令流。

#### **3.1.1 平台混合版本支持**

OpenCL旨在支持在单个平台下具有不同能力的设备。这包括符合OpenCL规范不同版本的设备。对于OpenCL系统，有三个重要的版本标识符需要考虑：平台版本、设备版本和设备上支持的OpenCL C语言版本（可能有多个）。

平台版本表示支持的OpenCL运行时的版本。这包括主机可用于与OpenCL运行时交互的所有API，如上下文、内存对象、设备和命令队列。

设备版本表示设备的能力，独立于运行时和编译器，由clGetDeviceInfo返回的设备信息表示。与设备版本相关联的属性示例包括资源限制和扩展功能。返回的版本对应于设备符合的OpenCL规范的最高版本，但不高于平台版本。

设备的语言版本表示开发人员可以假定在给定设备上支持的OpenCL编程语言特性。报告的版本是支持的语言的最高版本。

OpenCL C被设计为向后兼容，因此要考虑设备是否符合规范，不需要支持多个语言版本。如果支持多个语言版本，则编译器默认使用设备支持的最高语言版本。语言版本不高于平台版本，但可能超过设备版本（参见第5.6.3.5节）。

### **3.2 执行模型**

OpenCL程序的执行分为两部分：在一个或多个OpenCL设备上执行的内核和在主机上执行的主机程序。主机程序定义了内核的上下文并管理它们的执行。

OpenCL执行模型的核心是内核的执行方式。当主机提交内核以由主机执行时，将定义一个索引空间。内核的每个点在此索引空间中执行一个实例。这个内核实例称为一个工作项，其通过其在索引空间中的点进行标识，为工作项提供了一个全局ID。每个工作项执行相同的代码，但代码中的具体执行路径和操作的数据可能因工作项而异。

工作项被组织成工作组。工作组提供了对索引空间更粗粒度的分解。工作组被分配一个唯一的工作组ID，其维度与用于工作项的索引空间相同。工作项被分配到一个工作组，并分配一个在该维度中的工作组大小的唯一本地ID，以便可以通过其全局ID或其本地ID和工作组ID的组合来唯一标识单个工作项。给定工作组中的处理元素上的处理元素并行执行。

在OpenCL中支持的索引空间称为NDRange。NDRange是一个N维索引空间，其中N为1、2或3。NDRange由一个长度为N的整数数组定义，该数组指定了在每个维度中的索引空间的范围，从偏移索引F（默认为零）开始。每个工作项的全局ID和本地ID都是N维元组。全局ID的组件是从F到F加上该维度中元素数减一的范围内的值。

工作组的分配ID使用与工作项全局ID相似的方法。长度为N的数组定义了每个维度中的工作组数。工作项被分配到一个工作组，并为该维度中工作组的大小分配一个本地ID，该ID的组成部分在零到该维度中的工作组大小减一的范围内。因此，工作组ID和工作组内的本地ID的组合唯一定义了一个工作项。每个工作项可以通过其全局索引或通过工作组索引加上工作组内的本地索引的组合来识别。

例如，考虑图3.2中的二维索引空间。我们输入工作项（Gx，Gy）的索引空间，每个工作组的大小（Sx，Sy）和全局ID偏移（Fx，Fy）。全局索引定义了一个Gx乘以Gy的索引空间，其中工作项的总数是Gx和Gy的乘积。本地索引定义了一个Sx乘以Sy的索引空间，其中单个工作组中的工作项数是Sx和Sy的乘积。鉴于每个工作组的大小和工作项的总数，我们可以计算工作组的数量。一个二维索引空间用于唯一标识一个工作组。每个工作项可以通过其全局ID（gx，gy）或通过工作组ID（wx，wy），每个工作组的大小（Sx，Sy）和工作组内的本地ID（sx，sy）的组合来标识。满足以下等式：
\[ (gx, gy) = (wx \times Sx + sx + Fx, wy \times Sy + sy + Fy) \]

工作组的数量可以计算为：
\[ (Wx, Wy) = \left( \frac{Gx}{Sx}, \frac{Gy}{Sy} \right) \]

给定全局ID和工作组大小，工作项的工作组ID可计算为：
\[ (wx, wy) = \left( \left( \frac{gx - sx - Fx}{Sx} \right), \left( \frac{gy - sy - Fy}{Sy} \right) \right) \]

图3.2 展示了一个NDRange索引空间的示例，显示了工作项、它们的全局ID以及它们在工作组和本地ID对上的映射。

这个执行模型可以映射到广泛的编程模型。在OpenCL中，我们明确支持其中的两个模型：数据并行编程模型和任务并行编程模型。

#### 3.2.1 执行模型：上下文和命令队列

主机定义了执行内核的上下文。上下文包括以下资源：

1. **设备：** 主机要使用的所有OpenCL设备的集合。
2. **内核：** 在OpenCL设备上运行的OpenCL函数。
3. **程序对象：** 实现内核的程序源代码和可执行文件。
4. **内存对象：** 对主机和OpenCL设备可见的一组内存对象。内存对象包含可以由内核实例操作的值。

主机使用OpenCL API中的函数创建和操作上下文。主机创建一个称为命令队列的数据结构，以协调在设备上执行内核。主机将命令放入命令队列，然后这些命令被调度到上下文中的设备上。这些命令包括：

- 内核执行命令：在设备的处理元素上执行内核。
- 内存命令：将数据传输到内存对象，从内存对象传输数据，或在主机地址空间中映射和取消映射内存对象。
- 同步命令：约束命令的执行顺序。

命令队列安排命令在设备上执行。这些命令在主机和设备之间异步执行。命令以以下两种模式之一相对于彼此执行：

- **按顺序执行：** 命令按照它们在命令队列中出现的顺序启动和按顺序完成。换句话说，队列中的先前命令在后续命令开始之前完成。这在队列中序列化了命令的执行顺序。
- **无序执行：** 命令按顺序发出，但在后续命令执行之前不等待完成。任何顺序约束由程序员通过显式的同步命令强制执行。

提交给队列的内核执行和内存命令会生成事件对象。这些事件对象用于控制命令之间的执行，并协调主机和设备之间的执行。

可以将多个队列与单个上下文关联。这些队列并发且独立运行，OpenCL内部没有明确的机制在它们之间同步。

#### 3.2.2 执行模型：内核的类别

OpenCL执行模型支持两类内核：

- **OpenCL内核：** 使用OpenCL C编程语言编写并使用OpenCL编译器进行编译。所有OpenCL实现都支持OpenCL内核。实现可能提供其他机制来创建OpenCL内核。
  
- **本地内核：** 通过主机函数指针访问。本地内核与OpenCL内核一起排队在设备上执行，并与OpenCL内核共享内存对象。例如，这些本地内核可以是在应用程序代码中定义的函数或从库中导出的函数。请注意，执行本地内核的能力是OpenCL内部的可选功能，本地内核的语义是由实现定义的。OpenCL API包括用于查询设备功能的功能，以确定是否支持此功能。

### 3.3 内存模型

执行内核的工作项具有对四个不同内存区域的访问：

- **全局内存：** 此内存区域允许所有工作组中的所有工作项进行读/写访问。工作项可以从或写入内存对象的任何元素。根据设备的能力，对全局内存的读写可能会被缓存。

- **常量内存：** 内核执行期间保持不变的全局内存区域。主机分配并初始化放置到常量内存中的内存对象。

- **本地内存：** 与工作组相关的内存区域。该内存区域可用于分配由该工作组中的所有工作项共享的变量。它可以实现为OpenCL设备上的专用内存区域。另外，本地内存区域可以映射到全局内存的某些部分。

- **私有内存：** 工作项专用的内存区域。在一个工作项的私有内存中定义的变量对另一个工作项不可见。

表3.1描述了内核或主机是否可以从内存区域分配，分配类型（静态即编译时 vs 动态即运行时）以及允许的访问类型，即内核或主机是否可以读取和/或写入内存区域。3.2.1 执行模型：上下文和命令队列

主机定义了执行内核的上下文。上下文包括以下资源：

1. **设备：** 主机要使用的所有OpenCL设备的集合。
2. **内核：** 在OpenCL设备上运行的OpenCL函数。
3. **程序对象：** 实现内核的程序源代码和可执行文件。
4. **内存对象：** 对主机和OpenCL设备可见的一组内存对象。内存对象包含可以由内核实例操作的值。

主机使用OpenCL API中的函数创建和操作上下文。主机创建一个称为命令队列的数据结构，以协调在设备上执行内核。主机将命令放入命令队列，然后这些命令被调度到上下文中的设备上。这些命令包括：

- 内核执行命令：在设备的处理元素上执行内核。
- 内存命令：将数据传输到内存对象，从内存对象传输数据，或在主机地址空间中映射和取消映射内存对象。
- 同步命令：约束命令的执行顺序。

命令队列安排命令在设备上执行。这些命令在主机和设备之间异步执行。命令以以下两种模式之一相对于彼此执行：

- **按顺序执行：** 命令按照它们在命令队列中出现的顺序启动和按顺序完成。换句话说，队列中的先前命令在后续命令开始之前完成。这在队列中序列化了命令的执行顺序。
- **无序执行：** 命令按顺序发出，但在后续命令执行之前不等待完成。任何顺序约束由程序员通过显式的同步命令强制执行。

提交给队列的内核执行和内存命令会生成事件对象。这些事件对象用于控制命令之间的执行，并协调主机和设备之间的执行。

可以将多个队列与单个上下文关联。这些队列并发且独立运行，OpenCL内部没有明确的机制在它们之间同步。

#### 3.2.2 执行模型：内核的类别

OpenCL执行模型支持两类内核：

- **OpenCL内核：** 使用OpenCL C编程语言编写并使用OpenCL编译器进行编译。所有OpenCL实现都支持OpenCL内核。实现可能提供其他机制来创建OpenCL内核。
  
- **本地内核：** 通过主机函数指针访问。本地内核与OpenCL内核一起排队在设备上执行，并与OpenCL内核共享内存对象。例如，这些本地内核可以是在应用程序代码中定义的函数或从库中导出的函数。请注意，执行本地内核的能力是OpenCL内部的可选功能，本地内核的语义是由实现定义的。OpenCL API包括用于查询设备功能的功能，以确定是否支持此功能。

### 3.3 内存模型

执行内核的工作项具有对四个不同内存区域的访问：

- **全局内存：** 此内存区域允许所有工作组中的所有工作项进行读/写访问。工作项可以从或写入内存对象的任何元素。根据设备的能力，对全局内存的读写可能会被缓存。

- **常量内存：** 内核执行期间保持不变的全局内存区域。主机分配并初始化放置到常量内存中的内存对象。

- **本地内存：** 与工作组相关的内存区域。该内存区域可用于分配由该工作组中的所有工作项共享的变量。它可以实现为OpenCL设备上的专用内存区域。另外，本地内存区域可以映射到全局内存的某些部分。

- **私有内存：** 工作项专用的内存区域。在一个工作项的私有内存中定义的变量对另一个工作项不可见。

表3.1描述了内核或主机是否可以从内存区域分配，分配类型（静态即编译时 vs 动态即运行时）以及允许的访问类型，即内核或主机是否可以读取和/或写入内存区域。


## 4. OpenCL平台层

本节介绍了实现特定于平台的功能的OpenCL平台层，该功能允许应用程序查询OpenCL设备、设备配置信息，并使用一个或多个设备创建OpenCL上下文。

### 4.1 查询平台信息

可以使用以下函数获取可用平台的列表。
```c
cl_int clGetPlatformIDs(cl_uint num_entries,
                        cl_platform_id *platforms,
                        cl_uint *num_platforms)
```
`num_entries`是可以添加到`platforms`的`cl_platform_id`条目的数量。如果`platforms`不为NULL，则`num_entries`必须大于零。
`platforms`返回找到的OpenCL平台列表。`platforms`中返回的`cl_platform_id`值可用于标识特定的OpenCL平台。如果`platforms`参数为NULL，则忽略此参数。返回的OpenCL平台数量是由`num_entries`指定的值或可用的OpenCL平台数量的最小值。
`num_platforms`返回可用的OpenCL平台数量。如果`num_platforms`为NULL，则忽略此参数。
如果函数成功执行，`clGetPlatformIDs`返回`CL_SUCCESS`。否则，它返回以下错误之一：
- `CL_INVALID_VALUE`，如果`num_entries`等于零且`platforms`不为NULL，或者如果`num_platforms`和`platforms`均为NULL。
- `CL_OUT_OF_HOST_MEMORY`，如果在主机上无法分配OpenCL实现所需的资源。
```c
cl_int clGetPlatformInfo(cl_platform_id platform,
                         cl_platform_info param_name,
                         size_t param_value_size,
                         void *param_value,
                         size_t *param_value_size_ret)
```
此函数获取有关OpenCL平台的特定信息。使用`clGetPlatformInfo`可以查询的信息在表4.1中指定。
`platform`是由`clGetPlatformIDs`返回的平台ID，也可以为NULL。如果`platform`为NULL，则其行为由实现定义。
`param_name`是一个枚举常量，标识正在查询的平台信息。它可以是表4.1中指定的值之一。
`param_value`是一个指向内存位置的指针，在那里将返回表4.1中指定的给定`param_name`的适当值。如果`param_value`为NULL，则将忽略它。 `param_value_size`指定由`param_value`指向的内存的字节数。此字节数必须>=表4.1中指定的返回类型的大小。
`param_value_size_ret`返回由`param_value`查询的数据的实际字节数。如果`param_value_size_ret`为NULL，则将忽略它。


# mesa-18 clover 实现流程分析

一旦加载clover cl库之后， 全局变量_clover_platform 就调用构造函数进行设备探测以及winsys的创建.

## clover 全局变量platform 的定义及构造函数内部的一系列处理

```cpp
namespace {
   platform _clover_platform;
}

namespace clover {
   class platform : public _cl_platform_id,
                    public adaptor_range<
      evals, std::vector<intrusive_ref<device>> &> {
   public:
      platform();

      platform(const platform &platform) = delete;
      platform &
      operator=(const platform &platform) = delete;

   protected:
      std::vector<intrusive_ref<device>> devs;
   };
}

```
* 其中intrusive_ref相当std::atomic
* devs保存了当前平台可用的驱动

### _clover_platform的构造函数

```cpp
platform::platform() : adaptor_range(evals(), devs) {
   int n = pipe_loader_probe(NULL, 0);
   std::vector<pipe_loader_device *> ldevs(n);

    // 首先探测当前平台可用驱动
   pipe_loader_probe(&ldevs.front(), n);

   for (pipe_loader_device *ldev : ldevs) {
      try {
        // 调用device的构造函数创建上下文将设备加入当前平台
         if (ldev)
            devs.push_back(create<device>(*this, ldev));
      } catch (error &) {
         pipe_loader_release(&ldev, 1);
      }
   }
}
```
* pipe_loader_probe 用法和mesa OpenGL一样 会打开/dev/dri/render128 radeonsi 节点以及swarst作为设备驱动

### device的 screen创建

通过intrusive_ref::create<device> 完成这一动作 

```cpp
   /// Initialize a clover::intrusive_ref from a newly created object
   /// using the specified constructor arguments.
   ///
   template<typename T, typename... As>
   intrusive_ref<T>
   create(As &&... as) {
      intrusive_ref<T> ref { *new T(std::forward<As>(as)...) };
      ref().release();
      return ref;
   }
```
* 可以看到这里调用了clover::device的构造函数,将参数as 通过std::forward完美转发方式传递

```cpp
device::device(clover::platform &platform, pipe_loader_device *ldev) :
   platform(platform), ldev(ldev) {
   pipe = pipe_loader_create_screen(ldev);
   if (!pipe || !pipe->get_param(pipe, PIPE_CAP_COMPUTE)) {
      if (pipe)
         pipe->destroy(pipe);
      throw error(CL_INVALID_DEVICE);
   }
}

```
* 从这可以看到内部调用pipe_loader_create_screen进行screen的创建， 这一点和opengl 以及vulkan都是同样用法
* 同时pipe->get_param(...) 对swrast进行了过滤， swrast不支持计算平台
* 最中创建的设备数为1
  
## 根据使用流程对接口api简单分析

### clGetPlatformIDs  获取平台id 

```c

CLOVER_API cl_int
clGetPlatformIDs(cl_uint num_entries, cl_platform_id *rd_platforms,
                 cl_uint *rnum_platforms) {
   if ((!num_entries && rd_platforms) ||
       (!rnum_platforms && !rd_platforms))
      return CL_INVALID_VALUE;

   if (rnum_platforms)
      *rnum_platforms = 1;
   if (rd_platforms)
      *rd_platforms = desc(_clover_platform);

   return CL_SUCCESS;
}
```
* 直接返回1,以及_clover_platform

 ### clGetPlatformInfo 获取平台特定信息

 ```cpp
CLOVER_ICD_API cl_int
clGetPlatformInfo(cl_platform_id d_platform, cl_platform_info param,
                  size_t size, void *r_buf, size_t *r_size) {
   return GetPlatformInfo(d_platform, param, size, r_buf, r_size);
}

cl_int
clover::GetPlatformInfo(cl_platform_id d_platform, cl_platform_info param,
                        size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };

   obj(d_platform);

   switch (param) {
   case CL_PLATFORM_PROFILE:
      buf.as_string() = "FULL_PROFILE";
      break;

   case CL_PLATFORM_VERSION: {
      static const std::string version_string =
            debug_get_option("CLOVER_PLATFORM_VERSION_OVERRIDE", "1.1");

      buf.as_string() = "OpenCL " + version_string + " Mesa " PACKAGE_VERSION MESA_GIT_SHA1;
      break;
   }
   case CL_PLATFORM_NAME:
      buf.as_string() = "Clover";
      break;

   case CL_PLATFORM_VENDOR:
      buf.as_string() = "Mesa";
      break;

   case CL_PLATFORM_EXTENSIONS:
      buf.as_string() = "cl_khr_icd";
      break;

   case CL_PLATFORM_ICD_SUFFIX_KHR:
      buf.as_string() = "MESA";
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}



```

### clGetDeviceIDs 获取设备id

```cpp
CLOVER_API cl_int
clGetDeviceIDs(cl_platform_id d_platform, cl_device_type device_type = CL_TYPE_ALL_,
               cl_uint num_entries, cl_device_id *rd_devices,
               cl_uint *rnum_devices) try {
   auto &platform = obj(d_platform);
   std::vector<cl_device_id> d_devs;

   // Collect matching devices
   for (device &dev : platform) {
      if (((device_type & CL_DEVICE_TYPE_DEFAULT) &&
           dev == platform.front()) ||
          (device_type & dev.type()))
         d_devs.push_back(desc(dev));
   }

   // ...and return the requested data.
   if (rnum_devices)
      *rnum_devices = d_devs.size();
   if (rd_devices)
      copy(range(d_devs.begin(),
                 std::min((unsigned)d_devs.size(), num_entries)),
           rd_devices);
}
```

* 校验设备类型之后传出设备数以及设备

### clGetDeviceInfo 获取设备信息

```c

CLOVER_API cl_int
clGetDeviceInfo(cl_device_id d_dev, cl_device_info param,
                size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };
   auto &dev = obj(d_dev);

   switch (param) {
   case CL_DEVICE_TYPE:
      buf.as_scalar<cl_device_type>() = dev.type();
      break;

   ...

   case CL_DEVICE_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = 1;
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

```
### clCreateContext  创建上下文

```c

CLOVER_API cl_context
clCreateContext(const cl_context_properties *d_props, cl_uint num_devs,
                const cl_device_id *d_devs,
                void (CL_CALLBACK *pfn_notify)(const char *, const void *,
                                               size_t, void *),
    void *user_data, cl_int *r_errcode) try {
   auto props = obj<property_list_tag>(d_props);
   auto devs = objs(d_devs, num_devs);

   for (auto &prop : props) {
      if (prop.first == CL_CONTEXT_PLATFORM)
         obj(prop.second.as<cl_platform_id>());
      else
         throw error(CL_INVALID_PROPERTY);
   }

//  如果回调函数pfn_notify 存在构造一个lambda函数调用，传入context的构造函数
   const auto notify = (!pfn_notify ? context::notify_action() :
                        [=](const char *s) {
                           pfn_notify(s, NULL, 0, user_data);
                        });

   ret_error(r_errcode, CL_SUCCESS);
   return desc(new context(props, devs, notify));

} catch (error &e) {
   ret_error(r_errcode, e);
   return NULL;
}

context::context(const property_list &props,
                 const ref_vector<device> &devs,
                 const notify_action &notify) :
   notify(notify), props(props), devs(devs) {
}
```

### clCreateCommandQueue 创建命令队列

```cpp
CLOVER_API cl_command_queue
clCreateCommandQueue(cl_context d_ctx, cl_device_id d_dev,
                     cl_command_queue_properties props,
                     cl_int *r_errcode) try {
   auto &ctx = obj(d_ctx);
   auto &dev = obj(d_dev);
   ...
   //command_queue构造
   return new command_queue(ctx, dev, props);

} catch (error &e) {
   ret_error(r_errcode, e);
   return NULL;
}

// 
command_queue::command_queue(clover::context &ctx, clover::device &dev,
                             cl_command_queue_properties props) :
   context(ctx), device(dev), props(props) {

    // 
   pipe = dev.pipe->context_create(dev.pipe, NULL, PIPE_CONTEXT_COMPUTE_ONLY);
            [jump radeonsi si_pipe_create_context]

   if (ctx.notify) {
        ...
   }
}


static struct pipe_context *si_pipe_create_context(struct pipe_screen *screen,
						   void *priv, unsigned flags)
{
	struct si_screen *sscreen = (struct si_screen *)screen;
	struct pipe_context *ctx;

	if (sscreen->debug_flags & DBG(CHECK_VM))
		flags |= PIPE_CONTEXT_DEBUG;

	ctx = si_create_context(screen, flags);

    // 没有线程返回
	if (!(flags & PIPE_CONTEXT_PREFER_THREADED))
		return ctx;

    // 不支持线程计算
	/* Clover (compute-only) is unsupported. */
	if (flags & PIPE_CONTEXT_COMPUTE_ONLY)
		return ctx;

    ...
	return threaded_context_create(ctx, &sscreen->pool_transfers,
				       si_replace_buffer_storage,
				       sscreen->info.drm_major >= 3 ? si_create_fence : NULL,
				       &((struct si_context*)ctx)->tc);
}


```
* 从这可以看出在创建命令队列的时候和vulkan的用法一样，在内部创建驱动上下文环境为每个队列
* 这里值的注意的是多线程的上下文在计算环境不支持，所以就算不开启gallium_thread也是没有


### clCreateProgramWithSource 创建程序

```c
CLOVER_API cl_program
clCreateProgramWithSource(cl_context d_ctx, cl_uint count,
                          const char **strings, const size_t *lengths,
                          cl_int *r_errcode) try {
   auto &ctx = obj(d_ctx);
   std::string source;

   // Concatenate all the provided fragments together
   for (unsigned i = 0; i < count; ++i)
         source += (lengths && lengths[i] ?
                    std::string(strings[i], strings[i] + lengths[i]) :
                    std::string(strings[i]));

   // ...and create a program object for them.
   ret_error(r_errcode, CL_SUCCESS);
   return new program(ctx, source);

} catch (error &e) {
   ret_error(r_errcode, e);
   return NULL;
}
```
*

### clBuildProgram 构造程序

```c
CLOVER_API cl_int
clBuildProgram(cl_program d_prog, cl_uint num_devs,
               const cl_device_id *d_devs, const char *p_opts,
               void (*pfn_notify)(cl_program, void *),
               void *user_data) try {
   auto &prog = obj(d_prog);
   auto devs = (d_devs ? objs(d_devs, num_devs) :
                ref_vector<device>(prog.context().devices()));
   const auto opts = std::string(p_opts ? p_opts : "") + " " +
                     debug_get_option("CLOVER_EXTRA_BUILD_OPTIONS", "");

   validate_build_common(prog, num_devs, d_devs, pfn_notify, user_data);

   if (prog.has_source) {
      prog.compile(devs, opts);
      prog.link(devs, opts, { prog });
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

```

* 构造程序时使用llvm::compie, link

### clCreateKernel 创建内核

```cpp

CLOVER_API cl_kernel
clCreateKernel(cl_program d_prog, const char *name, cl_int *r_errcode) try {
   auto &prog = obj(d_prog);
   auto &sym = find(name_equals(name), prog.symbols());
   return new kernel(prog, name, range(sym.args));
}

```

### clCreateBuffer 创建缓冲

```cpp
CLOVER_API cl_mem
clCreateBuffer(cl_context d_ctx, cl_mem_flags d_flags, size_t size,
               void *host_ptr, cl_int *r_errcode) try {
   const cl_mem_flags flags = validate_flags(NULL, d_flags);
   auto &ctx = obj(d_ctx);
   return new root_buffer(ctx, flags, size, host_ptr);

} catch (error &e) {
   ret_error(r_errcode, e);
   return NULL;
}
```

### clEnqueueWriteBuffer 写入缓冲

```c++
CLOVER_API cl_int
clEnqueueWriteBuffer(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                     size_t offset, size_t size, const void *ptr,
                     cl_uint num_deps, const cl_event *d_deps,
                     cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &mem = obj<buffer>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   vector_t region = { size, 1, 1 };
   vector_t obj_origin = { offset };
   auto obj_pitch = pitch(region, {{ 1 }});

   auto hev = create<hard_event>(
      q, CL_COMMAND_WRITE_BUFFER, deps,
      soft_copy_op(q, &mem, obj_origin, obj_pitch,
                   ptr, {}, obj_pitch,
                   region));
----------------------------------------------
        hard_event::hard_event(command_queue &q, cl_command_type command,
                               const ref_vector<event> &deps, action action) :
           event(q.context(), deps, profile(q, action), [](event &ev){}),
           _queue(q), _command(command), _fence(NULL) {
           if (q.profiling_enabled())
              _time_queued = timestamp::current(q);
        
           q.sequence(*this);
               std::lock_guard<std::mutex> lock(queued_events_mutex);
               if (!queued_events.empty())
                  queued_events.back()().chain(ev);
            
               queued_events.push_back(ev);


           trigger();
               if (wait_count() == 1)
                  action_ok(*this);
               
               for (event &ev : trigger_self())
                  ev.trigger();
        }

   if (blocking)
       hev().wait_signalled();

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

event::action
hard_event::profile(command_queue &q, const action &action) const {
   if (q.profiling_enabled()) {
      return [&q, action] (event &ev) {
         auto &hev = static_cast<hard_event &>(ev);

         hev._time_submit = timestamp::current(q);
         hev._time_start = timestamp::query(q);

         action(ev);

         hev._time_end = timestamp::query(q);
      };

   } else {
      return action;
   }
}
```
* 设置一个action这个action就是将源数据拷贝到目的地址

### clSetKernelArg 设置内核参数

```c

CLOVER_API cl_int
clSetKernelArg(cl_kernel d_kern, cl_uint idx, size_t size,
               const void *value) try {
   obj(d_kern).args().at(idx).set(size, value);
   return CL_SUCCESS;

} catch (std::out_of_range &e) {
   return CL_INVALID_ARG_INDEX;

} catch (error &e) {
   return e.get();
}
```

### clEnqueueNDRangeKernel 执行内核

```cpp
CLOVER_API cl_int
clEnqueueNDRangeKernel(cl_command_queue d_q, cl_kernel d_kern,
                       cl_uint dims, const size_t *d_grid_offset,
                       const size_t *d_grid_size, const size_t *d_block_size,
                       cl_uint num_deps, const cl_event *d_deps,
                       cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &kern = obj(d_kern);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto grid_size = validate_grid_size(q, dims, d_grid_size);
   auto grid_offset = validate_grid_offset(q, dims, d_grid_offset);
   auto block_size = validate_block_size(q, kern, dims,
                                         d_grid_size, d_block_size);

   validate_common(q, kern, deps);

   auto hev = create<hard_event>(
      q, CL_COMMAND_NDRANGE_KERNEL, deps,
      [=, &kern, &q](event &) {
         kern.launch(q, grid_offset, grid_size, block_size);
      });

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}


void
kernel::launch(command_queue &q,
               const std::vector<size_t> &grid_offset,
               const std::vector<size_t> &grid_size,
               const std::vector<size_t> &block_size) {
   const auto m = program().build(q.device()).binary;
   const auto reduced_grid_size =
      map(divides(), grid_size, block_size);
   void *st = exec.bind(&q, grid_offset);
   struct pipe_grid_info info = {};

   // The handles are created during exec_context::bind(), so we need make
   // sure to call exec_context::bind() before retrieving them.
   std::vector<uint32_t *> g_handles = map([&](size_t h) {
         return (uint32_t *)&exec.input[h];
      }, exec.g_handles);

   q.pipe->bind_compute_state(q.pipe, st);
   q.pipe->bind_sampler_states(q.pipe, PIPE_SHADER_COMPUTE,
                               0, exec.samplers.size(),
                               exec.samplers.data());

   q.pipe->set_sampler_views(q.pipe, PIPE_SHADER_COMPUTE, 0,
                             exec.sviews.size(), exec.sviews.data());
   q.pipe->set_compute_resources(q.pipe, 0, exec.resources.size(),
                                 exec.resources.data());
   q.pipe->set_global_binding(q.pipe, 0, exec.g_buffers.size(),
                              exec.g_buffers.data(), g_handles.data());

   // Fill information for the launch_grid() call.
   info.work_dim = grid_size.size();
   copy(pad_vector(q, block_size, 1), info.block);
   copy(pad_vector(q, reduced_grid_size, 1), info.grid);
   info.pc = find(name_equals(_name), m.syms).offset;
   info.input = exec.input.data();

   q.pipe->launch_grid(q.pipe, &info);

   q.pipe->set_global_binding(q.pipe, 0, exec.g_buffers.size(), NULL, NULL);
   q.pipe->set_compute_resources(q.pipe, 0, exec.resources.size(), NULL);
   q.pipe->set_sampler_views(q.pipe, PIPE_SHADER_COMPUTE, 0,
                             exec.sviews.size(), NULL);
   q.pipe->bind_sampler_states(q.pipe, PIPE_SHADER_COMPUTE, 0,
                               exec.samplers.size(), NULL);

   q.pipe->memory_barrier(q.pipe, PIPE_BARRIER_GLOBAL_BUFFER);
   exec.unbind();
}
```
* 最终内核的执行和使用计算着色器一样, 不过这里通过set_compute_resources 和set_global_binding将资源绑定到cs

### clEnqueueReadBuffer 读取缓冲

```cpp

CLOVER_API cl_int
clEnqueueReadBuffer(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                    size_t offset, size_t size, void *ptr,
                    cl_uint num_deps, const cl_event *d_deps,
                    cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &mem = obj<buffer>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   vector_t region = { size, 1, 1 };
   vector_t obj_origin = { offset };
   auto obj_pitch = pitch(region, {{ 1 }});

    // 设置CL_COMMAND_READ_BUFFER
   auto hev = create<hard_event>(
      q, CL_COMMAND_READ_BUFFER, deps,
      soft_copy_op(q, ptr, {}, obj_pitch,
                   &mem, obj_origin, obj_pitch,
                   region));

   if (blocking)
       hev().wait_signalled();

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}
```

# 环境变量

## CLOVER_PLATFORM_VERSION_OVERRIDE

## CLOVER_EXTRA_BUILD_OPTIONS



# 调试

使用cl-program-bitcoin-phatk 进行测试
调试时无法打开GALLIUM_DDEBUG，由于dd_context没有实现set_compute_resources 以及set_global_binding
打开R600_DEBUG=compute

打印输出compute input 信息

```
# Platform supporting only version 1.1. Running test on that version.
# Running on:
#   Platform: Clover
#   Device: AMD Radeon RX 550 / 550 Series (POLARIS12, DRM 3.27.0, 4.19.0-25-amd64, LLVM 7.0.1)
#   OpenCL version: 2.0
input 0 : 451767447
input 1 : 953606276
input 2 : 3289282709
input 3 : 1043157133
input 4 : 3980456748
input 5 : 2761019621
input 6 : 3674259583
input 7 : 4041779413
input 8 : 1887291860
input 9 : 3618940936
input 10 : 4220496293
input 11 : 2530934726
input 12 : 204837088
input 13 : 3991048658
input 14 : 1322909696
input 15 : 2820830137
input 16 : 1604534223
input 17 : 3318230743
input 18 : 591787567
input 19 : 1627450019
input 20 : 151637226
input 21 : 4206519064
input 22 : 47933991
input 23 : 0
input 24 : 3072
input 25 : 1
input 26 : 1
input 27 : 0
input 28 : 0
input 29 : 0
```
