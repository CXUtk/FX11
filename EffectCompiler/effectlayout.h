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
    ID3DX11EffectConstantBuffer* ParentConstBuffer;
};

struct PEffectConstBuffer
{
    D3DX11_EFFECT_VARIABLE_DESC Desc;
    std::vector<PEffectVariable> Variables;
};

enum class ShaderType
{
    VertexShader,
    PixelShader,
    HullShader,
    GeometryShader,
    ComputeShader,
    DomainShader,
};

template<>
struct SRefl::EnumInfo<ShaderType>
{
    SREFL_TYPEINFO_HEADER(ShaderType);
    constexpr static auto _ENUMLIST()
    {
        return std::make_tuple(
            SREFL_ENUM_TERM(VertexShader),
            SREFL_ENUM_TERM(PixelShader),
            SREFL_ENUM_TERM(HullShader),
            SREFL_ENUM_TERM(GeometryShader),
            SREFL_ENUM_TERM(ComputeShader),
            SREFL_ENUM_TERM(DomainShader)
        );
    }
#define LISTFUNC(F) F(VertexShader) F(PixelShader) F(HullShader) F(GeometryShader) F(ComputeShader) F(DomainShader)
    GENERATE_ENUM_MAPPING(ShaderType, LISTFUNC)
#undef LISTFUNC
};

struct PShaderVariable
{
    D3D11_SHADER_VARIABLE_DESC Desc;
    D3D11_SHADER_TYPE_DESC Type;
};
struct PShaderBuffer
{
    D3D11_SHADER_BUFFER_DESC Desc;
    std::vector<PShaderVariable> Variables;
};
struct PShaderReflectData
{
    D3D11_SHADER_DESC Desc;
    std::vector<PShaderBuffer> ConstBuffers;
    std::vector<D3D11_SHADER_INPUT_BIND_DESC> Inputs;
};

struct PEffectShader
{
    ShaderType ShaderType;
    std::shared_ptr<PShaderReflectData> Data;
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
