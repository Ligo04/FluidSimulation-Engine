#ifndef BUFFER_H
#define BUFFER_H

#include <d3d11_1.h>
#include <wrl/client.h>
#include <string>

// 注意：确保T与着色器中结构体的大小/布局相同
template <class T> class StructuredBuffer
{
    public:
        StructuredBuffer(ID3D11Device *d3dDevice,
                         UINT          elements,
                         UINT          bindFlags  = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
                         bool          cpuUpdates = false);

        //provide a construct function by init data
        StructuredBuffer(ID3D11Device *d3dDevice,
                         UINT          elements,
                         void         *data,
                         UINT          bindFlags  = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
                         bool          cpuUpdates = false);

        ~StructuredBuffer()                                       = default;

        // 不允许拷贝，允许移动
        StructuredBuffer(const StructuredBuffer &)                = delete;
        StructuredBuffer &operator=(const StructuredBuffer &)     = delete;
        StructuredBuffer(StructuredBuffer &&)                     = default;
        StructuredBuffer          &operator=(StructuredBuffer &&) = default;

        ID3D11Buffer              *GetBuffer() { return m_pBuffer.Get(); }
        ID3D11UnorderedAccessView *GetUnorderedAccess() { return m_pUnorderedAccess.Get(); }
        ID3D11ShaderResourceView  *GetShaderResource() { return m_pShaderResource.Get(); }

        // 仅支持动态缓冲区
        // TODO: Support NOOVERWRITE ring buffer?
        T   *MapDiscard(ID3D11DeviceContext *d3dDeviceContext);
        void Unmap(ID3D11DeviceContext *d3dDeviceContext);
        //update
        void UpdateBufferCPU(ID3D11DeviceContext *d3dDeviceContext, T *data);
        void UpdateBufferCPU(ID3D11DeviceContext *d3dDeviceContext, T *data, UINT num);
        void UpdataBufferGPU(ID3D11DeviceContext *d3dDeviceContext, ID3D11Buffer *databuffer);
        void UpdataBufferGPU(ID3D11DeviceContext *d3dDeviceContext, ID3D11Buffer *databuffer, UINT num);

        void ClearUAV(ID3D11DeviceContext *d3dDeviceContext);
        // 设置调试对象名
        void SetDebugObjectName(const std::string &name);

    private:
        template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

        UINT                              m_Elements;
        ComPtr<ID3D11Buffer>              m_pBuffer;
        ComPtr<ID3D11ShaderResourceView>  m_pShaderResource;
        ComPtr<ID3D11UnorderedAccessView> m_pUnorderedAccess;
};

template <class T>
StructuredBuffer<T>::StructuredBuffer(ID3D11Device *d3dDevice, UINT elements, UINT bindFlags, bool cpuUpdates) :
    m_Elements(elements)
{
    D3D11_USAGE usage{};
    UINT        cpuAccessFlags = 0;

    if ((bindFlags & D3D11_BIND_UNORDERED_ACCESS) && cpuUpdates)
    {
        //绑定UAV
        //同时允许CPUupdate
        bindFlags = 0;
        usage     = D3D11_USAGE_STAGING;
        cpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    }
    else if (cpuUpdates)
    {
        //只绑定为SRV并且允许cpuUpdates
        usage = D3D11_USAGE_DYNAMIC;
        cpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        usage = D3D11_USAGE_DEFAULT;
    }

    CD3D11_BUFFER_DESC desc(sizeof(T) * elements,
                            bindFlags,
                            usage,
                            cpuAccessFlags,
                            D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
                            sizeof(T));

    d3dDevice->CreateBuffer(&desc, nullptr, m_pBuffer.GetAddressOf());

    if (bindFlags & D3D11_BIND_UNORDERED_ACCESS)
    {
        d3dDevice->CreateUnorderedAccessView(m_pBuffer.Get(), 0, m_pUnorderedAccess.GetAddressOf());
    }

    if (bindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        d3dDevice->CreateShaderResourceView(m_pBuffer.Get(), 0, m_pShaderResource.GetAddressOf());
    }
}

