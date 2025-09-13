#include "Time.h"

MyEngine::Time::Time()
{
	m_startTime = std::chrono::high_resolution_clock::now();
	m_lastTime = m_startTime;
	m_deltaTime = 0.0f;
	m_totalTime = 0.0f;
}

MyEngine::Time::~Time()
{

}