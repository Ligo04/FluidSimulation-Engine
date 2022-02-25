//***************************************************************************************
// d3dUtil.h by X_Jun(MKXJun) (C) 2018-2020 All Rights Reserved.
// Licensed under the MIT License.
//
// D3D实用工具集
// Direct3D utility tools.
//***************************************************************************************

#ifndef D3DUTIL_H
#define D3DUTIL_H

#include <d3d11_1.h>			// 已包含Windows.h
#include <DirectXCollision.h>	// 已包含DirectXMath.h
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <wincodec.h>			// 使用ScreenGrab需要包含
#include <vector>
#include <string>
#include <random>
#include "ScreenGrab.h"
#include <Graphics/DDSTextureLoader.h>
#include <Graphics/WICTextureLoader.h>

//
// 宏相关
//

// 默认开启图形调试器具名化
// 如果不需要该项功能，可通过全局文本替换将其值设置为0
#ifndef GRAPHICS_DEBUGGER_OBJECT_NAME
#define GRAPHICS_DEBUGGER_OBJECT_NAME (1)
#endif

// 安全COM组件释放宏
#define SAFE_RELEASE(p) { if ((p)) { (p)->Release(); (p) = nullptr; } }



//
// 辅助调试相关函数
//

// ------------------------------
// D3D11SetDebugObjectName函数
// ------------------------------
// 为D3D设备创建出来的对象在图形调试器中设置对象名
// [In]resource				D3D11设备创建出的对象
// [In]name					对象名
template<UINT TNameLength>
inline void D3D11SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_ const char(&name)[TNameLength])
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
#else
	UNREFERENCED_PARAMETER(resource);
	UNREFERENCED_PARAMETER(name);
#endif
}

// ------------------------------
// D3D11SetDebugObjectName函数
// ------------------------------
// 为D3D设备创建出来的对象在图形调试器中设置对象名
// [In]resource				D3D11设备创建出的对象
// [In]name					对象名
// [In]length				字符串长度
inline void D3D11SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_ LPCSTR name, _In_ UINT length)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, length, name);
#else
	UNREFERENCED_PARAMETER(resource);
	UNREFERENCED_PARAMETER(name);
	UNREFERENCED_PARAMETER(length);
#endif
}

// ------------------------------
// D3D11SetDebugObjectName函数
// ------------------------------
// 为D3D设备创建出来的对象在图形调试器中设置对象名
// [In]resource				D3D11设备创建出的对象
// [In]name					对象名
inline void D3D11SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_ const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
#else
	UNREFERENCED_PARAMETER(resource);
	UNREFERENCED_PARAMETER(name);
#endif
}

// ------------------------------
// D3D11SetDebugObjectName函数
// ------------------------------
// 为D3D设备创建出来的对象在图形调试器中清空对象名
// [In]resource				D3D11设备创建出的对象
inline void D3D11SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_ std::nullptr_t)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
#else
	UNREFERENCED_PARAMETER(resource);
#endif
}

// ------------------------------
// DXGISetDebugObjectName函数
// ------------------------------
// 为DXGI对象在图形调试器中设置对象名
// [In]object				DXGI对象
// [In]name					对象名
template<UINT TNameLength>
inline void DXGISetDebugObjectName(_In_ IDXGIObject* object, _In_ const char(&name)[TNameLength])
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
#else
	UNREFERENCED_PARAMETER(object);
	UNREFERENCED_PARAMETER(name);
#endif
}

// ------------------------------
// DXGISetDebugObjectName函数
// ------------------------------
// 为DXGI对象在图形调试器中设置对象名
// [In]object				DXGI对象
// [In]name					对象名
// [In]length				字符串长度
inline void DXGISetDebugObjectName(_In_ IDXGIObject* object, _In_ LPCSTR name, _In_ UINT length)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, length, name);
#else
	UNREFERENCED_PARAMETER(object);
	UNREFERENCED_PARAMETER(name);
	UNREFERENCED_PARAMETER(length);
#endif
}

// ------------------------------
// DXGISetDebugObjectName函数
// ------------------------------
// 为DXGI对象在图形调试器中设置对象名
// [In]object				DXGI对象
// [In]name					对象名
inline void DXGISetDebugObjectName(_In_ IDXGIObject* object, _In_ const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
#else
	UNREFERENCED_PARAMETER(object);
	UNREFERENCED_PARAMETER(name);
#endif
}

// ------------------------------
// DXGISetDebugObjectName函数
// ------------------------------
// 为DXGI对象在图形调试器中清空对象名
// [In]object				DXGI对象
inline void DXGISetDebugObjectName(_In_ IDXGIObject* object, _In_ std::nullptr_t)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
#else
	UNREFERENCED_PARAMETER(object);
