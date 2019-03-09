#ifndef SHADER_HPP
#define SHADER_HPP

#include <GL/glew.h>
#include <string>

GLuint LoadShaders(std::string vertex_shader_path, std::string fragment_shader_path);

#endif
