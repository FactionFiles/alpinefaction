#include <windows.h>
#include <d3d8.h>
#include "VideoDeviceInfoProvider.h"
#include <common/error/Exception.h>
#include <common/error/Win32Error.h>
#include <common/DynamicLinkLibrary.h>
#include <common/ComPtr.h>
#include <xlog/xlog.h>

class VideoDeviceInfoProviderD3D8 : public VideoDeviceInfoProvider
{
public:
    VideoDeviceInfoProviderD3D8();
    ~VideoDeviceInfoProviderD3D8() override = default;
    std::vector<std::string> get_adapters() override;
    std::set<Resolution> get_resolutions(unsigned adapter, unsigned format) override;
    std::set<unsigned> get_multisample_types(unsigned adapter, unsigned format, bool windowed) override;
    bool has_anisotropy_support(unsigned adapter) override;

    unsigned get_format_from_bpp(unsigned bpp) override
    {
        switch (bpp) {
            case 16: return static_cast<unsigned>(D3DFMT_R5G6B5);
            default: return static_cast<unsigned>(D3DFMT_X8R8G8B8);
        }
    }

private:
    DynamicLinkLibrary m_lib;
    ComPtr<IDirect3D8> m_d3d;
};

VideoDeviceInfoProviderD3D8::VideoDeviceInfoProviderD3D8() :
    m_lib{L"d3d8.dll"}
{
    if (!m_lib)
        THROW_WIN32_ERROR("Failed to load d3d8.dll");

    auto pDirect3DCreate8 = m_lib.get_proc_address<decltype(Direct3DCreate8)*>("Direct3DCreate8");
    if (!pDirect3DCreate8)
        THROW_WIN32_ERROR("Failed to load get Direct3DCreate8 function address");

    m_d3d = pDirect3DCreate8(D3D_SDK_VERSION);
    if (!m_d3d)
        THROW_EXCEPTION("Direct3DCreate8 failed");
}

std::vector<std::string> VideoDeviceInfoProviderD3D8::get_adapters()
{
    std::vector<std::string> adapters;
    auto count = m_d3d->GetAdapterCount();
    for (unsigned i = 0; i < count; ++i) {
        D3DADAPTER_IDENTIFIER8 adapter_identifier;
        auto hr = m_d3d->GetAdapterIdentifier(i, 0, &adapter_identifier);
        if (SUCCEEDED(hr)) {
            adapters.emplace_back(adapter_identifier.Description);
        }
        else {
            xlog::error("GetAdapterIdentifier failed %lx", hr);
        }
    }
    return adapters;
}

std::set<VideoDeviceInfoProvider::Resolution> VideoDeviceInfoProviderD3D8::get_resolutions(unsigned adapter, unsigned format)
{
    std::set<Resolution> result;
    unsigned mode_idx = 0;
    while (true) {
        D3DDISPLAYMODE d3d_display_mode;
        HRESULT hr = m_d3d->EnumAdapterModes(adapter, mode_idx++, &d3d_display_mode);
        if (hr == D3DERR_INVALIDCALL)
            break;
        if (FAILED(hr))
            THROW_EXCEPTION("EnumAdapterModes failed: %lx", hr);

        if (d3d_display_mode.Format != format)
            continue;

        result.insert({d3d_display_mode.Width, d3d_display_mode.Height});
    }

    return result;
}

std::set<unsigned> VideoDeviceInfoProviderD3D8::get_multisample_types(unsigned adapter, unsigned format, bool windowed)
{
    std::set<unsigned> result;
    for (unsigned i = 2; i < 16; ++i) {
        HRESULT hr = m_d3d->CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, static_cast<D3DFORMAT>(format), windowed,
            static_cast<D3DMULTISAMPLE_TYPE>(i));
        if (SUCCEEDED(hr))
            result.insert(i);
    }
    return result;
}

bool VideoDeviceInfoProviderD3D8::has_anisotropy_support(unsigned adapter)
{
    D3DCAPS8 caps;
    HRESULT hr = m_d3d->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps);
    if (FAILED(hr))
        THROW_EXCEPTION("GetDeviceCaps failed, hresult %lx", hr);

    bool anisotropy_cap = (caps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY) != 0;
    return anisotropy_cap && caps.MaxAnisotropy > 0;
}

std::unique_ptr<VideoDeviceInfoProvider> create_d3d8_device_info_provider()
{
    return std::make_unique<VideoDeviceInfoProviderD3D8>();
}
