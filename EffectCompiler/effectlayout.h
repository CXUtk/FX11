#pragma once
#include <vector>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

struct PEffectGroup;
struct PEffectTechnique;
struct PEffectPass;
struct PEffectReflectionInfo;
struct PEffectVariable;
struct PEffectConstBuffer;

struct PEffectVariable
{
    D3DX11_EFFECT_VARIABLE_DESC Desc;
    D3DX11_EFFECT_TYPE_DESC Type;
};

struct PEffectConstBuffer
{
    D3DX11_EFFECT_VARIABLE_DESC Desc;
    std::vector<PEffectVariable> Variables;
};


struct PEffectPass
{
    D3DX11_PASS_DESC Desc;
    std::vector<PEffectVariable> Annotations;
};

struct PEffectTechnique
{
    D3DX11_TECHNIQUE_DESC Desc;
    std::vector<PEffectPass> Passes;
};

struct PEffectReflectionInfo
{
    std::vector<PEffectTechnique>       Techniques;
    std::vector<PEffectVariable>        GlobalVariables;
    std::vector<PEffectConstBuffer>     ConstBuffers;
};
