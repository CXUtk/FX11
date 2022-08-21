#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <wrl.h>

#include "effectlayout.h"

#define CHECK_RESULT(message, ret)\
if(FAILED(hr))\
{\
    printf("Error: %s\n", message);\
    return ret;\
}\

Microsoft::WRL::ComPtr<ID3D11Device>        g_d3dDevice;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> g_d3dContext;


std::map<ID3DX11EffectConstantBuffer*, int> constBuffer_indexMap;


void Usage()
{
    printf("Usage: Effect Compiler\n");
    printf("\n");
    printf("Main options:\n");
    printf("  -h | -?               show help.\n");
    printf("  -i                    input shaderlab file name.\n");
    printf("  -o                    output compiled shader file name.\n");
    printf("  -include              include path.\n");
}

HRESULT InitD3D11()
{
    // Create D3D device and D3D device context
    UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[2] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0 };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    D3D_FEATURE_LEVEL curFeatureLevel;
    D3D_DRIVER_TYPE d3dDriverType;

    d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;

    HRESULT hr = D3D11CreateDevice(nullptr, d3dDriverType, nullptr, createDeviceFlags,
        featureLevels, numFeatureLevels, D3D11_SDK_VERSION, g_d3dDevice.GetAddressOf(),
        &curFeatureLevel, g_d3dContext.GetAddressOf());
    return hr;
}

Microsoft::WRL::ComPtr<ID3DX11Effect> GetEffect(const wchar_t* filename)
{
    unsigned int HLSLFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    if (FAILED(InitD3D11()))
    {
        printf("Failed to create D3D11 Context\n");
        return nullptr;
    }

    Microsoft::WRL::ComPtr<ID3DX11Effect> effect;
    Microsoft::WRL::ComPtr<ID3DBlob> errorLog;
    //HRESULT res = D3DX11CreateEffectFromFile(
    //    filename,
    //    0,
    //    g_d3dDevice.Get(),
    //    effect.GetAddressOf());
    HRESULT res = D3DX11CompileEffectFromFile(
        filename,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        HLSLFlags,
        0,
        g_d3dDevice.Get(),
        effect.GetAddressOf(),
        errorLog.GetAddressOf());

    if (FAILED(res))
    {
        printf("Failed to compile and create effect\n");
        return nullptr;
    }
    else
    {
        return effect;
    }
}

