cmake_minimum_required(VERSION 3.0.2)

PROJECT (rkffplay)

if(${CONFIG_ROCKCHIP_PLATEFORM} MATCHES "rv1108")
	add_definitions(-DCONFIG_ROCKCHIP_RV1108)

	set(SRC_FILES ffplay.c cmdutils.c)
	add_executable(rkffplay ${SRC_FILES})

	target_include_directories(rkffplay PUBLIC
		"${rkffplay_SOURCE_DIR}/../../ffmpeg-4.1.3/")

	#LINK_DIRECTORIES("${rkffplay_SOURCE_DIR}/../../../out/system/lib/")

	target_link_libraries(rkffplay
		avfilter swscale avresample avdevice avutil avcodec avformat swresample
		SDL2 rkfb ion rga m z rockchip_mpp asound)
else()
	option (DISPLAY_DRM_DIRECTLY "Use drm directly to display" ON)

	if(${DISPLAY_DRM_DIRECTLY})
		message("##Use drm directly to display")
		add_definitions(-DCONFIG_ROCKCHIP_DRM_DIRECTLY)

		set(SRC_FILES ffplay.c cmdutils.c buffers.c kmssink.c
			util/format.c util/kms.c util/pattern.c)
		add_executable(rkffplay ${SRC_FILES})

		find_package(PkgConfig)
		pkg_check_modules (DRM libdrm REQUIRED)

		target_include_directories(rkffplay PUBLIC
			${FFMPEG_BUILDDIR}
			${DRM_INCLUDE_DIRS})

		target_link_libraries(rkffplay
				avutil avcodec avformat avresample swresample avfilter avdevice swscale
				SDL2 m z asound rga ${DRM_LIBRARIES})

	else()
		set(SRC_FILES ffplay.c cmdutils.c)
		add_executable(rkffplay ${SRC_FILES})

		target_include_directories(rkffplay PUBLIC ${FFMPEG_BUILDDIR})
		target_link_libraries(rkffplay
				avutil avcodec avformat swresample avfilter avdevice swscale
				avresample SDL2 m z asound)
	endif()
endif()

install(TARGETS rkffplay DESTINATION bin)
