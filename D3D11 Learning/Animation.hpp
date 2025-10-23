#pragma once
#include <vector>
#include <algorithm>
#include "directxtk/SimpleMath.h"

using namespace DirectX::SimpleMath;

namespace MyEngine
{
	//======= Animation =======//
	template <typename T>
	struct AnimationKeyFrame
	{
		double time;
		T value;
	};

	template <typename T>
	class AnimationCurve
	{
	private:
		std::vector<AnimationKeyFrame<T>> m_keyframes;

		size_t FindKeyframeIndex(double time) const
		{
			for (size_t i = 0; i + 1 < m_keyframes.size(); ++i)
			{
				if (time >= m_keyframes[i].time && time <= m_keyframes[i + 1].time)
					return i;
			}
			return m_keyframes.size() - 1;
		}
	public:
		T Evaluate(double time) const
		{
			if (m_keyframes.empty())
				return T{};

			if (m_keyframes.size() == 1)
				return m_keyframes[0].value;

			if (time <= m_keyframes.front().time)
				return m_keyframes.front().value;
			if (time >= m_keyframes.back().time)
				return m_keyframes.back().value;

			size_t i = FindKeyframeIndex(time);
			const auto& k0 = m_keyframes[i];
			const auto& k1 = m_keyframes[i + 1];

			double t = (time - k0.time) / (k1.time - k0.time);
			return k0.value * (1.0 - t) + k1.value * t;
		}
		void AddKeyframe(double time, double value)
		{
			m_keyframes.push_back({ time, value });
		}
		void SortKeyFrames()
		{
			std::sort(m_keyframes.begin(), m_keyframes.end(), [](const AnimationKeyFrame<T>& a, const AnimationKeyFrame<T>& b)
				{
					return a.time < b.time;
				});
		}
		inline const std::vector<AnimationKeyFrame<T>>& GetKeyframes() const { return m_keyframes; }
	};

	template <>
	inline Quaternion AnimationCurve<Quaternion>::Evaluate(double time) const
	{
		if (m_keyframes.empty())
			return Quaternion(0, 0, 0, 1);

		if (m_keyframes.size() == 1)
			return m_keyframes[0].value;

		if (time <= m_keyframes.front().time)
			return m_keyframes.front().value;
		if (time >= m_keyframes.back().time)
			return m_keyframes.back().value;

		// 인접 키 찾기
		size_t i = FindKeyframeIndex(time);
		const auto& k0 = m_keyframes[i];
		const auto& k1 = m_keyframes[i + 1];

		double t = (time - k0.time) / (k1.time - k0.time);

		// 쿼터니언 보간 (SLERP)
		Quaternion q0 = k0.value;
		Quaternion q1 = k1.value;

		return Quaternion::Slerp(q0, q1, static_cast<float>(t));
	}

	struct AnimationClip
	{
		double duration;
		double frameRate;

		AnimationCurve<Vector3> pos;
		AnimationCurve<Quaternion> rot;
	};
}
