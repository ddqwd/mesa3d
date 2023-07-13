
## RadesonSI驱动中的compiler

### compile结构

radeonsi驱动使用的是ac_llvm_compiler 结构进行shader的编译。
首先看下这个结构。

```c
/* Per-thread persistent LLVM objects. */
/* 每个线程的持久化 LLVM 对象。*/
struct ac_llvm_compiler {
	LLVMTargetLibraryInfoRef	target_library_info;  // 目标库信息对象的引用
	LLVMPassManagerRef		passmgr;  // Pass 管理对象的引用

	/* Default compiler. */
	/* 默认编译器。*/
	LLVMTargetMachineRef		tm;  // 默认的编译器目标机器对象的引用
	struct ac_compiler_passes	*passes;  // 默认编译器的 Pass 配置

	/* Optional compiler for faster compilation with fewer optimizations.
	 * LLVM modules can be created with "tm" too. There is no difference.
	 */
	/* 用于更快速编译和较少优化的可选编译器。
	 * LLVM 模块也可以使用 "tm" 创建。没有区别。
	 */
	LLVMTargetMachineRef		low_opt_tm;  // 用于快速编译且较少优化的编译器目标机器对象的引用
	struct ac_compiler_passes	*low_opt_passes;  // 快速编译且较少优化的编译器的 Pass 配置
};


```

### 初始化流程

这个结构的初始化是在sscreen创建时候进行初始化的。

amdgpu_winsys
radeonsi_screen_create
si_init_compiler
	ac_init_llvm_once
		ac_init_llvm_target
	ac_init_llvm_compiler
		ac_create_target_machine
		ac_create_target_library_info
		ac_create_passmgr
	ac_create_llvm_passes

### 流程重要函数分析

### ac_init_llvm_target

以下是对代码片段中函数调用和注释的分析：

1. `LLVMInitializeAMDGPUTargetInfo()`: 这个函数用于初始化 AMD GPU 的目标信息。

2. `LLVMInitializeAMDGPUTarget()`: 这个函数用于初始化 AMD GPU 的目标。

3. `LLVMInitializeAMDGPUTargetMC()`: 这个函数用于初始化 AMD GPU 的目标机器码。

4. `LLVMInitializeAMDGPUAsmPrinter()`: 这个函数用于初始化 AMD GPU 的汇编打印器。

5. `LLVMInitializeAMDGPUAsmParser()`: 这个函数用于初始化 AMD GPU 的汇编解析器，以支持内联汇编。

6. `const char *argv[3] = { "mesa", "-simplifycfg-sink-common=false", "-global-isel-abort=2" }`: 这行代码定义了一个包含三个字符串指针的数组 `argv`，用于设置命令行选项。这些选项为 "-simplifycfg-sink-common=false" 和 "-global-isel-abort=2"，并且数组的第一个元素 "mesa" 是用于错误消息的前缀。

7. `LLVMParseCommandLineOptions(3, argv, NULL)`: 这个函数调用解析命令行选项，并将数组 `argv` 中的选项应用于 LLVM。它允许在编译过程中设置命令行选项，以控制 LLVM 的行为。

综合而言，这段代码的目的是初始化 AMD GPU 相关的 LLVM 目标信息、目标、目标机器码、汇编打印器和汇编解析器。同时，通过设置命令行选项，可以影响 LLVM 的行为，例如关闭简化控制流图的常见汇聚，以及在全局指令选择失败时回退到 SelectionDAG 并打印诊断信息。这些初始化和命令行选项的设置是为了确保编译过程中的正确性和一致性，以及处理特定的编译器行为和问题。

#### ac_create_target_machine

