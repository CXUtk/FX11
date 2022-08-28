#define NOMINMAX

#include "SJson.hpp"
#include <fstream>
#include <wrl.h>

#include "effectlayout.h"
#include "DXBC2GLSL/DXBC2GLSL.hpp"
#include "StringUtils.h"

using namespace effectCompiler;

#define CHECK_RESULT(message, ret)\
if(FAILED(hr))\
{\
    printf("Error: %s\n", message);\
    return ret;\
}\

Microsoft::WRL::ComPtr<ID3D11Device>        g_d3dDevice;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> g_d3dContext;

std::map<std::string, int> constBuffer_indexMap;

void WriteToFile(const std::string& path, const std::string & content);

SJson::JsonNode convertShaderInfoToJson(const PEffectReflectionInfo& info);
SJson::JsonNode GetGlobalVariablesJson(const std::vector<PEffectVariable> globalVariables);
SJson::JsonNode GetConstBuffersJson(const std::vector<PEffectConstBuffer> constBuffers);
SJson::JsonNode GetTechniquesJson(const std::vector<PEffectTechnique> techniques);
SJson::JsonNode GetPassJson(const std::vector<PEffectPass> passes);
SJson::JsonNode GetShadersJson(const std::vector<PEffectShader> shaders);
SJson::JsonNode GetResourceBindingsJson(const std::vector<D3D11_SHADER_INPUT_BIND_DESC>&inputDesc);
SJson::JsonNode GetShaderCodeJson(const PEffectShader & shaderDesc);

SJson::JsonNode GetVector4Json(const float vec[4]);

