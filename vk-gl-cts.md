
# vulkan 

测试可执行文件:
**deqp-vk**

测试case的库:

set(DEQP_VK_LIBS
	tcutil 
	vkutil
	glutil
	deqp-vk-api
	deqp-vk-pipeline
	deqp-vk-binding-model
	deqp-vk-spirv-assembly
	deqp-vk-shaderrender
	deqp-vk-shaderexecutor
	deqp-vk-memory
	deqp-vk-ubo
	deqp-vk-dynamic-state
	deqp-vk-ssbo
	deqp-vk-query-pool
	deqp-vk-conditional-rendering
	deqp-vk-draw
	deqp-vk-device-group
	deqp-vk-compute
	deqp-vk-image
	deqp-vk-wsi
	deqp-vk-sparse-resources
	deqp-vk-tessellation
	deqp-vk-rasterization
	deqp-vk-synchronization
	deqp-vk-clipping
	deqp-vk-fragment-ops
	deqp-vk-texture
	deqp-vk-geometry
	deqp-vk-robustness
	deqp-vk-render-pass
	deqp-vk-multiview
	deqp-vk-subgroups
	deqp-vk-ycbcr
	deqp-vk-protected-memory
	deqp-vk-memory-model
	deqp-vk-amber
	deqp-vk-imageless-framebuffer
	deqp-vk-transform-feedback
	deqp-vk-descriptor-indexing
	deqp-vk-fragment-shader-interlock
	deqp-vk-modifiers
	deqp-vk-ray-tracing
	deqp-vk-ray-query
	deqp-vk-postmortem
	deqp-vk-fragment-shading-rate
	deqp-vk-reconvergence
	deqp-vk-mesh-shader
	deqp-vk-fragment-shading-barycentric
	deqp-vk-video
	deqp-vk-shader-object
	)


//~/VK-GL-CTS/external/vulkancts/modules/vulkan/CMakeLists.txt
add_deqp_module(deqp-vk "${DEQP_VK_SRCS}" "${DEQP_VK_LIBS}" "tcutil-platform" vktTestPackageEntry.cpp )

// ~/VK-GL-CTS/CMakeLists.txt
add_executable(${MODULE_NAME} ${PROJECT_SOURCE_DIR}/framework/platform/tcuMain.cpp ${ENTRY})




入口点为tcuMain.cpp

```
int main (int argc, char** argv)
{
	int exitStatus = EXIT_SUCCESS;

#if (DE_OS != DE_OS_WIN32)
	// Set stdout to line-buffered mode (will be fully buffered by default if stdout is pipe).
	setvbuf(stdout, DE_NULL, _IOLBF, 4*1024);
#endif

	try
	{
		tcu::CommandLine				cmdLine		(argc, argv);

		if (cmdLine.quietMode())
			disableStdout();

		tcu::DirArchive					archive		(cmdLine.getArchiveDir());
		tcu::TestLog					log			(cmdLine.getLogFileName(), cmdLine.getLogFlags());
		de::UniquePtr<tcu::Platform>	platform	(createPlatform());
		de::UniquePtr<tcu::App>			app			(new tcu::App(*platform, archive, log, cmdLine));

		// Main loop.
		for (;;)
		{
			if (!app->iterate())
			{
				if (cmdLine.getRunMode() == tcu::RUNMODE_EXECUTE &&
					(!app->getResult().isComplete || app->getResult().numFailed))
				{
					exitStatus = EXIT_FAILURE;
				}

				break;
			}
		}
	}
	catch (const std::exception& e)
	{
		tcu::die("%s", e.what());
	}

	return exitStatus;
}
```
