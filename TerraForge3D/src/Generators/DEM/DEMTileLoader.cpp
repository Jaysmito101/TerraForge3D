#include "Generators/DEM/DEMTileLoader.h"

#include "Base/Texture2D.h"
#include "Utils/Utils.h"
#include <stb/stb_image.h>
#include "webp/decode.h"

#include <filesystem>

namespace tf3d::generators::dem
{

    TileLoader::TileLoader(const std::string &texturesDirectory)
    {
        const auto textureRoot = std::filesystem::path(texturesDirectory);
        m_LoadingTexture       = std::make_shared<base::Texture2D>((textureRoot / "loading.png").string(), false);
        m_UnavailableTexture   = std::make_shared<base::Texture2D>((textureRoot / "black.jpg").string(), false);
    }

    TileLoadResult TileLoader::Load(const std::string &path) const
    {
        int width = 1, height = 1, size = 0;
        uint8_t *data = reinterpret_cast<uint8_t *>(ReadBinaryFile(path, &size));
        if (data == nullptr || size <= 0) {
            delete[] data;
            return {TileLoadStatus::Invalid, m_UnavailableTexture};
        }

        auto rgbaData = WebPDecodeRGBA(data, size, &width, &height);
        if (rgbaData == nullptr || width <= 0 || height <= 0) {
            delete[] data;
            if (rgbaData != nullptr)
                free(rgbaData);
            return {TileLoadStatus::Invalid, m_UnavailableTexture};
        }

        auto texture = std::make_shared<base::Texture2D>(width, height);
        texture->SetData(rgbaData, 0, true);
        delete[] data;
        free(rgbaData);
        return {TileLoadStatus::Loaded, std::move(texture)};
    }

    TileLoadResult TileLoader::LoadSatellite(const std::string &path) const
    {
        int width = 1, height = 1, channels = 0;
        stbi_set_flip_vertically_on_load(0);
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 3);
        if (data == nullptr || width <= 0 || height <= 0) {
            if (data != nullptr) {
                stbi_image_free(data);
            }
            return {TileLoadStatus::Invalid, m_UnavailableTexture};
        }

        auto texture = std::make_shared<base::Texture2D>(width, height);
        texture->SetData(data, 0, false);
        stbi_image_free(data);
        return {TileLoadStatus::Loaded, std::move(texture)};
    }

} // namespace tf3d::generators::dem
