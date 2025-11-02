#pragma once
#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;

namespace MyEngine
{
	class AABB
	{
	public:
		Vector3 min;
		Vector3 max;
		inline const std::vector<Vector3> ExtractCorners() const
		{
            float bounds[2][3] = {
                { min.x, min.y, min.z }, // bounds[0] -> Min
                { max.x, max.y, max.z }  // bounds[1] -> Max
            };

            std::vector<Vector3> corners(8);

            for (int i = 0; i < 8; ++i)
            {
                Vector3 corner;
                for (int j = 0; j < 3; ++j)
                {
                    const int index = (i >> j) & 1;
                    if (j == 0)      // x√‡
                    {
                        corner.x = bounds[index][j];
                    }
                    else if (j == 1) // y√‡
                    {
                        corner.y = bounds[index][j];
                    }
                    else             // z√‡
                    {
                        corner.z = bounds[index][j];
                    }
                }
                corners[i] = corner;
            }

            return corners;
		}
	};
}