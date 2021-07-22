#include "shader.hh"

#include <GL/gl.h>
#include <spdlog/spdlog.h>

Shader::Shader(const std::string& vert, const std::string& frag) : m_ProgramID{ glCreateProgram() }
{
    static const auto compile_shader = [](GLenum type, const char* src) -> GLuint {
        GLuint id{ glCreateShader(type) };
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        GLint result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);
        if (!result)
        {
            GLint length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            std::string msg(length, ' ');
            glGetShaderInfoLog(id, length, &length, &msg[0]);
            spdlog::error(fmt::format("Failed to compile shader:\n{}", msg));

            glDeleteShader(id);
            return 0;
        }

        return id;
    };

    GLuint vs{ compile_shader(GL_VERTEX_SHADER, vert.c_str()) };
    GLuint fs{ compile_shader(GL_FRAGMENT_SHADER, frag.c_str()) };

    glAttachShader(m_ProgramID, vs);
    glAttachShader(m_ProgramID, fs);
    glLinkProgram(m_ProgramID);
    glValidateProgram(m_ProgramID);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader()
{
    glDeleteProgram(m_ProgramID);
}

void Shader::set_1i(std::string_view name, int value) const
{
    GLint loc{ glGetUniformLocation(m_ProgramID, name.data()) };
    glUniform1i(loc, value);
}

void Shader::set_1f(std::string_view name, float value) const
{
    GLint loc{ glGetUniformLocation(m_ProgramID, name.data()) };
    glUniform1f(loc, value);
}

void Shader::set_2f(std::string_view name, const std::array<float, 2>& v) const
{
    GLint loc{ glGetUniformLocation(m_ProgramID, name.data()) };
    glUniform2f(loc, v[0], v[1]);
}

void Shader::set_4f(std::string_view name, const std::array<float, 4>& v) const
{
    GLint loc{ glGetUniformLocation(m_ProgramID, name.data()) };
    glUniform4f(loc, v[0], v[1], v[2], v[3]);
}