#endif
}

//
// 着色器编译相关函数
//

// ------------------------------
// CreateShaderFromFile函数
// ------------------------------
// [In]csoFileNameInOut 编译好的着色器二进制文件(.cso)，若有指定则优先寻找该文件并读取
// [In]hlslFileName     着色器代码，若未找到着色器二进制文件则编译着色器代码
// [In]entryPoint       入口点(指定开始的函数)
// [In]shaderModel      着色器模型，格式为"*s_5_0"，*可以为c,d,g,h,p,v之一
// [Out]ppBlobOut       输出着色器二进制信息
HRESULT CreateShaderFromFile(
	const WCHAR* csoFileNameInOut,
	const WCHAR* hlslFileName,
	LPCSTR entryPoint,
	LPCSTR shaderModel,
	ID3DBlob** ppBlobOut);

//
// 缓冲区相关函数
//

// ------------------------------
// CreateVertexBuffer函数
// ------------------------------
// [In]d3dDevice			D3D设备
// [In]data					初始化数据
// [In]byteWidth			缓冲区字节数
// [Out]vertexBuffer		输出的顶点缓冲区
// [InOpt]dynamic			是否需要CPU经常更新
// [InOpt]streamOutput		是否还用于流输出阶段(不能与dynamic同时设为true)
HRESULT CreateVertexBuffer(
	ID3D11Device * d3dDevice,
	void * data,
	UINT byteWidth,
	ID3D11Buffer ** vertexBuffer,
	/* 可选扩展部分 */
	bool dynamic = false,
	bool streamOutput = false);

// ------------------------------
// CreateIndexBuffer函数
// ------------------------------
// [In]d3dDevice			D3D设备
// [In]data					初始化数据
// [In]byteWidth			缓冲区字节数
// [Out]indexBuffer			输出的索引缓冲区
// [InOpt]dynamic			是否需要CPU经常更新
HRESULT CreateIndexBuffer(
	ID3D11Device * d3dDevice,
	void * data,
	UINT byteWidth,
	ID3D11Buffer ** indexBuffer,
	/* 可选扩展部分 */
	bool dynamic = false);

// ------------------------------
// CreateConstantBuffer函数
// ------------------------------
// [In]d3dDevice			D3D设备
// [In]data					初始化数据
// [In]byteWidth			缓冲区字节数，必须是16的倍数
// [Out]indexBuffer			输出的索引缓冲区
// [InOpt]cpuUpdates		是否允许CPU更新
// [InOpt]gpuUpdates		是否允许GPU更新
HRESULT CreateConstantBuffer(
	ID3D11Device * d3dDevice,
	void * data,
	UINT byteWidth,
	ID3D11Buffer ** constantBuffer,
	/* 可选扩展部分 */
	bool cpuUpdates = true,
	bool gpuUpdates = false);

// ------------------------------
// CreateTypedBuffer函数
// ------------------------------
// [In]d3dDevice			D3D设备
// [In]data					初始化数据
// [In]byteWidth			缓冲区字节数
// [Out]typedBuffer			输出的有类型的缓冲区
// [InOpt]cpuUpdates		是否允许CPU更新
// [InOpt]gpuUpdates		是否允许使用RWBuffer
HRESULT CreateTypedBuffer(
	ID3D11Device * d3dDevice,
	void * data,
	UINT byteWidth,
	ID3D11Buffer ** typedBuffer,
	/* 可选扩展部分 */
	bool cpuUpdates = false,
	bool gpuUpdates = false);

// ------------------------------
// CreateStructuredBuffer函数
// ------------------------------
// 如果需要创建Append/Consume Buffer，需指定cpuUpdates为false, gpuUpdates为true
// [In]d3dDevice			D3D设备
// [In]data					初始化数据
// [In]byteWidth			缓冲区字节数
// [In]structuredByteStride 每个结构体的字节数
// [Out]structuredBuffer	输出的结构化缓冲区
// [InOpt]cpuUpdates		是否允许CPU更新
// [InOpt]gpuUpdates		是否允许使用RWStructuredBuffer
HRESULT CreateStructuredBuffer(
	ID3D11Device * d3dDevice,
	void * data,
	UINT byteWidth,
	UINT structuredByteStride,
	ID3D11Buffer ** structuredBuffer,
	/* 可选扩展部分 */
	bool cpuUpdates = false,
	bool gpuUpdates = false);