template <class T>
StructuredBuffer<T>::StructuredBuffer(ID3D11Device *d3dDevice,
                                      UINT          elements,
                                      void         *data,
                                      UINT          bindFlags,
                                      bool          cpuUpdates) :
    m_Elements(elements)
{
    D3D11_USAGE usage{};
    UINT        cpuAccessFlags = 0;

    if ((bindFlags & D3D11_BIND_UNORDERED_ACCESS) && cpuUpdates)
    {
        //绑定UAV
        //同时允许CPUupdate
        bindFlags = 0;
        usage     = D3D11_USAGE_STAGING;
        cpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    }
    else if (cpuUpdates)
    {
        //只绑定为SRV并且允许cpuUpdates
        usage = D3D11_USAGE_DYNAMIC;
        cpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        usage = D3D11_USAGE_DEFAULT;
    }

    CD3D11_BUFFER_DESC     desc(sizeof(T) * elements,
                            bindFlags,
                            usage,
                            cpuAccessFlags,
                            D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
                            sizeof(T));

    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(initData));
    initData.pSysMem = data;

    d3dDevice->CreateBuffer(&desc, &initData, m_pBuffer.GetAddressOf());

    if (bindFlags & D3D11_BIND_UNORDERED_ACCESS)
    {
        d3dDevice->CreateUnorderedAccessView(m_pBuffer.Get(), 0, m_pUnorderedAccess.GetAddressOf());
    }

    if (bindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        d3dDevice->CreateShaderResourceView(m_pBuffer.Get(), 0, m_pShaderResource.GetAddressOf());
    }
}

template <typename T> T *StructuredBuffer<T>::MapDiscard(ID3D11DeviceContext *d3dDeviceContext)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    d3dDeviceContext->Map(m_pBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    return static_cast<T *>(mappedResource.pData);
}

template <typename T> void StructuredBuffer<T>::Unmap(ID3D11DeviceContext *d3dDeviceContext)
{
    d3dDeviceContext->Unmap(m_pBuffer.Get(), 0);
}

template <class T> void StructuredBuffer<T>::UpdateBufferCPU(ID3D11DeviceContext *d3dDeviceContext, T *data)
{
    auto dst = MapDiscard(d3dDeviceContext);
    memcpy(dst, data, sizeof(T) * m_Elements);
    Unmap(d3dDeviceContext);
}

template <class T> void StructuredBuffer<T>::UpdateBufferCPU(ID3D11DeviceContext *d3dDeviceContext, T *data, UINT num)
{
    auto dst = MapDiscard(d3dDeviceContext);
    memcpy(dst, data, sizeof(T) * num);
    Unmap(d3dDeviceContext);
}

template <class T>
void StructuredBuffer<T>::UpdataBufferGPU(ID3D11DeviceContext *d3dDeviceContext, ID3D11Buffer *databuffer)
{
    d3dDeviceContext->CopyResource(m_pBuffer.Get(), databuffer);
}

template <class T>
void StructuredBuffer<T>::UpdataBufferGPU(ID3D11DeviceContext *d3dDeviceContext, ID3D11Buffer *databuffer, UINT num)
{
    CD3D11_BOX sourceBox(0, 0, 0, num * sizeof(T), 1, 1);
    d3dDeviceContext->CopySubresourceRegion(m_pBuffer.Get(), 0, 0, 0, 0, databuffer, 0, &sourceBox);
}

template <class T> void StructuredBuffer<T>::ClearUAV(ID3D11DeviceContext *d3dDeviceContext)
{
    UINT zero = 0;
    d3dDeviceContext->ClearUnorderedAccessViewUint(m_pUnorderedAccess.Get(), &zero);
}

template <class T> inline void StructuredBuffer<T>::SetDebugObjectName(const std::string &name)
{
#if (defined(DEBUG) || defined(_DEBUG))

    m_pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.length()), name.c_str());
    if (m_pShaderResource)
    {
        std::string srvName = name + ".SRV";
        m_pShaderResource->SetPrivateData(WKPDID_D3DDebugObjectName,
                                          static_cast<UINT>(srvName.length()),
                                          srvName.c_str());
    }
    if (m_pUnorderedAccess)
    {
        std::string uavName = name + ".UAV";
        m_pUnorderedAccess->SetPrivateData(WKPDID_D3DDebugObjectName,
                                           static_cast<UINT>(uavName.length()),
                                           uavName.c_str());
    }

#else
    UNREFERENCED_PARAMETER(name);
#endif
}

