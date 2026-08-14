#pragma once

#include "Base/Base.h"

#include <memory>
#include <string>

TF3D_FWD_DEC_CLASS(Texture2D, tf3d::base)

namespace tf3d::generators::dem
{

    enum class TileLoadStatus {
        Loaded,
        Invalid
    };

    struct TileLoadResult {
        TileLoadStatus status = TileLoadStatus::Invalid;
        std::shared_ptr<base::Texture2D> texture;
    };

    class TileLoader
    {
    public:
        explicit TileLoader(const std::string &texturesDirectory);

        TileLoadResult Load(const std::string &path) const;
        TileLoadResult LoadSatellite(const std::string &path) const;

        inline const std::shared_ptr<base::Texture2D> &LoadingTexture() const
        {
            return m_LoadingTexture;
        }

        inline const std::shared_ptr<base::Texture2D> &UnavailableTexture() const
        {
            return m_UnavailableTexture;
        }

    private:
        std::shared_ptr<base::Texture2D> m_LoadingTexture;
        std::shared_ptr<base::Texture2D> m_UnavailableTexture;
    };

} // namespace tf3d::generators::dem
