#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <DirectXTex.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <directxtk/SimpleMath.h>

namespace MyEngine
{
	/// <summary>
	/// 후처리를 담당하는 정적 클래스입니다. 
	/// 후처리용 전역 셰이더, 렌더 타겟 뷰, 셰이더 리소스 뷰 등을 관리합니다.
	/// MyD3DContext에서 스크린사이즈 변경 메시지를 받으면 후처리용 렌더 타겟 뷰도 함께 재생성합니다.
	/// </summary>
	class PostProcessingFX
	{

	};
}