#pragma once
#include <vector>
#include <memory>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

struct PEffectGroup;
struct PEffectTechnique;
struct PEffectPass;
struct PEffectReflectionInfo;
struct PEffectVariable;
struct PEffectConstBuffer;
struct PEffectShader;
struct PShaderReflectData;

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

enum ShaderType
{
    VertexShader,
    PixelShader,
    HullShader,
    GeometryShader,
    ComputeShader,
    DomainShader,
};

struct PShaderVariable
{
    D3D11_SHADER_VARIABLE_DESC desc;
    D3D11_SHADER_TYPE_DESC type;
};
struct PShaderBuffer
{
    D3D11_SHADER_BUFFER_DESC desc;
    std::vector<PShaderVariable> variables;
};
struct PShaderReflectData
{
    D3D11_SHADER_DESC desc;
    std::vector<PShaderBuffer> constBuffers;
    std::vector<D3D11_SHADER_INPUT_BIND_DESC> inputs;
};

struct PEffectShader
{
    unsigned int type;
    std::shared_ptr<PShaderReflectData> data;
};

struct PEffectPass
{
    D3DX11_PASS_DESC Desc;
    std::vector<PEffectVariable> Annotations;
    std::vector<PEffectShader> Shaders;
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