```c

// 该函数用于创建 LLVM 目标机器（Target Machine）。它根据给定的 GPU 家族（family）、目标机器选项（tm_options）、优化级别（level）和目标三元组（out_triple）创建 LLVM 目标机器。
static LLVMTargetMachineRef ac_create_target_machine(enum radeon_family family,
						     enum ac_target_machine_options tm_options,
						     LLVMCodeGenOptLevel level,
						     const char **out_triple)
{
	assert(family >= CHIP_TAHITI);
	char features[256];
	// 这块在LLVM IR调试的打印输出有体现
	const char *triple = (tm_options & AC_TM_SUPPORTS_SPILL) ? "amdgcn-mesa-mesa3d" : "amdgcn--"; 
	LLVMTargetRef target = ac_get_llvm_target(triple);

	...
	...

	//调用 LLVMCreateTargetMachine 函数创建 LLVM 目标机器对象。
	LLVMTargetMachineRef tm = LLVMCreateTargetMachine(
	                             target,
	                             triple,
	                             ac_get_llvm_processor_name(family),
				     features,
	                             level,
	                             LLVMRelocDefault,
	                             LLVMCodeModelDefault);

	....
	....

	return tm;
}

```

### ac_create_passmgr 


1. `LLVMPassManagerRef passmgr = LLVMCreatePassManager()`: 这行代码调用 `LLVMCreatePassManager` 函数创建一个 Pass 管理对象，并将其赋值给 `passmgr` 变量。

2. `if (!passmgr) return NULL`: 如果 Pass 管理对象创建失败，则函数返回 `NULL`。

3. `if (target_library_info) LLVMAddTargetLibraryInfo(target_library_info, passmgr)`: 如果目标库信息对象存在，则调用 `LLVMAddTargetLibraryInfo` 函数将目标库信息应用于 Pass 管理对象。

4. `if (check_ir) LLVMAddVerifierPass(passmgr)`: 如果需要检查 IR，则调用 `LLVMAddVerifierPass` 函数添加一个验证器 Pass。

5. `LLVMAddAlwaysInlinerPass(passmgr)`: 添加一个总是内联器（Always Inliner）的 Pass，用于在编译过程中尽可能进行函数内联。

6. `ac_llvm_add_barrier_noop_pass(passmgr)`: 添加一个无操作的屏障 Pass，用于强制 Pass 管理对象在运行其他 Pass 之前先运行内联器（Always Inliner）以优化内联函数。

7. `LLVMAddPromoteMemoryToRegisterPass(passmgr)`: 添加一个将内存访问提升为寄存器访问的 Pass，用于优化内存操作。

8. `LLVMAddScalarReplAggregatesPass(passmgr)`: 添加一个标量替换聚合体（Scalar Replacement of Aggregates）的 Pass，用于优化聚合体的操作。

9. `LLVMAddLICMPass(passmgr)`: 添加一个循环不变代码外提（Loop Invariant Code Motion）的 Pass，用于提取循环中不变的代码。

10. `LLVMAddAggressiveDCEPass(passmgr)`: 添加一个主动死代码消除（Aggressive Dead Code Elimination）的 Pass，用于消除无用的代码。

11. `LLVMAddCFGSimplificationPass(passmgr)`: 添加一个控制流图简化（Control Flow Graph Simplification）的 Pass，用于简化控制流图。

12. `LLVMAddEarlyCSEMemSSAPass(passmgr)`: 添加一个早期公共子表达式消除（Early CSE with MemorySSA）的 Pass，用于消除内存操作的冗余计算。

13. `LLVMAddInstructionCombiningPass(passmgr)`: 添加一个指令组合（Instruction Combining）的 Pass，用于对指令进行组合和简化。

14. `return passmgr`: 返回创建的 Pass 管理对象。

综合而言，该函数用于创建 LLVM 的 Pass 管理对象，并添加一系列的 Pass 来进行编译过程中的优化和转换。这些 Pass 的顺序和选择经过了一定的考虑，以提供合理的优化效果和代码质量。返回的 Pass 管理对象可以在编译过程中使用，以应用相应的优化和转换。


### ac_create_llvm_passes

这个函数比较简短，就是创建了一个ac_compile_passes, 然后调用了addPassesToEmitFile.