// 注意：确保T与着色器中结构体的大小/布局相同
template <class T> class Buffer
{
    public:
        Buffer(ID3D11Device *d3dDevice,
               UINT          elements,
               DXGI_FORMAT   format,
               UINT          bindFlags  = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
               bool          cpuUpdates = false);
        ~Buffer()                                       = default;

        // 不允许拷贝，允许移动
        Buffer(const Buffer &)                          = delete;
        Buffer &operator=(const Buffer &)               = delete;
        Buffer(Buffer &&)                               = default;
        Buffer                    &operator=(Buffer &&) = default;

        ID3D11Buffer              *GetBuffer() { return m_pBuffer.Get(); }
        ID3D11UnorderedAccessView *GetUnorderedAccess() { return m_pUnorderedAccess.Get(); }
        ID3D11ShaderResourceView  *GetShaderResource() { return m_pShaderResource.Get(); }

        // 仅支持动态缓冲区
        // TODO: Support NOOVERWRITE ring buffer?
        T   *MapDiscard(ID3D11DeviceContext *d3dDeviceContext);
        void Unmap(ID3D11DeviceContext *d3dDeviceContext);

        // 设置调试对象名
        void SetDebugObjectName(const std::string &name);

    private:
        template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

        UINT                              m_Elements;
        ComPtr<ID3D11Buffer>              m_pBuffer;
        ComPtr<ID3D11ShaderResourceView>  m_pShaderResource;
        ComPtr<ID3D11UnorderedAccessView> m_pUnorderedAccess;
};

template <class T>
inline Buffer<T>::Buffer(ID3D11Device *d3dDevice, UINT elements, DXGI_FORMAT format, UINT bindFlags, bool cpuUpdates) :
    m_Elements(elements)
{
    D3D11_USAGE usage{};
    UINT        cpuAccessFlags = 0;

    if ((bindFlags & D3D11_BIND_UNORDERED_ACCESS) && cpuUpdates)
    {
        //绑定UAV
        //同时允许CPUupdate
        bindFlags = 0;
        usage     = D3D11_USAGE_STAGING;
        cpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    }
    else if (cpuUpdates)
    {
        //只绑定为SRV并且允许cpuUpdates
        usage = D3D11_USAGE_DYNAMIC;
        cpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        usage = D3D11_USAGE_DEFAULT;
    }

    CD3D11_BUFFER_DESC desc(sizeof(T) * elements, bindFlags, usage, cpuAccessFlags, 0, 0);

    d3dDevice->CreateBuffer(&desc, nullptr, m_pBuffer.GetAddressOf());

    if (bindFlags & D3D11_BIND_UNORDERED_ACCESS)
    {
        CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc(D3D11_UAV_DIMENSION_BUFFER, format, 0, elements);
        d3dDevice->CreateUnorderedAccessView(m_pBuffer.Get(), &uavDesc, m_pUnorderedAccess.GetAddressOf());
    }

    if (bindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_BUFFER, format, 0, elements);
        d3dDevice->CreateShaderResourceView(m_pBuffer.Get(), &srvDesc, m_pShaderResource.GetAddressOf());
    }
}

template <class T> T *Buffer<T>::MapDiscard(ID3D11DeviceContext *d3dDeviceContext)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    d3dDeviceContext->Map(m_pBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    return static_cast<T *>(mappedResource.pData);
}

template <class T> void Buffer<T>::Unmap(ID3D11DeviceContext *d3dDeviceContext)
{
    d3dDeviceContext->Unmap(m_pBuffer.Get(), 0);
}

template <class T> inline void Buffer<T>::SetDebugObjectName(const std::string &name)
{
#if (defined(DEBUG) || defined(_DEBUG))

    m_pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.length()), name.c_str());
    if (m_pShaderResource)
    {
        std::string srvName = name + ".SRV";
        m_pShaderResource->SetPrivateData(WKPDID_D3DDebugObjectName,
                                          static_cast<UINT>(srvName.length()),
                                          srvName.c_str());
    }
    if (m_pUnorderedAccess)
    {
        std::string uavName = name + ".UAV";
        m_pShaderResource->SetPrivateData(WKPDID_D3DDebugObjectName,
                                          static_cast<UINT>(uavName.length()),
                                          uavName.c_str());
    }

#else
    UNREFERENCED_PARAMETER(name);
#endif
}

#endif