#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>

#include <vector>

using namespace std;

#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>

#include "exceptions.h"
#include "log.h"
#include "shader.h"

GLuint LoadShaders(std::string vertex_shader_path, std::string fragment_shader_path)
{
    Log log("ShaderLoader");
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_shader_path, std::ios::in);

    STRONG_ASSERT(VertexShaderStream.is_open(), "Cannot open vertex shader file!");

    {
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_shader_path, std::ios::in);

    STRONG_ASSERT(FragmentShaderStream.is_open(), "Cannot open vertex shader file!");

    {
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        FragmentShaderCode = sstr.str();
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    log.Info() << "Compiling shader: " << vertex_shader_path;

    char const *VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::string VertexShaderErrorMessage;
        VertexShaderErrorMessage.resize(InfoLogLength + 1);

        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL,
                           &VertexShaderErrorMessage[0]);

        log.Error() << "An error has occured during vertex shader compilation: "
                    << VertexShaderErrorMessage;
    }

    log.Info() << "Compiling shader: " << fragment_shader_path;
    char const *FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);

    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

    if (InfoLogLength > 0)
    {
        std::string FragmentShaderErrorMessage;
        FragmentShaderErrorMessage.resize(InfoLogLength + 1);

        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL,
                           &FragmentShaderErrorMessage[0]);

        log.Error() << "An error has occured during fragment shader compilation: "
                    << FragmentShaderErrorMessage;
    }

    log.Info() << "Linking program";

    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::string error_msg;
        error_msg.resize(InfoLogLength + 1);

        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &error_msg[0]);

        log.Error() << "An error has occured during fragment shader linking: "
                    << error_msg;

        STRONG_ASSERT(0);
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}
