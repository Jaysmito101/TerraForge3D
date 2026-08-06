#pragma once

namespace tf3d::base
{

    struct IVec2 {
        int x;
        int y;

        IVec2(int x, int y)
            : x(x), y(y)
        {
        }

        IVec2(int x)
            : x(x), y(x)
        {
        }

        IVec2()
        {
            x = 0;
            y = 0;
        }
    };

} // namespace tf3d::base
using tf3d::base::IVec2;
