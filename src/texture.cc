#include "texture.hh"

#include <GL/gl.h>
#include <fmt/core.h>
#include <stdexcept>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Texture::Texture(std::string path) : m_Path{ std::move(path) }
{
    stbi_set_flip_vertically_on_load(true);
    auto* pixel_data{ stbi_load(m_Path.c_str(), &m_Width, &m_Height, nullptr, 3) };

    if (!pixel_data)
        throw std::runtime_error(fmt::format("Failed to load image {}", m_Path));

    glGenTextures(1, &m_TexID);
    bind(0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB8, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel_data);
    unbind();

    stbi_image_free(pixel_data);
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_TexID);
    glFinish();
}

void Texture::bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_TexID);
}

void Texture::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}