// ------------------------------
// CreateRawBuffer函数
// ------------------------------
// [In]d3dDevice			D3D设备
// [In]data					初始化数据
// [In]byteWidth			缓冲区字节数
// [Out]rawBuffer			输出的字节地址缓冲区
// [InOpt]cpuUpdates		是否允许CPU更新
// [InOpt]gpuUpdates		是否允许使用RWByteAddressBuffer
HRESULT CreateRawBuffer(
	ID3D11Device * d3dDevice,
	void * data,
	UINT byteWidth,
	ID3D11Buffer ** rawBuffer,
	/* 可选扩展部分 */
	bool cpuUpdates = false,
	bool gpuUpdates = false);

//
// 纹理数组相关函数
//

// ------------------------------
// CreateRandomTexture1D函数
// ------------------------------
// 创建1D随机值向量纹理，范围在[-1.0f, 1.0f]
// [In]d3dDevice            D3D设备
// [OutOpt]texture		    输出的纹理资源
// [OutOpt]textureView      输出的纹理资源视图
HRESULT CreateRandomTexture1D(
	ID3D11Device* d3dDevice,
	ID3D11Texture1D** texture,
	ID3D11ShaderResourceView** textureView);


//
// 纹理数组相关函数
//

// ------------------------------
// CreateTexture2DArrayFromFile函数
// ------------------------------
// 该函数要求所有纹理的宽高、数据格式、mip等级一致
// [In]d3dDevice			D3D设备
// [In]d3dDeviceContext		D3D设备上下文
// [In]fileNames			dds文件名数组
// [OutOpt]textureArray		输出的纹理数组资源
// [OutOpt]textureArrayView 输出的纹理数组资源视图
// [In]generateMips			是否生成mipmaps
HRESULT CreateTexture2DArrayFromFile(
	ID3D11Device* d3dDevice,
	ID3D11DeviceContext* d3dDeviceContext,
	const std::vector<std::wstring>& fileNames,
	ID3D11Texture2D** textureArray,
	ID3D11ShaderResourceView** textureArrayView,
	bool generateMips = false);

//
// 纹理立方体相关函数
//


// ------------------------------
// CreateWICTexture2DCubeFromFile函数
// ------------------------------
// 根据给定的一张包含立方体六个面的位图，创建纹理立方体
// 要求纹理宽高比为4:3，且按下面形式布局:
// .  +Y .  .
// -X +Z +X -Z 
// .  -Y .  .
// [In]d3dDevice			D3D设备
// [In]d3dDeviceContext		D3D设备上下文
// [In]cubeMapFileName		位图文件名
// [OutOpt]textureArray		输出的纹理数组资源
// [OutOpt]textureCubeView	输出的纹理立方体资源视图
// [In]generateMips			是否生成mipmaps
HRESULT CreateWICTexture2DCubeFromFile(
	ID3D11Device * d3dDevice,
	ID3D11DeviceContext * d3dDeviceContext,
	const std::wstring& cubeMapFileName,
	ID3D11Texture2D** textureArray,
	ID3D11ShaderResourceView** textureCubeView,
	bool generateMips = false);

// ------------------------------
// CreateWICTexture2DCubeFromFile函数
// ------------------------------
// 根据按D3D11_TEXTURECUBE_FACE索引顺序给定的六张纹理，创建纹理立方体
// 要求位图是同样宽高、数据格式的正方形
// 你也可以给定超过6张的纹理，然后在获取到纹理数组的基础上自行创建更多的资源视图
// [In]d3dDevice			D3D设备
// [In]d3dDeviceContext		D3D设备上下文
// [In]cubeMapFileNames		位图文件名数组
// [OutOpt]textureArray		输出的纹理数组资源
// [OutOpt]textureCubeView	输出的纹理立方体资源视图
// [In]generateMips			是否生成mipmaps
HRESULT CreateWICTexture2DCubeFromFile(
	ID3D11Device * d3dDevice,
	ID3D11DeviceContext * d3dDeviceContext,
	const std::vector<std::wstring>& cubeMapFileNames,
	ID3D11Texture2D** textureArray,
	ID3D11ShaderResourceView** textureCubeView,
	bool generateMips = false);

//
// 数学相关函数
//

// ------------------------------
// InverseTranspose函数
// ------------------------------
inline DirectX::XMMATRIX XM_CALLCONV InverseTranspose(DirectX::FXMMATRIX M)
{
	using namespace DirectX;

	// 世界矩阵的逆的转置仅针对法向量，我们也不需要世界矩阵的平移分量
	// 而且不去掉的话，后续再乘上观察矩阵之类的就会产生错误的变换结果
	XMMATRIX A = M;
	A.r[3] = g_XMIdentityR3;

	return XMMatrixTranspose(XMMatrixInverse(nullptr, A));
}

#endif
