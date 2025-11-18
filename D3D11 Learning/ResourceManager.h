#pragma once
#include "Singleton.h"
#include <Windows.h>
#include <memory>
#include <unordered_map>
#include <string>

namespace MyEngine
{
	class Resource;
	class ResourceManager : public Singleton<ResourceManager>
	{
	public:
		ResourceManager() = default;
		~ResourceManager() = default;
		void StartUp();
		void Shutdown();

		template <typename T>
		std::shared_ptr<T> Load(std::string key)
		{
			//키가 있는 지 확인
			auto resource_iter = m_resourceTable.find(key);

			if (resource_iter != m_resourceTable.end())
			{
				auto res_shared = resource_iter->second.lock();
				if (res_shared)
					if (auto derived_shared = std::dynamic_pointer_cast<T>(res_shared))
						return derived_shared;

				m_resourceTable.erase(resource_iter);
			}

			//없는 경우 새로 만들기
			auto newRes = std::make_shared<T>();
			m_resourceTable[key] = newRes;
			return newRes;
		}

		bool Containskey(std::string key)
		{
			auto resource_iter = m_resourceTable.find(key);
			if (resource_iter != m_resourceTable.end())
			{
				auto res_shared = resource_iter->second.lock();
				if (res_shared)
					return true;

				m_resourceTable.erase(resource_iter);
			}
			
			return false;
		}

	private:
		friend class Engine;
		friend class MyApp;
		friend class MyD3DContext;
		friend class AssimpConverter;
	
		std::unordered_map<std::string, std::weak_ptr<Resource>> m_resourceTable;
	};
}