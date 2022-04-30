#include <Graphics/RenderStates.h>
#include <Utils/d3dUtil.h>
#include <Utils/DXTrace.h>

using namespace Microsoft::WRL;

ComPtr<ID3D11RasterizerState> RenderStates::RSNoCull			= nullptr;
ComPtr<ID3D11RasterizerState> RenderStates::RSWireframe			= nullptr;
ComPtr<ID3D11RasterizerState> RenderStates::RSCullClockWise		= nullptr;
ComPtr<ID3D11RasterizerState> RenderStates::RSDepth				= nullptr;

ComPtr<ID3D11SamplerState> RenderStates::SSPointClamp			= nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSAnistropicWrap		= nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSLinearWrap			= nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSShadow				= nullptr;
ComPtr<ID3D11SamplerState> RenderStates::SSClampWrap			= nullptr;


ComPtr<ID3D11BlendState> RenderStates::BSAlphaToCoverage		= nullptr;
ComPtr<ID3D11BlendState> RenderStates::BSNoColorWrite			= nullptr;
ComPtr<ID3D11BlendState> RenderStates::BSTransparent			= nullptr;
ComPtr<ID3D11BlendState> RenderStates::BSAdditive				= nullptr;
ComPtr<ID3D11BlendState> RenderStates::BSAlphaWeightedAdditive  = nullptr;

ComPtr<ID3D11DepthStencilState> RenderStates::DSSEqual          = nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSLessEqual		= nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSWriteStencil	= nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSDrawWithStencil= nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSNoDoubleBlend	= nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSNoDepthTest	= nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSNoDepthWrite	= nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSNoDepthTestWithStencil		= nullptr;
ComPtr<ID3D11DepthStencilState> RenderStates::DSSNoDepthWriteWithStencil	= nullptr;

bool RenderStates::IsInit()
{
	// 一般来说初始化操作会把所有的状态都创建出来
	return RSWireframe != nullptr;
}

