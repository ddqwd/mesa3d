#!/bin/sh
#static const struct debug_control radv_debug_options[] = {
#	{"nofastclears", RADV_DEBUG_NO_FAST_CLEARS},
#	{"nodcc", RADV_DEBUG_NO_DCC},
#	{"shaders", RADV_DEBUG_DUMP_SHADERS},
#	{"nocache", RADV_DEBUG_NO_CACHE},
#	{"shaderstats", RADV_DEBUG_DUMP_SHADER_STATS},
#	{"nohiz", RADV_DEBUG_NO_HIZ},
#	{"nocompute", RADV_DEBUG_NO_COMPUTE_QUEUE},
#	{"unsafemath", RADV_DEBUG_UNSAFE_MATH},
#	{"allbos", RADV_DEBUG_ALL_BOS},
#	{"noibs", RADV_DEBUG_NO_IBS},
#	{"spirv", RADV_DEBUG_DUMP_SPIRV},
#	{"vmfaults", RADV_DEBUG_VM_FAULTS},
#	{"zerovram", RADV_DEBUG_ZERO_VRAM},
#	{"syncshaders", RADV_DEBUG_SYNC_SHADERS},
#	{"nosisched", RADV_DEBUG_NO_SISCHED},
#	{"preoptir", RADV_DEBUG_PREOPTIR},
#	{"nodynamicbounds", RADV_DEBUG_NO_DYNAMIC_BOUNDS},
#	{"nooutoforder", RADV_DEBUG_NO_OUT_OF_ORDER},
#	{"info", RADV_DEBUG_INFO},
#	{"errors", RADV_DEBUG_ERRORS},
#	{"startup", RADV_DEBUG_STARTUP},
#	{"checkir", RADV_DEBUG_CHECKIR},
#	{"nothreadllvm", RADV_DEBUG_NOTHREADLLVM},
#	{NULL, 0}
#};



export RADV_DEBUG=info,startup,spirv,shaders


#static const struct debug_control radv_perftest_options[] = {
#	{"nobatchchain", RADV_PERFTEST_NO_BATCHCHAIN},
#	{"sisched", RADV_PERFTEST_SISCHED},
#	{"localbos", RADV_PERFTEST_LOCAL_BOS},
#	{"binning", RADV_PERFTEST_BINNING},
#	{"dccmsaa", RADV_PERFTEST_DCC_MSAA},
#	{NULL, 0}
#};


export RADV_PERFTEST=


#export RADV_FORCE_FAMILY=


export RADV_TRACE_FILE=./vulkan.log

#export RADV_TEX_ANISO=
