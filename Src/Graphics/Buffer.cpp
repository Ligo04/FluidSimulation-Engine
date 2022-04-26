#include <Graphics/Buffer.h>
#include <Utils/d3dUtil.h>
#include <cassert>

#pragma warning(disable: 26812)

using namespace Microsoft::WRL;


void StructuredBuffer::InternalConsturct(ID3D11Device* d3dDevice, UINT size, DXGI_FORMAT format,UINT bindFlags)
{
	m_pBuffer.Reset();
	m_pShaderResourceViews.clear();
	m_p1UnorderedAccessView.clear();

	CD3D11_BUFFER_DESC desc{
		format,
		bindFlags,

	}
}
