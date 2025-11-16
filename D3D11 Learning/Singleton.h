#pragma once
#include <memory>

namespace MyEngine
{
    template <typename T>
    class Singleton
    {
    private:
        static std::unique_ptr<T> m_instance;
    public:
        // 복사 생성자와 대입 연산자를 삭제하여 복사 방지
        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;
        static T* Get()
        {
            if (!m_instance)
            {
                // 기본 생성자를 호출하는 방식
                // 파생 클래스에서 필요에 따라 Initialize 메서드 추가 가능
                m_instance = std::unique_ptr<T>(new T());
            }
            return m_instance.get();
        }
    protected:
        // 생성자는 protected로 파생 클래스만 접근 가능
        Singleton() {}
        // 가상 소멸자
        virtual ~Singleton() {}
    };

    template <typename T>
    std::unique_ptr<T> Singleton<T>::m_instance = nullptr;
}