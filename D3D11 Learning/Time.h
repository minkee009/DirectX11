#pragma once
#include <chrono>

namespace MyEngine
{
	class Time
	{
	private:
		std::chrono::high_resolution_clock::time_point m_startTime;
		std::chrono::high_resolution_clock::time_point m_lastTime;
		float m_deltaTime; // in seconds
		float m_totalTime; // in seconds
	public:
		Time();
		~Time();
		void Update();
		inline float GetDeltaTime() const { return m_deltaTime; }
		inline float GetTotalTime() const { return m_totalTime; }
	};
}