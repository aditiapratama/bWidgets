set(INC
	../glew/include
	gawain
)

set(SRC
	src/attrib_binding.c
	src/batch.c
	src/element.c
	src/buffer_id.cpp
	src/immediate.c
	src/imm_util.c
	src/primitive.c
	src/shader_interface.c
	src/vertex_buffer.c
	src/vertex_format.c

	gawain/attrib_binding.h
	gawain/batch.h
	gawain/buffer_id.h
	gawain/common.h
	gawain/element.h
	gawain/immediate.h
	gawain/imm_util.h
	gawain/primitive.h
	gawain/shader_interface.h
	gawain/vertex_buffer.h
	gawain/vertex_format.h
)

set(LIB
	bwd_extern_glew
	bwd_gpu
)

add_definitions(-DWITH_GL_PROFILE_CORE)

add_library(bwd_extern_gawain ${SRC})
include_directories(${INC})
target_link_libraries(bwd_extern_gawain ${LIB})
