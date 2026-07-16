#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <wincodec.h>
#include <winhttp.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <cstdint>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace AvatarCache {

    struct Entry {
        ID3D11ShaderResourceView* srv = nullptr;
        int w = 0;
        int h = 0;
        std::atomic<int> state{ 0 }; // 0 none, 1 loading, 2 ready, 3 fail
        std::vector<uint8_t> pendingPng;
        std::atomic<bool> pngReady{ false };
    };

    inline ID3D11Device* gDevice = nullptr;
    inline std::mutex gMutex;
    inline std::unordered_map<int64_t, Entry*> gMap;

    inline void Init(ID3D11Device* device) {
        gDevice = device;
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    }

    inline std::string HttpGet(const std::wstring& host, const std::wstring& path)
    {
        std::string out;
        HINTERNET session = WinHttpOpen(L"Fade/1.5", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!session) return out;

        HINTERNET connect = WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!connect) { WinHttpCloseHandle(session); return out; }

        HINTERNET request = WinHttpOpenRequest(connect, L"GET", path.c_str(), nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!request) {
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return out;
        }

        if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(request, nullptr)) {
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(request, &avail) && avail > 0) {
                std::vector<char> buf(avail);
                DWORD read = 0;
                if (!WinHttpReadData(request, buf.data(), avail, &read) || read == 0) break;
                out.append(buf.data(), read);
            }
        }

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return out;
    }

    inline std::string ExtractImageUrl(const std::string& json)
    {
        // {"data":[{"targetId":..,"state":"Completed","imageUrl":"https://..."}]}
        auto key = json.find("\"imageUrl\"");
        if (key == std::string::npos) return {};
        auto colon = json.find(':', key);
        auto q1 = json.find('"', colon + 1);
        auto q2 = json.find('"', q1 + 1);
        if (q1 == std::string::npos || q2 == std::string::npos) return {};
        return json.substr(q1 + 1, q2 - q1 - 1);
    }

    inline bool DownloadUrl(const std::string& url, std::vector<uint8_t>& bytes)
    {
        // parse https://host/path
        if (url.rfind("https://", 0) != 0) return false;
        auto rest = url.substr(8);
        auto slash = rest.find('/');
        if (slash == std::string::npos) return false;
        std::string hostA = rest.substr(0, slash);
        std::string pathA = rest.substr(slash);
        std::wstring host(hostA.begin(), hostA.end());
        std::wstring path(pathA.begin(), pathA.end());

        std::string body = HttpGet(host, path);
        if (body.empty()) return false;
        bytes.assign(body.begin(), body.end());
        return true;
    }

    inline ID3D11ShaderResourceView* CreateSrvFromPng(const std::vector<uint8_t>& png, int& outW, int& outH)
    {
        outW = outH = 0;
        if (!gDevice || png.empty()) return nullptr;

        IWICImagingFactory* factory = nullptr;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory))) || !factory)
            return nullptr;

        IWICStream* stream = nullptr;
        if (FAILED(factory->CreateStream(&stream)) || !stream) {
            factory->Release();
            return nullptr;
        }
        if (FAILED(stream->InitializeFromMemory((BYTE*)png.data(), (DWORD)png.size()))) {
            stream->Release();
            factory->Release();
            return nullptr;
        }

        IWICBitmapDecoder* decoder = nullptr;
        if (FAILED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder)) || !decoder) {
            stream->Release();
            factory->Release();
            return nullptr;
        }

        IWICBitmapFrameDecode* frame = nullptr;
        if (FAILED(decoder->GetFrame(0, &frame)) || !frame) {
            decoder->Release();
            stream->Release();
            factory->Release();
            return nullptr;
        }

        IWICFormatConverter* converter = nullptr;
        if (FAILED(factory->CreateFormatConverter(&converter)) || !converter) {
            frame->Release();
            decoder->Release();
            stream->Release();
            factory->Release();
            return nullptr;
        }
        if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone,
            nullptr, 0.0, WICBitmapPaletteTypeCustom))) {
            converter->Release();
            frame->Release();
            decoder->Release();
            stream->Release();
            factory->Release();
            return nullptr;
        }

        UINT w = 0, h = 0;
        if (FAILED(converter->GetSize(&w, &h)) || w == 0 || h == 0) {
            converter->Release();
            frame->Release();
            decoder->Release();
            stream->Release();
            factory->Release();
            return nullptr;
        }
        outW = (int)w;
        outH = (int)h;
        std::vector<uint8_t> pixels((size_t)w * h * 4);
        if (FAILED(converter->CopyPixels(nullptr, w * 4, (UINT)pixels.size(), pixels.data()))) {
            converter->Release();
            frame->Release();
            decoder->Release();
            stream->Release();
            factory->Release();
            return nullptr;
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sub{};
        sub.pSysMem = pixels.data();
        sub.SysMemPitch = w * 4;

        ID3D11Texture2D* tex = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
        if (SUCCEEDED(gDevice->CreateTexture2D(&desc, &sub, &tex)) && tex) {
            gDevice->CreateShaderResourceView(tex, nullptr, &srv);
            tex->Release();
        }

        converter->Release();
        frame->Release();
        decoder->Release();
        stream->Release();
        factory->Release();
        return srv;
    }

    inline void Request(int64_t userId)
    {
        if (userId <= 0 || !gDevice) return;

        Entry* e = nullptr;
        {
            std::lock_guard<std::mutex> lock(gMutex);
            auto it = gMap.find(userId);
            if (it != gMap.end()) {
                if (it->second->state.load() != 0) return;
                e = it->second;
            }
            else {
                e = new Entry();
                gMap[userId] = e;
            }
            e->state = 1;
        }

        std::thread([userId, e]() {
            std::wstring path = L"/v1/users/avatar-headshot?userIds=" + std::to_wstring(userId) +
                L"&size=48x48&format=Png&isCircular=false";
            std::string json = HttpGet(L"thumbnails.roblox.com", path);
            std::string imageUrl = ExtractImageUrl(json);
            std::vector<uint8_t> png;
            if (imageUrl.empty() || !DownloadUrl(imageUrl, png) || png.empty()) {
                e->state = 3;
                return;
            }
            e->pendingPng = std::move(png);
            e->pngReady = true;
        }).detach();
    }

    inline void ProcessPending()
    {
        if (!gDevice) return;
        std::lock_guard<std::mutex> lock(gMutex);
        for (auto& pair : gMap) {
            Entry* e = pair.second;
            if (!e || e->state.load() != 1 || !e->pngReady.load())
                continue;
            e->pngReady = false;
            std::vector<uint8_t> png = std::move(e->pendingPng);
            int w = 0, h = 0;
            ID3D11ShaderResourceView* srv = CreateSrvFromPng(png, w, h);
            if (!srv) {
                e->state = 3;
                continue;
            }
            e->srv = srv;
            e->w = w;
            e->h = h;
            e->state = 2;
        }
    }

    inline ID3D11ShaderResourceView* Get(int64_t userId)
    {
        if (userId <= 0) return nullptr;
        std::lock_guard<std::mutex> lock(gMutex);
        auto it = gMap.find(userId);
        if (it == gMap.end()) {
            return nullptr;
        }
        if (it->second->state.load() == 0) {
            // kick off
        }
        if (it->second->state.load() == 2) return it->second->srv;
        return nullptr;
    }

    inline void Ensure(int64_t userId)
    {
        if (userId <= 0) return;
        {
            std::lock_guard<std::mutex> lock(gMutex);
            auto it = gMap.find(userId);
            if (it != gMap.end() && it->second->state.load() != 0) return;
        }
        Request(userId);
    }

    // Catalog asset thumbnails (animations, etc.)
    inline std::unordered_map<int64_t, Entry*> gAssetMap;

    inline void RequestAsset(int64_t assetId)
    {
        if (assetId <= 0 || !gDevice) return;
        Entry* e = nullptr;
        {
            std::lock_guard<std::mutex> lock(gMutex);
            auto it = gAssetMap.find(assetId);
            if (it != gAssetMap.end()) {
                if (it->second->state.load() != 0) return;
                e = it->second;
            } else {
                e = new Entry();
                gAssetMap[assetId] = e;
            }
            e->state = 1;
        }
        std::thread([assetId, e]() {
            std::wstring path = L"/v1/assets?assetIds=" + std::to_wstring(assetId) +
                L"&returnPolicy=PlaceHolder&size=150x150&format=Png&isCircular=false";
            std::string json = HttpGet(L"thumbnails.roblox.com", path);
            std::string imageUrl = ExtractImageUrl(json);
            std::vector<uint8_t> png;
            if (imageUrl.empty() || !DownloadUrl(imageUrl, png) || png.empty()) {
                e->state = 3;
                return;
            }
            e->pendingPng = std::move(png);
            e->pngReady = true;
        }).detach();
    }

    inline void ProcessPendingAssets()
    {
        if (!gDevice) return;
        std::lock_guard<std::mutex> lock(gMutex);
        for (auto& pair : gAssetMap) {
            Entry* e = pair.second;
            if (!e || e->state.load() != 1 || !e->pngReady.load())
                continue;
            e->pngReady = false;
            std::vector<uint8_t> png = std::move(e->pendingPng);
            int w = 0, h = 0;
            ID3D11ShaderResourceView* srv = CreateSrvFromPng(png, w, h);
            if (!srv) {
                e->state = 3;
                continue;
            }
            e->srv = srv;
            e->w = w;
            e->h = h;
            e->state = 2;
        }
    }

    inline ID3D11ShaderResourceView* GetAsset(int64_t assetId)
    {
        if (assetId <= 0) return nullptr;
        std::lock_guard<std::mutex> lock(gMutex);
        auto it = gAssetMap.find(assetId);
        if (it == gAssetMap.end()) return nullptr;
        if (it->second->state.load() == 2) return it->second->srv;
        return nullptr;
    }

    inline void EnsureAsset(int64_t assetId)
    {
        if (assetId <= 0) return;
        {
            std::lock_guard<std::mutex> lock(gMutex);
            auto it = gAssetMap.find(assetId);
            if (it != gAssetMap.end() && it->second->state.load() != 0) return;
        }
        RequestAsset(assetId);
    }
}
