#include <Effect/SkyRender.h>
#include <Utils/Geometry.h>
#include <Utils/d3dUtil.h>

#pragma warning(disable: 26812)

using namespace DirectX;
using namespace Microsoft::WRL;

HRESULT SkyRender::InitResource(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::wstring& cubemapFilename, float skySphereRadius, bool generateMips)
{
	// 防止重复初始化造成内存泄漏
	m_pIndexBuffer.Reset();
	m_pVertexBuffer.Reset();
	m_pTextureCubeSRV.Reset();

	HRESULT hr;
	// 天空盒纹理加载
	if (cubemapFilename.substr(cubemapFilename.size() - 3) == L"dds")
	{
		hr = CreateDDSTextureFromFile(device,
			generateMips ? deviceContext : nullptr,
			cubemapFilename.c_str(),
			nullptr,
			m_pTextureCubeSRV.GetAddressOf());
	}
	else
	{
		hr = CreateWICTexture2DCubeFromFile(device,
			deviceContext,
			cubemapFilename,
			nullptr,
			m_pTextureCubeSRV.GetAddressOf(),
			generateMips);
	}

	if (FAILED(hr))
		return hr;

	return InitResource(device, skySphereRadius);
}

HRESULT SkyRender::InitResource(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::vector<std::wstring>& cubemapFilenames, float skySphereRadius, bool generateMips)
{
	// 防止重复初始化造成内存泄漏
	m_pIndexBuffer.Reset();
	m_pVertexBuffer.Reset();
	m_pTextureCubeSRV.Reset();

	HRESULT hr;
	// 天空盒纹理加载
	hr = CreateWICTexture2DCubeFromFile(device,
		deviceContext,
		cubemapFilenames,
		nullptr,
		m_pTextureCubeSRV.GetAddressOf(),
		generateMips);
	if (FAILED(hr))
		return hr;

	return InitResource(device, skySphereRadius);
}

ID3D11ShaderResourceView* SkyRender::GetTextureCube()
{
	return m_pTextureCubeSRV.Get();
}

void SkyRender::Draw(ID3D11DeviceContext* deviceContext, SkyEffect& skyEffect, const Camera& camera)
{
	UINT strides[1] = { sizeof(XMFLOAT3) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// 抹除平移分量，避免摄像机移动带来天空盒抖动
	XMMATRIX V = camera.GetViewXM();
	V.r[3] = g_XMIdentityR3;

	skyEffect.SetWorldMatrix(XMMatrixIdentity());
	skyEffect.SetViewMatrix(V);
	skyEffect.SetProjMatrix(camera.GetProjXM());
	
	skyEffect.SetTextureCube(m_pTextureCubeSRV.Get());
	skyEffect.Apply(deviceContext);
	deviceContext->DrawIndexed(m_IndexCount, 0, 0);
}

void SkyRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	// 先清空可能存在的名称
	D3D11SetDebugObjectName(m_pTextureCubeSRV.Get(), nullptr);

	D3D11SetDebugObjectName(m_pTextureCubeSRV.Get(), name + ".CubeMapSRV");
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");
#else
	UNREFERENCED_PARAMETER(name);
#endif
}

