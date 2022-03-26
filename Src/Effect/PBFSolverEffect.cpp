#include <Graphics/Vertex.h>
#include <Effect/Effects.h>
#include <Utils/d3dUtil.h>
#include <Effect/EffectHelper.h>	// 必须晚于Effects.h和d3dUtil.h包含
#include <Utils/DXTrace.h>

using namespace DirectX;
# pragma warning(disable: 26812)

class PBFSolverEffect::Impl
{
public:
	// 必须显式指定
	Impl() {}
	~Impl() = default;

public:
	std::unique_ptr<EffectHelper> m_pEffectHelper;

	std::shared_ptr<IEffectPass> m_pCurrEffectPass;
};


PBFSolverEffect::PBFSolverEffect()
{
	m_pImpl = std::make_unique<PBFSolverEffect::Impl>();
}


PBFSolverEffect::~PBFSolverEffect()
{

}

PBFSolverEffect::PBFSolverEffect(PBFSolverEffect&& moveForm) noexcept
{
	this->m_pImpl.swap(moveForm.m_pImpl);
}

PBFSolverEffect& PBFSolverEffect::operator=(PBFSolverEffect&& moveFrom) noexcept
{
	this->m_pImpl.swap(moveFrom.m_pImpl);
	return *this;
}

bool PBFSolverEffect::Init(ID3D11Device* device, const std::wstring& effectPath)
{
	if (!device)
		return false;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	m_pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();
	return true;
}

void PBFSolverEffect::Apply(ID3D11DeviceContext* deviceContext)
{
	if (m_pImpl->m_pCurrEffectPass)
		m_pImpl->m_pCurrEffectPass->Apply(deviceContext);
}