#define GET_SHADER(shaderType) hr = pass->Get##shaderType##Desc(&desc);\
CHECK_RESULT(("Failed to get " + std::string(#shaderType) + " desc").c_str(), shaders);\
if (desc.pShaderVariable->IsValid())\
{\
    shaders.push_back({shaderType,ReflectShader(desc.pShaderVariable)});\
}\

std::shared_ptr<PShaderReflectData> ReflectShader(ID3DX11EffectShaderVariable* shader)
{
    HRESULT hr;
    PShaderReflectData data;
    D3DX11_EFFECT_SHADER_DESC desc;
    hr = shader->GetShaderDesc(0, &desc);
    CHECK_RESULT("Fail to get shader desc", nullptr);

    ID3D11ShaderReflection* reflector = nullptr;
    hr = D3DReflect(desc.pBytecode, desc.BytecodeLength, IID_ID3D11ShaderReflection, (void**)&reflector);
    CHECK_RESULT("Fail to reflect", nullptr);

    reflector->GetDesc(&data.desc);

    std::vector<int> constBufferIndex;
    for (size_t i = 0; i < data.desc.ConstantBuffers; i++)
    {
        auto cb = reflector->GetConstantBufferByIndex(i);
        PShaderBuffer cbuffer;
        hr = cb->GetDesc(&cbuffer.desc);
        for (size_t j = 0; j < cbuffer.desc.Variables; j++)
        {
            auto variable = cb->GetVariableByIndex(j);
            D3D11_SHADER_VARIABLE_DESC vDesc;
            hr = variable->GetDesc(&vDesc);

            auto type = variable->GetType();
            D3D11_SHADER_TYPE_DESC tDesc;
            type->GetDesc(&tDesc);

            cbuffer.variables.push_back(PShaderVariable{ vDesc, tDesc });
        }
        data.constBuffers.push_back(cbuffer);
    }
    
    for (size_t i = 0; i < data.desc.BoundResources; i++)
    {
        D3D11_SHADER_INPUT_BIND_DESC rDesc;
        hr = reflector->GetResourceBindingDesc(i, &rDesc);
        data.inputs.push_back(rDesc);
    }

    return std::make_shared<PShaderReflectData>(data);
}

std::vector<PEffectShader> GetShaders(ID3DX11EffectPass* pass)
{
    HRESULT hr;
    std::vector<PEffectShader> shaders;
    D3DX11_PASS_SHADER_DESC desc;
    
    GET_SHADER(VertexShader);
    GET_SHADER(PixelShader);
    GET_SHADER(HullShader);
    GET_SHADER(GeometryShader);
    GET_SHADER(ComputeShader);
    GET_SHADER(DomainShader);

    return shaders;
}

std::vector<PEffectVariable> GetAnnotations(ID3DX11EffectPass* pass, const D3DX11_PASS_DESC& desc)
{
    HRESULT hr;
    std::vector<PEffectVariable> variables;
    for (int i = 0; i < desc.Annotations; i++)
    {
        auto variable = pass->GetAnnotationByIndex(i);

        D3DX11_EFFECT_VARIABLE_DESC varDesc = {};
        hr = variable->GetDesc(&varDesc);
        CHECK_RESULT("Failed to get variable desc", variables);

        PEffectVariable p = {};
        p.Desc = varDesc;
        variable->GetType()->GetDesc(&p.Type);
        variables.push_back(p);
    }
    return variables;
}

std::vector<PEffectPass> GetPasses(ID3DX11EffectTechnique* technique, const D3DX11_TECHNIQUE_DESC& desc)
{
    HRESULT hr;
    std::vector<PEffectPass> passes;
    for (int i = 0; i < desc.Passes; i++)
    {
        auto pass = technique->GetPassByIndex(i);

        D3DX11_PASS_DESC passDesc = {};
        hr = pass->GetDesc(&passDesc);
        CHECK_RESULT("Failed to get pass desc", passes);

        PEffectPass p = {};
        p.Desc = passDesc;
        p.Annotations = GetAnnotations(pass, passDesc);
        p.Shaders = GetShaders(pass);
        


        passes.push_back(p);
    }
    return passes;
}


PEffectReflectionInfo GenerateReflectionData(Microsoft::WRL::ComPtr<ID3DX11Effect> effect, const D3DX11_EFFECT_DESC& desc)
{
    HRESULT hr;
    PEffectReflectionInfo info;
    for (int i = 0; i < desc.Groups; i++)
    {
        auto group = effect->GetGroupByIndex(i);
        D3DX11_GROUP_DESC groupDesc = {};
        hr = group->GetDesc(&groupDesc);
        CHECK_RESULT("Failed to get group desc", info);
    }

    std::vector<PEffectTechnique> techniques;
    for (int i = 0; i < desc.Techniques; i++)
    {
        auto technique = effect->GetTechniqueByIndex(i);

        D3DX11_TECHNIQUE_DESC techDesc = {};
        hr = technique->GetDesc(&techDesc);
        CHECK_RESULT("Failed to get technique desc", info);

        PEffectTechnique tech = {};
        tech.Desc = techDesc;
        tech.Passes = GetPasses(technique, techDesc);
        techniques.push_back(tech);
    }


    std::vector<PEffectConstBuffer> constBuffers;
    for (int i = 0; i < desc.ConstantBuffers; i++)
    {
        auto constBuffer = effect->GetConstantBufferByIndex(i);
        constBuffer_indexMap[constBuffer] = constBuffers.size();

        D3DX11_EFFECT_VARIABLE_DESC constBufferDesc = {};
        hr = constBuffer->GetDesc(&constBufferDesc);
        CHECK_RESULT("Failed to get const buffer desc", info);

        PEffectConstBuffer p = {};
        p.Desc = constBufferDesc;
        constBuffers.push_back(p);
    }

    std::vector<PEffectVariable> globalVariables;
    for (int i = 0; i < desc.GlobalVariables; i++)
    {
        auto variable = effect->GetVariableByIndex(i);

        D3DX11_EFFECT_VARIABLE_DESC varDesc = {};
        hr = variable->GetDesc(&varDesc);
        CHECK_RESULT("Failed to get global variable desc", info);

        PEffectVariable p = {};
        p.Desc = varDesc;
        variable->GetType()->GetDesc(&p.Type);
        globalVariables.push_back(p);

        auto constBuffer = variable->GetParentConstantBuffer();
        if (constBuffer->IsValid())
        {
            if (constBuffer_indexMap.find(constBuffer) != constBuffer_indexMap.end())
            {
                int index = constBuffer_indexMap[constBuffer];
                constBuffers[index].Variables.push_back(p);
            }
        }
    }

    return PEffectReflectionInfo{ techniques, globalVariables, constBuffers };
}

int main(int argc, char** argv)
{
    //if (argc < 2)
    //{
    //    Usage();
    //    return 0;
    //}
    auto effect = GetEffect(L"Test.fx");
    if (!effect)
    {
        return -1;
    }
    D3DX11_EFFECT_DESC effectDesc = {};
    HRESULT res = effect->GetDesc(&effectDesc);
    if (FAILED(res))
    {
        printf("Failed to get effect description\n");
        return -1;
    }
    auto reflectionData = GenerateReflectionData(effect, effectDesc);
    return 0;
}
