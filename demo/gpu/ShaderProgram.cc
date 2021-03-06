#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

#include "GPU.h"
extern "C" {
#include "../extern/gawain/gawain/shader_interface.h"
}

#include "File.h"

#include "ShaderProgram.h"


namespace bWidgetsDemo {

class ShaderType
{
public:
	ShaderType(ShaderTypeID id, GLuint gl_id) :
		id(id), gl_id(gl_id)
	{

	}

	ShaderTypeID id;
	GLuint gl_id;
};

static ShaderType shader_types[SHADER_TYPE_TOT] = {
	ShaderType{SHADER_TYPE_VERTEX, GL_VERTEX_SHADER},
	ShaderType{SHADER_TYPE_FRAGMENT, GL_FRAGMENT_SHADER},
};

class ShaderProgramType
{
public:
	ShaderProgramType(const std::array<std::string, SHADER_TYPE_TOT>& names) :
		shader_names(names)
	{

	}
	std::array<std::string, SHADER_TYPE_TOT> shader_names;
};

static ShaderProgramType shader_program_types[ShaderProgram::SHADER_PROGRAM_ID_TOT] = {
	ShaderProgramType{{"uniform_color_vert.glsl", "uniform_color_frag.glsl"}},
	ShaderProgramType{{"smooth_color_vert.glsl", "smooth_color_frag.glsl"}},
	ShaderProgramType{{"texture_vert.glsl", "bitmap_texture_uniform_color_frag.glsl"}},
};

} // namespace bWidgetsDemo

using namespace bWidgetsDemo;


static unsigned int shaderprog_compileShader(const std::string& shader_str, const ShaderType& shader_type)
{
	GLuint shader_id = glCreateShader(shader_type.gl_id);
	const char* shader_c_str = shader_str.c_str();

	glShaderSource(shader_id, 1, &shader_c_str, NULL);
	glCompileShader(shader_id);

#ifndef NDEBUG
	GLint success = GL_FALSE;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		int len;
		char log[568];

		glGetShaderInfoLog(shader_id, 568, &len, log);
		std::cout << log << std::endl;
	}
	assert(success == GL_TRUE);
#endif

	return shader_id;
}

static unsigned int shaderprog_linkProgram(const std::array<unsigned int, SHADER_TYPE_TOT>& shader_ids)
{
	unsigned int program_id = glCreateProgram();

	for (unsigned int shader_id : shader_ids) {
		glAttachShader(program_id, shader_id);
	}
	glLinkProgram(program_id);

#ifndef NDEBUG
	GLint success = GL_FALSE;
	glGetProgramiv(program_id, GL_LINK_STATUS, &success);
	assert(success == GL_TRUE);
#endif

	return program_id;
}

ShaderProgram::ShaderProgram(ShaderProgram::ShaderProgramID shader_program_id)
{
	ShaderProgramType& type = shader_program_types[shader_program_id];

	for (int i = 0; i < shader_ids.size(); i++) {
		File shader_file{std::string(SHADERS_PATH_STR) + "/" + type.shader_names[i]};
		std::string shader_str = shader_file.readIntoString();
		unsigned int shader_id = shaderprog_compileShader(shader_str, shader_types[i]);
		shader_ids[i] = shader_id;
	}
	programID = shaderprog_linkProgram(shader_ids);
	interface = ShaderInterface_create(programID);
}

ShaderProgram::~ShaderProgram()
{
	ShaderInterface_discard(interface);
	for (unsigned int shader_id : shader_ids) {
		glDeleteShader(shader_id);
	}
	glDeleteProgram(programID);
}

unsigned int ShaderProgram::ProgramID() const
{
	return programID;
}

ShaderInterface* ShaderProgram::getInterface() const
{
	return interface;
}
