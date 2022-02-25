#include <Hierarchy/GameObject.h>
#include <Utils/d3dUtil.h>
#include <Utils/DXTrace.h>
using namespace DirectX;

struct InstancedData
{
	XMMATRIX world;
	XMMATRIX worldInvTranspose;
};

Transform& GameObject::GetTransform()
{
	return m_Transform;
}

const Transform& GameObject::GetTransform() const
{
	return m_Transform;
}

BoundingBox GameObject::GetBoundingBox() const
{
	BoundingBox box;
	m_Model.boundingBox.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
	return box;
}

BoundingOrientedBox GameObject::GetBoundingOrientedBox() const
{
	BoundingOrientedBox box;
	BoundingOrientedBox::CreateFromBoundingBox(box, m_Model.boundingBox);
	box.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
	return box;
}


BoundingBox GameObject::GetLocalBoundingBox() const
{
	return m_Model.boundingBox;
}

size_t GameObject::GetCapacity() const
{
	return m_Capacity;
}

void GameObject::ResizeBuffer(ID3D11Device * device, size_t count)
{
	// 设置实例缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = (UINT)count * sizeof(InstancedData);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	// 创建实例缓冲区
	HR(device->CreateBuffer(&vbd, nullptr, m_pInstancedBuffer.ReleaseAndGetAddressOf()));

	// 重新调整m_Capacity
	m_Capacity = count;
}




void GameObject::SetModel(Model && model)
{
	std::swap(m_Model, model);
	model.modelParts.clear();
	model.boundingBox = BoundingBox();
}

void GameObject::SetModel(const Model & model)
{
	m_Model = model;
}

void GameObject::Draw(ID3D11DeviceContext * deviceContext, IEffect * effect)
{
	UINT strides = m_Model.vertexStride;
	UINT offsets = 0;

	for (auto& part : m_Model.modelParts)
	{
		// 设置顶点/索引缓冲区
		deviceContext->IASetVertexBuffers(0, 1, part.vertexBuffer.GetAddressOf(), &strides, &offsets);
		deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);

		// 更新数据并应用
		IEffectTransform* pEffectTransform = dynamic_cast<IEffectTransform*>(effect);
		if (pEffectTransform)
		{
			pEffectTransform->SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());
		}

		IEffectTextureDiffuse* pEffectTextureDiffuse = dynamic_cast<IEffectTextureDiffuse*>(effect);
		if (pEffectTextureDiffuse)
		{
			pEffectTextureDiffuse->SetTextureDiffuse(part.texDiffuse.Get());
		}

		IEffectTextureNormalMap* pEffectTextureNormalMap = dynamic_cast<IEffectTextureNormalMap*>(effect);
		if (pEffectTextureNormalMap)
		{
			pEffectTextureNormalMap->SetTextureNormalMap(part.texNormalMap.Get());
		}

		BasicEffect* pBasicEffect = dynamic_cast<BasicEffect*>(effect);
		if (pBasicEffect)
		{
			pBasicEffect->SetMaterial(part.material);
		}
		
		effect->Apply(deviceContext);

		deviceContext->DrawIndexed(part.indexCount, 0, 0);
	}
}

void GameObject::DrawInstanced(ID3D11DeviceContext* deviceContext, IEffect * effect, const std::vector<Transform> & data)
{
	D3D11_MAPPED_SUBRESOURCE mappedData;
	UINT numInsts = (UINT)data.size();
	// 若传入的数据比实例缓冲区还大，需要重新分配
	if (numInsts > m_Capacity)
	{
		ComPtr<ID3D11Device> device;
		deviceContext->GetDevice(device.GetAddressOf());
		ResizeBuffer(device.Get(), numInsts);
	}

	HR(deviceContext->Map(m_pInstancedBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	InstancedData* iter = reinterpret_cast<InstancedData*>(mappedData.pData);
	for (auto& t : data)
	{
		XMMATRIX W = t.GetLocalToWorldMatrixXM();
		iter->world = XMMatrixTranspose(W);
		iter->worldInvTranspose = InverseTranspose(W);
		iter++;
	}

	deviceContext->Unmap(m_pInstancedBuffer.Get(), 0);

	UINT strides[2] = { m_Model.vertexStride, sizeof(InstancedData) };
	UINT offsets[2] = { 0, 0 };
	ID3D11Buffer* buffers[2] = { nullptr, m_pInstancedBuffer.Get() };
	for (auto& part : m_Model.modelParts)
	{
		buffers[0] = part.vertexBuffer.Get();

		// 设置顶点/索引缓冲区
		deviceContext->IASetVertexBuffers(0, 2, buffers, strides, offsets);
		deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);

		// 更新数据并应用
		IEffectTransform* pEffectTransform = dynamic_cast<IEffectTransform*>(effect);
		if (pEffectTransform)
		{
			pEffectTransform->SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());
		}

		IEffectTextureDiffuse* pEffectTextureDiffuse = dynamic_cast<IEffectTextureDiffuse*>(effect);
		if (pEffectTextureDiffuse)
		{
			pEffectTextureDiffuse->SetTextureDiffuse(part.texDiffuse.Get());
		}

		IEffectTextureNormalMap* pEffectTextureNormalMap = dynamic_cast<IEffectTextureNormalMap*>(effect);
		if (pEffectTextureNormalMap)
		{
			pEffectTextureNormalMap->SetTextureNormalMap(part.texNormalMap.Get());
		}

		BasicEffect* pBasicEffect = dynamic_cast<BasicEffect*>(effect);
		if (pBasicEffect)
		{
			pBasicEffect->SetMaterial(part.material);
		}

		effect->Apply(deviceContext);

		deviceContext->DrawIndexedInstanced(part.indexCount, numInsts, 0, 0, 0);
	}
}


void GameObject::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)

	m_Model.SetDebugObjectName(name);
	if (m_pInstancedBuffer)
	{
		D3D11SetDebugObjectName(m_pInstancedBuffer.Get(), name + ".InstancedBuffer");
	}

#endif
}