std::string Compile2GLSL(const void* dxbcCode);


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
    auto shaderDesc = ReflectShader(desc.pShaderVariable);\
    if(shaderDesc)\
    {\
        shaders.push_back(PEffectShader{ effectCompiler::ShaderType::##shaderType, shaderDesc}); \
    }\
}\

std::shared_ptr<PShaderReflectData> ReflectShader(ID3DX11EffectShaderVariable* shader)
{
    HRESULT hr;
    PShaderReflectData data;
    D3DX11_EFFECT_SHADER_DESC desc;
    hr = shader->GetShaderDesc(0, &desc);
    CHECK_RESULT("Fail to get shader desc", nullptr);

    if (!desc.pBytecode)
    {
        return nullptr;
    }

    ID3D11ShaderReflection* reflector = nullptr;
    hr = D3DReflect(desc.pBytecode, desc.BytecodeLength, IID_ID3D11ShaderReflection, (void**)&reflector);
    CHECK_RESULT("Fail to reflect", nullptr);

    reflector->GetDesc(&data.Desc);

    std::vector<int> constBufferIndex;
    for (size_t i = 0; i < data.Desc.ConstantBuffers; i++)
    {
        auto cb = reflector->GetConstantBufferByIndex(i);
        PShaderBuffer cbuffer;
        hr = cb->GetDesc(&cbuffer.Desc);
        for (size_t j = 0; j < cbuffer.Desc.Variables; j++)
        {
            auto variable = cb->GetVariableByIndex(j);
            D3D11_SHADER_VARIABLE_DESC vDesc;
            hr = variable->GetDesc(&vDesc);

            auto type = variable->GetType();
            D3D11_SHADER_TYPE_DESC tDesc;
            type->GetDesc(&tDesc);

            cbuffer.Variables.push_back(PShaderVariable{ vDesc, tDesc });
        }
        data.ConstBuffers.push_back(cbuffer);
    }
    
    for (size_t i = 0; i < data.Desc.BoundResources; i++)
    {
        D3D11_SHADER_INPUT_BIND_DESC rDesc;
        hr = reflector->GetResourceBindingDesc(i, &rDesc);
        data.Inputs.push_back(rDesc);
    }

    data.EffectDesc = desc;
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
        D3DX11_EFFECT_VARIABLE_DESC constBufferDesc = {};
        hr = constBuffer->GetDesc(&constBufferDesc);
        CHECK_RESULT("Failed to get const buffer desc", info);
        constBuffer_indexMap[constBufferDesc.Name] = constBuffers.size();

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

        auto constBuffer = variable->GetParentConstantBuffer();
        if (constBuffer->IsValid())
        {
            D3DX11_EFFECT_VARIABLE_DESC constBufferDesc = {};
            hr = constBuffer->GetDesc(&constBufferDesc);
            CHECK_RESULT("Failed to get const buffer desc", info);
            p.ParentConstBuffer = constBuffer;

            if (constBuffer_indexMap.find(constBufferDesc.Name) != constBuffer_indexMap.end())
            {
                int index = constBuffer_indexMap[constBufferDesc.Name];
                constBuffers[index].Variables.push_back(p);
            }
        }
        globalVariables.push_back(p);
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
    auto node = convertShaderInfoToJson(reflectionData);
    SJson::JsonFormatOption option = {};
    option.Inline = false;
    option.KeysWithQuotes = true;
    option.UseTab = false;
    auto text = node.ToString(option);
    WriteToFile("test.json", text);
    return 0;
}


void WriteToFile(const std::string& path, const std::string& content)
{
    std::ofstream ofStream;
    ofStream.open(path);
    if (ofStream.is_open())
    {
        ofStream << content << std::endl;
    }
    ofStream.close();
}

SJson::JsonNode convertShaderInfoToJson(const PEffectReflectionInfo& info)
{
    SJson::JsonNode rootNode;
    // Just the name of parameters used for sanity check
    rootNode["Parameters"] = GetGlobalVariablesJson(info.GlobalVariables);
    rootNode["ConstBuffers"] = GetConstBuffersJson(info.ConstBuffers);
    rootNode["Techniques"] = GetTechniquesJson(info.Techniques);
    return rootNode;
}


SJson::JsonNode GetGlobalVariablesJson(const std::vector<PEffectVariable> variables)
{
    SJson::JsonNode node;
    for (auto& var : variables)
    {
        SJson::JsonNode varNode;
        varNode["Type"] = var.Type.TypeName;

        // If there is no parent const buffer, then it must be a binded resource
        varNode["ConstBufferOffset"] = (int)(var.ParentConstBuffer ? var.Desc.BufferOffset : -1);

        //if (var.ParentConstBuffer)
        //{
        //    D3DX11_EFFECT_VARIABLE_DESC constBufferDesc = {};
        //    var.ParentConstBuffer->GetDesc(&constBufferDesc);
        //    varNode["ParentConstBuffer"] = constBufferDesc.Name;
        //}
        //else
        //{
        //    varNode["ParentConstBuffer"] = "";
        //}
        node[var.Desc.Name] = varNode;
    }
    return node;
}

SJson::JsonNode GetConstBuffersJson(const std::vector<PEffectConstBuffer> constBuffers)
{
    SJson::JsonNode node;
    for (auto& buffer : constBuffers)
    {
        SJson::JsonNode bufferNode;
        bufferNode["BindPoint"] = buffer.Desc.ExplicitBindPoint;
        bufferNode["Variables"] = GetGlobalVariablesJson(buffer.Variables);
        node[buffer.Desc.Name] = bufferNode;
    }
    return node;
}

SJson::JsonNode GetTechniquesJson(const std::vector<PEffectTechnique> techniques)
{
    SJson::JsonNode node;
    for (auto& technique : techniques)
    {
        SJson::JsonNode techniqueNode;
        techniqueNode["Passes"] = GetPassJson(technique.Passes);
        node[technique.Desc.Name] = techniqueNode;
    }
    return node;
}

SJson::JsonNode GetPassJson(const std::vector<PEffectPass> passes)
{
    SJson::JsonNode node;
    for (auto& pass : passes)
    {
        SJson::JsonNode passNode;
        passNode["StencilRef"] = pass.Desc.StencilRef;
        passNode["SampleMask"] = pass.Desc.SampleMask;
        passNode["BlendFactor"] = GetVector4Json(pass.Desc.BlendFactor);

        passNode["Shaders"] = GetShadersJson(pass.Shaders);
        node[pass.Desc.Name] = passNode;
    }
    return node;
}

SJson::JsonNode GetShadersJson(const std::vector<PEffectShader> shaders)
{
    SJson::JsonNode node;
    for (auto& shader : shaders)
    {
        SJson::JsonNode shaderNode;
        shaderNode["Type"] = SRefl::EnumInfo<effectCompiler::ShaderType>::enum_to_string(shader.ShaderType);
        shaderNode["ResouceBindings"] = GetResourceBindingsJson(shader.Data->Inputs);
        shaderNode["Codes"] = GetShaderCodeJson(shader);
        node.push_back(shaderNode);
    }
    return node;
}

SJson::JsonNode GetResourceBindingsJson(const std::vector<D3D11_SHADER_INPUT_BIND_DESC>& inputDesc)
{
    SJson::JsonNode node;
    for (auto& input : inputDesc)
    {
        SJson::JsonNode inputNode;
        inputNode["Type"] = (int)input.Type;
        inputNode["BindPoint"] = input.BindPoint;
        inputNode["BindCount"] = input.BindCount;
        node[input.Name] = inputNode;
    }
    return node;
}

SJson::JsonNode GetShaderCodeJson(const PEffectShader& shaderDesc)
{
    SJson::JsonNode node;
    std::string hlsl;
    Base64Encode(shaderDesc.Data->EffectDesc.pBytecode, shaderDesc.Data->EffectDesc.BytecodeLength, hlsl);

    std::string glslOut = Compile2GLSL(shaderDesc.Data->EffectDesc.pBytecode);
    std::string glsl;
    RegularizeString(glslOut, glsl);

    node["DXBC"] = hlsl;
    node["GLSL"] = glsl;
    return node;
}

SJson::JsonNode GetVector4Json(const float vec[4])
{
    return SJson::JsonNode({vec[0], vec[1], vec[2], vec[3]});
}

std::string Compile2GLSL(const void* dxbcCode)
{
    std::string glsl;
    try
    {
        DXBC2GLSL::DXBC2GLSL dxbc2glsl;
        dxbc2glsl.FeedDXBC(dxbcCode, true, true, STP_Fractional_Odd, STOP_Triangle_CW, GSV_430);
        glsl = dxbc2glsl.GLSLString();

        if (dxbc2glsl.NumInputParams() > 0)
        {
            std::cout << "Input:" << std::endl;
            for (uint32_t i = 0; i < dxbc2glsl.NumInputParams(); ++i)
            {
                std::cout << "\t" << dxbc2glsl.InputParam(i).semantic_name
                    << dxbc2glsl.InputParam(i).semantic_index << std::endl;
            }
            std::cout << std::endl;
        }
        if (dxbc2glsl.NumOutputParams() > 0)
        {
            std::cout << "Output:" << std::endl;
            for (uint32_t i = 0; i < dxbc2glsl.NumOutputParams(); ++i)
            {
                std::cout << "\t" << dxbc2glsl.OutputParam(i).semantic_name
                    << dxbc2glsl.OutputParam(i).semantic_index << std::endl;
            }
            std::cout << std::endl;
        }

        for (uint32_t i = 0; i < dxbc2glsl.NumCBuffers(); ++i)
        {
            std::cout << "CBuffer " << i << ":" << std::endl;
            for (uint32_t j = 0; j < dxbc2glsl.NumVariables(i); ++j)
            {
                std::cout << "\t" << dxbc2glsl.VariableName(i, j)
                    << ' ' << (dxbc2glsl.VariableUsed(i, j) ? "USED" : "UNUSED");
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }

        if (dxbc2glsl.NumResources() > 0)
        {
            std::cout << "Resource:" << std::endl;
            for (uint32_t i = 0; i < dxbc2glsl.NumResources(); ++i)
            {
                std::cout << "\t" << dxbc2glsl.ResourceName(i) << " : "
                    << dxbc2glsl.ResourceBindPoint(i)
                    << ' ' << (dxbc2glsl.ResourceUsed(i) ? "USED" : "UNUSED");
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }

        if (dxbc2glsl.GSInputPrimitive() != SP_Undefined)
        {
            std::cout << "GS input primitive: " << ShaderPrimitiveName(dxbc2glsl.GSInputPrimitive()) << std::endl;

            std::cout << "GS output:" << std::endl;
            for (uint32_t i = 0; i < dxbc2glsl.NumGSOutputTopology(); ++i)
            {
                std::cout << "\t" << ShaderPrimitiveTopologyName(dxbc2glsl.GSOutputTopology(i)) << std::endl;
            }

            std::cout << "Max GS output vertex " << dxbc2glsl.MaxGSOutputVertex() << std::endl << std::endl;
        }
    }
    catch (std::exception& ex)
    {
        std::cout << "Error(s) in conversion:" << std::endl;
        std::cout << ex.what() << std::endl;
        std::cout << "Please send this information and your bytecode file to webmaster at klayge.org. We'll fix this ASAP." << std::endl;
    }
    return glsl;
}
