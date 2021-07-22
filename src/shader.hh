#pragma once

#include <array>
#include <string>

class Shader
{
public:
    Shader(const std::string& vert, const std::string& frag);
    ~Shader();

    unsigned int get_id() const { return m_ProgramID; }

    void set_1i(std::string_view name, int value) const;

    void set_1f(std::string_view name, float value) const;
    void set_2f(std::string_view name, const std::array<float, 2>& v) const;
    // void set_3f(std::string_view name, std::array<float, 3>& v) const;
    void set_4f(std::string_view name, const std::array<float, 4>& v) const;

private:
    unsigned int m_ProgramID;
};
