#include "Time.h"

MyEngine::Time*  MyEngine::Time::instance;

MyEngine::Time::Time()
{
	m_startTime = std::chrono::high_resolution_clock::now();
	m_lastTime = m_startTime;
	m_deltaTime = 0.0f;
	m_totalTime = 0.0f;

	if(instance == nullptr)
		instance = this;
}

MyEngine::Time::~Time()
{
	if(instance == this)
		instance = nullptr;
}

void MyEngine::Time::Update()
{
	auto currentTime = std::chrono::high_resolution_clock::now();
	m_deltaTime = std::chrono::duration<float>(currentTime - m_lastTime).count();
	m_totalTime = std::chrono::duration<float>(currentTime - m_startTime).count();
	m_lastTime = currentTime;

	if (m_deltaTime > 0.05f)
	{
		m_deltaTime = 0.05f;
	}
}

