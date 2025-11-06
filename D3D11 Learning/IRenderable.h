#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <initializer_list>

namespace MyEngine
{
	class IRenderable
	{
	public:
		virtual ~IRenderable() = default; 
		virtual void Draw(ID3D11DeviceContext* context) = 0;
	};
}