```c
struct ac_compiler_passes *ac_create_llvm_passes(LLVMTargetMachineRef tm)
{
	struct ac_compiler_passes *p = new ac_compiler_passes();
	if (!p)
		return NULL;

	llvm::TargetMachine *TM = reinterpret_cast<llvm::TargetMachine*>(tm);

	if (TM->addPassesToEmitFile(p->passmgr, p->ostream,
#if HAVE_LLVM >= 0x0700
				    nullptr,
#endif
				    llvm::TargetMachine::CGFT_ObjectFile)) {
		fprintf(stderr, "amd: TargetMachine can't emit a file of this type!\n");
		delete p;
		return NULL;
	}
	return p;
}
```

`addPassesToEmitFile` 函数用于将一系列的优化和代码生成 Pass 添加到 Pass 管理器中，以生成目标文件或可执行文件。

该函数的签名如下：
```cpp
void TargetMachine::addPassesToEmitFile(
    PassManagerBase &PM, raw_pwrite_stream &Out,
    CodeGenFileType FileType = CGFT_ObjectFile,
    bool DisableVerify = true,
    AnalysisID StartAfter = nullptr,
    AnalysisID StopAfter = nullptr
);
```

参数说明：
- `PM`: Pass 管理器，用于管理要执行的优化和代码生成 Pass。
- `Out`: 输出流，指定将生成的目标文件或可执行文件写入的目标。
- `FileType`: 目标文件的类型，默认为 `CGFT_ObjectFile`，表示生成目标文件。
- `DisableVerify`: 是否禁用验证，默认为 `true`，表示禁用验证。
- `StartAfter`: 可选的分析 ID，指定从哪个分析 Pass 开始执行。默认为 `nullptr`，表示从头开始执行。
- `StopAfter`: 可选的分析 ID，指定在哪个分析 Pass 后停止执行。默认为 `nullptr`，表示执行完所有 Pass。

该函数的作用是将与目标机器相关的优化和代码生成 Pass 添加到传入的 Pass 管理器中。这些 Pass 会按照指定的顺序依次执行。通过将 Pass 添加到 Pass 管理器中，可以对 LLVM IR 进行优化，并生成目标文件或可执行文件。

在调用该函数之前，需要先创建好目标机器（`TargetMachine`）对象，并在初始化阶段进行必要的设置和配置。然后，将 Pass 管理器和输出流传递给 `addPassesToEmitFile` 函数，它会自动将与目标机器相关的优化和代码生成 Pass 添加到 Pass 管理器中。最后，执行 Pass 管理器的 `run` 方法，即可触发优化和代码生成过程，并将生成的目标文件或可执行文件写入指定的输出流中。

需要注意的是，具体的优化和代码生成 Pass 取决于所选的目标机器和配置。可以根据需求自定义和调整 Pass 的顺序和参数，以获得最佳的优化和代码生成效果。
addPassesToEmitFile 是 LLVM 编译器框架中的一个函数，用于将编译后的代码生成到指定的输出文件中。

该函数的作用是向编译器传递一系列的优化和代码生成 Pass，以生成最终的目标代码，并将其写入指定的输出文件中。它的参数包括一个 PassManager 对象、输出文件的文件描述符、目标文件类型、代码生成选项等。

具体来说，addPassesToEmitFile 函数会将一系列的优化 Pass 和代码生成 Pass 添加到传入的 PassManager 对象中，以按顺序执行这些 Pass。这些 Pass 可以包括诸如函数内联、常量折叠、循环优化、指令选择等优化 Pass，以及将 LLVM IR 代码转换为目标机器代码的代码生成 Pass。

在执行这些 Pass 的过程中，编译器会对输入的 LLVM IR 代码进行优化和转换，然后将生成的目标机器代码写入到指定的输出文件中。这样，使用 addPassesToEmitFile 函数可以方便地配置和控制编译器的优化和代码生成过程，并生成目标文件或可执行文件等输出。

