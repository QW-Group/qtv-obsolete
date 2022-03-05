cmake_minimum_required(VERSION 3.4.0)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Set project name and languge.
project(qtv C)


######################################################################################################

# Set where sources located.
set(DIR_SRC "src")

# Add sources
set(SRC_COMMON
	"${DIR_SRC}/ban.c"
	"${DIR_SRC}/build.c"
	"${DIR_SRC}/cl_cmds.c"
	"${DIR_SRC}/cmd.c"
	"${DIR_SRC}/crc.c"
	"${DIR_SRC}/cvar.c"
	"${DIR_SRC}/forward.c"
	"${DIR_SRC}/forward_pending.c"
	"${DIR_SRC}/fs.c"
	"${DIR_SRC}/httpsv.c"
	"${DIR_SRC}/httpsv_generate.c"
	"${DIR_SRC}/info.c"
	"${DIR_SRC}/main.c"
	"${DIR_SRC}/mdfour.c"
	"${DIR_SRC}/msg.c"
	"${DIR_SRC}/net_utils.c"
	"${DIR_SRC}/parse.c"
	"${DIR_SRC}/qw.c"
	"${DIR_SRC}/source.c"
	"${DIR_SRC}/source_cmds.c"
	"${DIR_SRC}/sys.c"
	"${DIR_SRC}/token.c"
	"${DIR_SRC}/sha3.c"
	"${DIR_SRC}/udp.c"
	)


######################################################################################################

# Set base compiler flags
set(CFLAGS -Wall)
set(LFLAGS)


######################################################################################################

# Set target
add_executable(${PROJECT_NAME} ${SRC_COMMON})
set_target_properties(${PROJECT_NAME}
	PROPERTIES #PREFIX "" # Strip lib prefix.
	C_VISIBILITY_PRESET hidden # Hide all symbols unless excplicitly marked to export.
	)


######################################################################################################

# Set include directories
target_include_directories(${PROJECT_NAME} PRIVATE)


######################################################################################################

# Check build target, and included sources and libs
if(UNIX)
else()
	target_link_libraries(${PROJECT_NAME} ws2_32)
	set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc")
endif()


######################################################################################################

# Assign compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS})


######################################################################################################