HRESULT SkyRender::InitResource(ID3D11Device* device, float skySphereRadius)
{
	HRESULT hr;
	auto sphere = Geometry::CreateSphere<VertexPos>(skySphereRadius);

	// 顶点缓冲区创建
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(XMFLOAT3) * (UINT)sphere.vertexVec.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = sphere.vertexVec.data();

	hr = device->CreateBuffer(&vbd, &InitData, &m_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// 索引缓冲区创建
	m_IndexCount = (UINT)sphere.indexVec.size();

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(DWORD) * m_IndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.StructureByteStride = 0;
	ibd.MiscFlags = 0;

	InitData.pSysMem = sphere.indexVec.data();

	return device->CreateBuffer(&ibd, &InitData, &m_pIndexBuffer);
}

HRESULT DynamicSkyRender::InitResource(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::wstring& cubemapFilename,
	float skySphereRadius, int dynamicCubeSize, bool generateMips)
{
	// 防止重复初始化造成内存泄漏
	m_pCacheRTV.Reset();
	m_pCacheDSV.Reset();
	m_pDynamicCubeMapDSV.Reset();
	m_pDynamicCubeMapSRV.Reset();
	for (auto& ptr : m_pDynamicCubeMapRTVs)
	{
		ptr.Reset();
	}

	HRESULT hr;
	hr = SkyRender::InitResource(device, deviceContext, cubemapFilename, skySphereRadius, generateMips);
	if (FAILED(hr))
		return hr;
	return DynamicSkyRender::InitResource(device, dynamicCubeSize);
}

HRESULT DynamicSkyRender::InitResource(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::vector<std::wstring>& cubemapFilenames,
	float skySphereRadius, int dynamicCubeSize, bool generateMips)
{
	// 防止重复初始化造成内存泄漏
	m_pCacheRTV.Reset();
	m_pCacheDSV.Reset();
	m_pDynamicCubeMapDSV.Reset();
	m_pDynamicCubeMapSRV.Reset();
	for (auto& ptr : m_pDynamicCubeMapRTVs)
	{
		ptr.Reset();
	}

	HRESULT hr;
	hr = SkyRender::InitResource(device, deviceContext, cubemapFilenames, skySphereRadius, generateMips);
	if (FAILED(hr))
		return hr;
	return DynamicSkyRender::InitResource(device, dynamicCubeSize);
}

void DynamicSkyRender::Cache(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
{
	deviceContext->OMGetRenderTargets(1, m_pCacheRTV.GetAddressOf(), m_pCacheDSV.GetAddressOf());

	// 清掉绑定在着色器的动态天空盒，需要立即生效
	effect.SetTextureCube(nullptr);
	effect.Apply(deviceContext);
}

void DynamicSkyRender::BeginCapture(ID3D11DeviceContext* deviceContext, BasicEffect& effect, const XMFLOAT3& pos,
	D3D11_TEXTURECUBE_FACE face, float nearZ, float farZ)
{
	static XMFLOAT3 ups[6] = {
		{ 0.0f, 1.0f, 0.0f },	// +X
		{ 0.0f, 1.0f, 0.0f },	// -X
		{ 0.0f, 0.0f, -1.0f },	// +Y
		{ 0.0f, 0.0f, 1.0f },	// -Y
		{ 0.0f, 1.0f, 0.0f },	// +Z
		{ 0.0f, 1.0f, 0.0f }	// -Z
	};

	static XMFLOAT3 looks[6] = {
		{ 1.0f, 0.0f, 0.0f },	// +X
		{ -1.0f, 0.0f, 0.0f },	// -X
		{ 0.0f, 1.0f, 0.0f },	// +Y
		{ 0.0f, -1.0f, 0.0f },	// -Y
		{ 0.0f, 0.0f, 1.0f },	// +Z
		{ 0.0f, 0.0f, -1.0f },	// -Z
	};

	// 设置天空盒摄像机
	m_pCamera.LookTo(pos, looks[face], ups[face]);

	// 这里尽可能捕获近距离物体
	m_pCamera.SetFrustum(XM_PIDIV2, 1.0f, nearZ, farZ);

	// 应用观察矩阵、投影矩阵
	effect.SetViewMatrix(m_pCamera.GetViewXM());
	effect.SetProjMatrix(m_pCamera.GetProjXM());

	// 清空缓冲区
	deviceContext->ClearRenderTargetView(m_pDynamicCubeMapRTVs[face].Get(), reinterpret_cast<const float*>(&Colors::Black));
	deviceContext->ClearDepthStencilView(m_pDynamicCubeMapDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	// 设置渲染目标和深度模板视图
	deviceContext->OMSetRenderTargets(1, m_pDynamicCubeMapRTVs[face].GetAddressOf(), m_pDynamicCubeMapDSV.Get());
	// 设置视口
	D3D11_VIEWPORT viewPort = m_pCamera.GetViewPort();
	deviceContext->RSSetViewports(1, &viewPort);
}



void DynamicSkyRender::Restore(ID3D11DeviceContext* deviceContext, BasicEffect& effect, const Camera& camera)
{
	// 恢复默认设定
	D3D11_VIEWPORT viewPort = camera.GetViewPort();
	deviceContext->RSSetViewports(1, &viewPort);
	deviceContext->OMSetRenderTargets(1, m_pCacheRTV.GetAddressOf(), m_pCacheDSV.Get());

	// 生成动态天空盒后必须要生成mipmap链
	deviceContext->GenerateMips(m_pDynamicCubeMapSRV.Get());

	effect.SetViewMatrix(camera.GetViewXM());
	effect.SetProjMatrix(camera.GetProjXM());
	// 恢复绑定的动态天空盒
	effect.SetTextureCube(m_pDynamicCubeMapSRV.Get());

	// 清空临时缓存的渲染目标视图和深度模板视图
	m_pCacheDSV.Reset();
	m_pCacheRTV.Reset();
}

ID3D11ShaderResourceView* DynamicSkyRender::GetDynamicTextureCube()
{
	return m_pDynamicCubeMapSRV.Get();
}

const Camera& DynamicSkyRender::GetCamera() const
{
	return m_pCamera;
}

void DynamicSkyRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	SkyRender::SetDebugObjectName(name);
	D3D11SetDebugObjectName(m_pDynamicCubeMapSRV.Get(), name + ".dynamicCubeMapSRV");
	D3D11SetDebugObjectName(m_pDynamicCubeMapDSV.Get(), name + ".dynamicCubeMapDSV");
	for (size_t i = 0; i < 6; ++i)
	{
		D3D11SetDebugObjectName(m_pDynamicCubeMapRTVs[i].Get(), name + ".dynamicCubeMapRTVs[" + std::to_string(i) + "]");
	}
#else
	UNREFERENCED_PARAMETER(name);
#endif
}

HRESULT DynamicSkyRender::InitResource(ID3D11Device* device, int dynamicCubeSize)
{
	HRESULT hr;
	// ******************
	// 1. 创建纹理数组
	//

	ComPtr<ID3D11Texture2D> texCube;
	D3D11_TEXTURE2D_DESC texDesc;

	texDesc.Width = dynamicCubeSize;
	texDesc.Height = dynamicCubeSize;
	texDesc.MipLevels = 0;
	texDesc.ArraySize = 6;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;

	// 现在texCube用于新建纹理
	hr = device->CreateTexture2D(&texDesc, nullptr, texCube.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// ******************
	// 2. 创建渲染目标视图
	//

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.MipSlice = 0;
	// 一个视图只对应一个纹理数组元素
	rtvDesc.Texture2DArray.ArraySize = 1;

	// 每个元素创建一个渲染目标视图
	for (int i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		hr = device->CreateRenderTargetView(texCube.Get(), &rtvDesc,
			m_pDynamicCubeMapRTVs[i].GetAddressOf());
		if (FAILED(hr))
			return hr;
	}

	// ******************
	// 3. 创建着色器资源视图
	//

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = -1;	// 使用所有的mip等级

	hr = device->CreateShaderResourceView(texCube.Get(), &srvDesc,
		m_pDynamicCubeMapSRV.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// ******************
	// 4. 创建深度/模板缓冲区与对应的视图
	//

	texDesc.Width = dynamicCubeSize;
	texDesc.Height = dynamicCubeSize;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Format = DXGI_FORMAT_D32_FLOAT;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> depthTex;
	hr = device->CreateTexture2D(&texDesc, nullptr, depthTex.GetAddressOf());
	if (FAILED(hr))
		return hr;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = texDesc.Format;
	dsvDesc.Flags = 0;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	hr = device->CreateDepthStencilView(
		depthTex.Get(),
		&dsvDesc,
		m_pDynamicCubeMapDSV.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// ******************
	// 5. 初始化视口
	//

	m_pCamera.SetViewPort(0.0f, 0.0f, static_cast<float>(dynamicCubeSize), static_cast<float>(dynamicCubeSize));

	return S_OK;
}
