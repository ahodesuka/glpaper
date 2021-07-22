#pragma once

#include <string>

class Texture
{
public:
    Texture(std::string path);
    ~Texture();

    void bind(unsigned int slot) const;
    void unbind() const;

    inline std::string_view get_path() const { return m_Path; }
    inline int get_width() const { return m_Width; }
    inline int get_height() const { return m_Height; }

private:
    std::string m_Path;
    unsigned int m_TexID;

    int m_Width, m_Height;
};