总结来说，addPassesToEmitFile 接口的作用是将一系列优化和代码生成 Pass 添加到编译器中，以将输入的 LLVM IR 代码转换为目标机器代码，并将生成的代码写入指定的输出文件中。



## amdgpu elf文件的生成

根据llvm 后端处理流程来看， elf文件的首先是产生目标对象文件

RadesonSI 通过调用ac_compile_module_to_binary等函数将目标文件的符号等信息写入到ac_compiler_passes里面的code_string 。

###  主要函数调用流程

ac_compile_module_to_binary";
si_compile_llvm";
si_llvm_compile";
ac_compile_module_to_binary
ac_elf_read

####  重要函数分析

### ac_compile_module_to_binary 

这个函数主要有两处需要注意
1. `p->passmgr.run(*llvm::unwrap(module))`: 这一行代码调用了 `passmgr` 对象的 `run` 方法，并传入了 `module` 参数。它是一个对 LLVM Pass Manager 进行运行的调用。通过这个调用，编译器将对给定的 LLVM 模块进行一系列的优化和转换操作。这函数启动了passmgr，这个会将pass过程中的对象文件数据逐个获取从而给下面函数进行链接。

2. `bool success = ac_elf_read(data.data(), data.size(), binary)`: 这一行代码调用了 `ac_elf_read` 函数，传入了 `data` 字节码数据、字节码数据的大小以及 `binary` 结构体的指针。它的目的是将字节码数据解析为 ELF 格式，并将解析结果存储到 `binary` 结构体中。

总体而言，`ac_compile_module_to_binary` 函数是一个将 LLVM 模块编译为 ELF 字节码数据的过程。它通过运行 LLVM Pass Manager 对模块进行优化和转换，生成 ELF 字节码数据并解析到 `binary` 结构体中。在解析过程中，如果出现错误，会输出相应的错误消息。这个函数是整个编译过程中的重要环节，将模块转换为 ELF 字节码数据，为后续的处理和操作提供了基础。

### ac_elf_read

它接收一个 ELF 数据的指针、ELF 数据的大小以及一个用于存储结果的结构体 `ac_shader_binary` 的指针。这个函数用到了大量libelf的接口， 关于接口的使用可详细参考官方参考手册[libelf-doc]()

以下是对该函数的分析：

1. 首先，函数初始化了一些变量和数据结构，包括 ELF 文件的缓冲区、`Elf` 结构体和相关的辅助变量。

2. 接下来，函数通过调用 `elf_memory()` 以内存中的数据创建一个 ELF 对象。

3. 然后，函数通过循环遍历 ELF 文件的各个节（section）来读取不同的节数据。对于每个节，函数根据其名称进行判断，并根据不同的名称执行相应的处理逻辑。

4. 当节的名称为 ".text" 时，函数将读取该节的数据，并将其存储到 `binary` 结构体中的 `code` 字段。

5. 当节的名称为 ".AMDGPU.config" 时，函数将读取该节的数据，并将其存储到 `binary` 结构体中的 `config` 字段。

6. 当节的名称为 ".AMDGPU.disasm" 时，函数将读取该节的数据，并将其存储为 `binary` 结构体中的 `disasm_string` 字段。

7. 当节的名称以 ".rodata" 开头时，函数将读取该节的数据，并将其存储到 `binary` 结构体中的 `rodata` 字段。

8. 当节的名称为 ".symtab" 时，函数将读取该节的数据，并将其解析为符号表，将相关信息存储到 `binary` 结构体中。

9. 当节的名称为 ".rel.text" 时，函数将读取该节的数据，并根据相关信息解析重定位表。

10. 最后，函数进行一些清理操作，释放内存，并返回一个布尔值表示操作是否成功。

该函数的主要目的是根据给定的 ELF 数据解析出相关的代码、配置信息、符号表等，并将这些信息存储到 `ac_shader_binary` 结构体中。


经过read之后就可以下发到winsys通过libdrm_amdgpu传输到gpu底层执行.