void RenderStates::InitAll(ID3D11Device * device)
{
	// 先前初始化过的话就没必要重来了
	if (IsInit())
		return;
	// ******************
	// 初始化光栅化器状态
	//
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));

	// 线框模式
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthClipEnable = true;
	HR(device->CreateRasterizerState(&rasterizerDesc, RSWireframe.GetAddressOf()));
	 
	// 无背面剔除模式
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthClipEnable = true;
	HR(device->CreateRasterizerState(&rasterizerDesc, RSNoCull.GetAddressOf()));

	// 顺时针剔除模式
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthClipEnable = true;
	HR(device->CreateRasterizerState(&rasterizerDesc, RSCullClockWise.GetAddressOf()));

	// 深度偏移模式
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.DepthBias = 100000;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 1.0f;
	HR(device->CreateRasterizerState(&rasterizerDesc, RSDepth.GetAddressOf()));

	// ******************
	// 初始化采样器状态
	//
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));

	// 点过滤与Clamp模式
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(device->CreateSamplerState(&sampDesc, SSPointClamp.GetAddressOf()));

	// 线性过滤与Wrap模式
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(device->CreateSamplerState(&sampDesc, SSLinearWrap.GetAddressOf()));

	// 各向异性过滤与Wrap模式
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MaxAnisotropy = 4;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(device->CreateSamplerState(&sampDesc, SSAnistropicWrap.GetAddressOf()));

	// 采样器状态：深度比较与Border模式
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	sampDesc.BorderColor[0] = { 1.0f };
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(device->CreateSamplerState(&sampDesc, SSShadow.GetAddressOf()));

	//采样器状态
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(device->CreateSamplerState(&sampDesc, SSClampWrap.GetAddressOf()));
	
	// ******************
	// 初始化混合状态
	//
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	auto& rtDesc = blendDesc.RenderTarget[0];
	// Alpha-To-Coverage模式
	blendDesc.AlphaToCoverageEnable = true;
	blendDesc.IndependentBlendEnable = false;
	rtDesc.BlendEnable = false;
	rtDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HR(device->CreateBlendState(&blendDesc, BSAlphaToCoverage.GetAddressOf()));

	// 透明混合模式
	// Color = SrcAlpha * SrcColor + (1 - SrcAlpha) * DestColor 
	// Alpha = SrcAlpha
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	rtDesc.BlendEnable = true;
	rtDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtDesc.BlendOp = D3D11_BLEND_OP_ADD;
	rtDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;

	HR(device->CreateBlendState(&blendDesc, BSTransparent.GetAddressOf()));
	
	// 加法混合模式
	// Color = SrcColor + DestColor
	// Alpha = SrcAlpha
	rtDesc.SrcBlend = D3D11_BLEND_ONE;
	rtDesc.DestBlend = D3D11_BLEND_ONE;
	rtDesc.BlendOp = D3D11_BLEND_OP_ADD;
	rtDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;

	HR(device->CreateBlendState(&blendDesc, BSAdditive.GetAddressOf()));

	// 带Alpha权重的加法混合模式
	// Color = SrcAlpha * SrcColor + DestColor
	// Alpha = SrcAlpha
	rtDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtDesc.DestBlend = D3D11_BLEND_ONE;
	rtDesc.BlendOp = D3D11_BLEND_OP_ADD;
	rtDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;

	HR(device->CreateBlendState(&blendDesc, BSAlphaWeightedAdditive.GetAddressOf()));

	// 无颜色写入混合模式
	// Color = DestColor
	// Alpha = DestAlpha
	rtDesc.BlendEnable = false;
	rtDesc.SrcBlend = D3D11_BLEND_ZERO;
	rtDesc.DestBlend = D3D11_BLEND_ONE;
	rtDesc.BlendOp = D3D11_BLEND_OP_ADD;
	rtDesc.SrcBlendAlpha = D3D11_BLEND_ZERO;
	rtDesc.DestBlendAlpha = D3D11_BLEND_ONE;
	rtDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtDesc.RenderTargetWriteMask = 0;
	HR(device->CreateBlendState(&blendDesc, BSNoColorWrite.GetAddressOf()));

	// ******************
	// 初始化深度/模板状态
	//
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof dsDesc);
	// 仅允许深度值一致的像素进行写入的深度/模板状态
	// 没必要写入深度
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_EQUAL;

	HR(device->CreateDepthStencilState(&dsDesc, DSSEqual.GetAddressOf()));

	// 允许使用深度值一致的像素进行替换的深度/模板状态
	// 该状态用于绘制天空盒，因为深度值为1.0时默认无法通过深度测试
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	HR(device->CreateDepthStencilState(&dsDesc, DSSLessEqual.GetAddressOf()));

	// 写入模板值的深度/模板状态
	// 这里不写入深度信息
	// 无论是正面还是背面，原来指定的区域的模板值都会被写入StencilRef
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// 对于背面的几何体我们是不进行渲染的，所以这里的设置无关紧要
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	HR(device->CreateDepthStencilState(&dsDesc, DSSWriteStencil.GetAddressOf()));

	// 对指定模板值进行绘制的深度/模板状态
	// 对满足模板值条件的区域才进行绘制，并更新深度
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	// 对于背面的几何体我们是不进行渲染的，所以这里的设置无关紧要
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	HR(device->CreateDepthStencilState(&dsDesc, DSSDrawWithStencil.GetAddressOf()));

	// 无二次混合深度/模板状态
	// 允许默认深度测试
	// 通过自递增使得原来StencilRef的值只能使用一次，实现仅一次混合
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	// 对于背面的几何体我们是不进行渲染的，所以这里的设置无关紧要
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	HR(device->CreateDepthStencilState(&dsDesc, DSSNoDoubleBlend.GetAddressOf()));

	// 关闭深度测试的深度/模板状态
	// 若绘制非透明物体，务必严格按照绘制顺序
	// 绘制透明物体则不需要担心绘制顺序
	// 而默认情况下模板测试就是关闭的
	dsDesc.DepthEnable = false;
	dsDesc.StencilEnable = false;

	HR(device->CreateDepthStencilState(&dsDesc, DSSNoDepthTest.GetAddressOf()));


	// 关闭深度测试
	// 若绘制非透明物体，务必严格按照绘制顺序
	// 绘制透明物体则不需要担心绘制顺序
	// 对满足模板值条件的区域才进行绘制
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	// 对于背面的几何体我们是不进行渲染的，所以这里的设置无关紧要
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	HR(device->CreateDepthStencilState(&dsDesc, DSSNoDepthTestWithStencil.GetAddressOf()));

	// 进行深度测试，但不写入深度值的状态
	// 若绘制非透明物体时，应使用默认状态
	// 绘制透明物体时，使用该状态可以有效确保混合状态的进行
	// 并且确保较前的非透明物体可以阻挡较后的一切物体
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = false;

	HR(device->CreateDepthStencilState(&dsDesc, DSSNoDepthWrite.GetAddressOf()));


	// 进行深度测试，但不写入深度值的状态
	// 若绘制非透明物体时，应使用默认状态
	// 绘制透明物体时，使用该状态可以有效确保混合状态的进行
	// 并且确保较前的非透明物体可以阻挡较后的一切物体
	// 对满足模板值条件的区域才进行绘制
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	// 对于背面的几何体我们是不进行渲染的，所以这里的设置无关紧要
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	HR(device->CreateDepthStencilState(&dsDesc, DSSNoDepthWriteWithStencil.GetAddressOf()));

	// ******************
	// 设置调试对象名
	//
	D3D11SetDebugObjectName(RSCullClockWise.Get(), "RSCullClockWise");
	D3D11SetDebugObjectName(RSNoCull.Get(), "RSNoCull");
	D3D11SetDebugObjectName(RSWireframe.Get(), "RSWireframe");
	D3D11SetDebugObjectName(RSDepth.Get(), "RSDepth");

	D3D11SetDebugObjectName(SSAnistropicWrap.Get(), "SSAnistropicWrap");
	D3D11SetDebugObjectName(SSLinearWrap.Get(), "SSLinearWrap");
	D3D11SetDebugObjectName(SSShadow.Get(), "SSShadow");

	D3D11SetDebugObjectName(BSAlphaToCoverage.Get(), "BSAlphaToCoverage");
	D3D11SetDebugObjectName(BSNoColorWrite.Get(), "BSNoColorWrite");
	D3D11SetDebugObjectName(BSTransparent.Get(), "BSTransparent");
	D3D11SetDebugObjectName(BSAdditive.Get(), "BSAdditive");

	D3D11SetDebugObjectName(DSSLessEqual.Get(), "DSSLessEqual");
	D3D11SetDebugObjectName(DSSWriteStencil.Get(), "DSSWriteStencil");
	D3D11SetDebugObjectName(DSSDrawWithStencil.Get(), "DSSDrawWithStencil");
	D3D11SetDebugObjectName(DSSNoDoubleBlend.Get(), "DSSNoDoubleBlend");
	D3D11SetDebugObjectName(DSSNoDepthTest.Get(), "DSSNoDepthTest");
	D3D11SetDebugObjectName(DSSNoDepthWrite.Get(), "DSSNoDepthWrite");
	D3D11SetDebugObjectName(DSSNoDepthTestWithStencil.Get(), "DSSNoDepthTestWithStencil");
	D3D11SetDebugObjectName(DSSNoDepthWriteWithStencil.Get(), "DSSNoDepthWriteWithStencil");
}
