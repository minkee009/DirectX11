#pragma once

namespace MyEngine::D3DCTX
{
    extern const char* g_vscode_def;
    extern const char* g_vscode_outline_static;
    extern const char* g_vscode_outline_rigid;
    extern const char* g_vscode_outline_skinning;

    extern const char* g_vscode_common_static;
    extern const char* g_vscode_common_rigid;
    extern const char* g_vscode_common_skinning;
}

namespace MyEngine::D3DCTX
{
    extern const char* g_pscode_def;
    extern const char* g_pscode_outline;

    extern const char* g_pscode_blinnphong;
    extern const char* g_pscode_blinnphong_toon;
    extern const char* g_pscode_blinnphong_shadowmap;

    extern const char* g_pscode_BRDF_cook_torrance;
	
    //extern const char* g_pscode_skybox;
}

namespace MyEngine::D3DCTX
{
    extern const char* g_vscode_shadowcast_common;

    extern const char* g_vscode_deffered_static;
    extern const char* g_vscode_deffered_skinning;

    extern const char* g_pscode_deffered_Geometry;
    extern const char* g_pscode_deffered_Light;
    extern const char* g_pscode_deffered_AdditivePointLight;
}


namespace MyEngine::D3DCTX
{
    extern const char* g_postprocess_vscode_quad;
    extern const char* g_postprocess_pscode_ACES_toneMapping;
    extern const char* g_postprocess_pscode_Brightness;
    extern const char* g_postprocess_pscode_GaussianBlur;
    extern const char* g_postprocess_pscode_BloomCombine;
    extern const char* g_postprocess_pscode_PickingMask;
    extern const char* g_postprocess_pscode_sobelOutline;
}