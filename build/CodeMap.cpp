/*
 * CodeMap — 独立桌面宿主（Win32 + WebView2）
 *
 * v2 增补：
 *  - 导航完成事件 → 写诊断日志到 %LOCALAPPDATA%\CodeMap\diagnostic.log
 *  - 导航完成后自动执行自检脚本，把页面内部状态回写到日志
 *  - 页面标题变化时同步窗口标题
 *  - 背景色改为浅色，便于分辨"未渲染"与"渲染了但内容为空"
 *  - 设 CODEMAP_DEBUG_PORT 环境变量时才开启远程调试端口
 *
 * 源码刻意只用 ASCII（中文写成 \uXXXX 转义），避免编码问题。
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <dwmapi.h>              /* 自定义标题栏：无边框窗口的阴影/圆角 */
#include <shobjidl_core.h>   /* IFileOpenDialog：让用户指定项目根目录，用于拼接绝对路径 */
#include <string>
#include <vector>
#include <cstring>
#include <wrl.h>
#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "dwmapi.lib")   /* DwmExtendFrameIntoClientArea：无边框窗口保留系统阴影/圆角 */

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Callback;

#define IDI_APP       1
#define IDR_HTML      1001

#define APP_TITLE     L"CodeMap"
/* 虚拟主机名。**绝不要用 .local** —— 那是 mDNS 保留域，Chromium 会优先按真实域名解析，
   导航会永久挂起（NavigationCompleted 永不触发）。
   用 .example（IANA 保留给文档示例）安全，且不会被真实解析。
   用 https 而非 file:// 的原因：file:// 是 opaque origin，没有存储配额，
   File System Access API 无法持久化目录授权 → 遍历目录时报
   "could not be modified due to the state of the underlying filesystem"。 */
#define ENTRY_FILE    L"CodeMap.html"
#define WND_CLASS     L"CodeMapWndClass"

static HWND                            g_hwnd = nullptr;
static ComPtr<ICoreWebView2Controller> g_ctrl;
static ComPtr<ICoreWebView2>           g_wv;
static ComPtr<ICoreWebView2Environment> g_env;
static bool                            g_debug = false;
static std::wstring                    g_fileUrl;        /* 降级备用的 file:// URL */

/* 前向声明（CapturePreviewToFile / WndProc 用到） */
static std::wstring DataDir();
static void         Log(const std::wstring& line);
static void         RunSelfCheck();

/* WebView2 官方截图：GPU 合成窗口无法用 BitBlt/PrintWindow 抓取，
   必须让 Chromium 自己渲染位图。仅调试模式下调用。 */
static void CapturePreviewToFile()
{
    if (!g_wv) { Log(L"[capture] 无 webview"); return; }

    const std::wstring path = DataDir() + L"\\preview.png";
    IStream* stream = nullptr;
    HRESULT hr = SHCreateStreamOnFileEx(
        path.c_str(),
        STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_NONE,
        FILE_ATTRIBUTE_NORMAL, TRUE, nullptr, &stream);
    if (FAILED(hr) || !stream) {
        Log(L"[capture] 建流失败 hr=0x" + std::to_wstring(static_cast<unsigned>(hr)));
        return;
    }

    HRESULT hrc = g_wv->CapturePreview(
        COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG, stream,
        Callback<ICoreWebView2CapturePreviewCompletedHandler>(
        [](HRESULT hr2) -> HRESULT {
            Log(L"[capture] 完成 hr=0x" + std::to_wstring(static_cast<unsigned>(hr2)));
            return S_OK;
        }).Get());
    Log(L"[capture] 发起 hr=0x" + std::to_wstring(static_cast<unsigned>(hrc)) +
        L" -> " + path);
    if (stream) stream->Release();
}

/* ------------------------------------------------------------------ */
/* 路径工具                                                            */
/* ------------------------------------------------------------------ */

static std::wstring ExeDir()
{
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (wchar_t* p = wcsrchr(buf, L'\\')) *p = L'\0';
    return buf;
}

static std::wstring LocalAppDataDir()
{
    PWSTR p = nullptr;
    std::wstring r;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        r = p;
        CoTaskMemFree(p);
    }
    return r;
}

static void EnsureDir(const std::wstring& d)
{
    std::wstring cur;
    cur.reserve(d.size());
    for (size_t i = 0; i < d.size(); ++i) {
        cur.push_back(d[i]);
        if (d[i] == L'\\' || i + 1 == d.size())
            CreateDirectoryW(cur.c_str(), nullptr);
    }
}

static std::wstring DataDir() { return LocalAppDataDir() + L"\\CodeMap"; }

/* 诊断日志（UTF-8 追加写） */
static void Log(const std::wstring& line)
{
    const std::wstring dir = DataDir();
    EnsureDir(dir);
    const std::wstring path = dir + L"\\diagnostic.log";

    /* 日志只用于排障：超过 1 MB 直接截断，避免长期运行无限增长占盘 */
    WIN32_FILE_ATTRIBUTE_DATA fad = {};
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad)
        && fad.nFileSizeHigh == 0 && fad.nFileSizeLow > (1u << 20)) {
        HANDLE t = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                               TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (t != INVALID_HANDLE_VALUE) CloseHandle(t);
    }

    HANDLE f = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return;

    const int n = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(),
                                      nullptr, 0, nullptr, nullptr);
    std::string u(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), &u[0], n, nullptr, nullptr);
    u += "\r\n";

    SetFilePointer(f, 0, nullptr, FILE_END);
    DWORD w = 0;
    WriteFile(f, u.data(), (DWORD)u.size(), &w, nullptr);
    CloseHandle(f);
}

/* ------------------------------------------------------------------ */
/* 内嵌 HTML 释放                                                      */
/* ------------------------------------------------------------------ */

static bool ExtractEmbeddedHtml(HINSTANCE hInst, int id, const std::wstring& out)
{
    HRSRC hr = FindResourceW(hInst, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!hr) { Log(L"[res] FindResource 失败"); return false; }

    DWORD   sz = SizeofResource(hInst, hr);
    HGLOBAL hg = LoadResource(hInst, hr);
    if (!hg || !sz) { Log(L"[res] LoadResource 失败"); return false; }

    const void* data = LockResource(hg);
    if (!data) { Log(L"[res] LockResource 失败"); return false; }

    /* 同尺寸 ≠ 同内容。旧逻辑只比文件尺寸，一旦新旧内嵌页恰好等长
       （例如仅调换同长度色值、改一位版本号），副本将永远停留在旧版。
       现改为：尺寸相同再比对内容，完全一致才跳过，否则重写。 */
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExW(out.c_str(), GetFileExInfoStandard, &fad) &&
        fad.nFileSizeLow == sz && fad.nFileSizeHigh == 0) {
        HANDLE ef = CreateFileW(out.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (ef != INVALID_HANDLE_VALUE) {
            std::vector<char> oldBuf(static_cast<size_t>(sz));
            DWORD rd = 0;
            BOOL rok = ReadFile(ef, oldBuf.data(), sz, &rd, nullptr);
            CloseHandle(ef);
            if (rok && rd == sz && memcmp(oldBuf.data(), data, sz) == 0) {
                Log(L"[res] 副本与内嵌内容一致，跳过写出");
                return true;
            }
            Log(L"[res] 副本与内嵌内容不一致（同尺寸），重写");
        }
    }

    HANDLE f = CreateFileW(out.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) { Log(L"[res] 无法创建输出文件"); return false; }

    DWORD written = 0;
    BOOL ok = WriteFile(f, data, sz, &written, nullptr);
    CloseHandle(f);
    Log(L"[res] 写出 " + std::to_wstring(written) + L" / " + std::to_wstring(sz) + L" 字节");
    return ok && written == sz;
}

static std::wstring ResolveHtmlDir(HINSTANCE hInst)
{
    std::wstring dir = ExeDir();
    if (GetFileAttributesW((dir + L"\\" ENTRY_FILE).c_str()) != INVALID_FILE_ATTRIBUTES) {
        Log(L"[dir] 使用 exe 同目录: " + dir);
        return dir;
    }
    dir = DataDir() + L"\\app";
    EnsureDir(dir);
    ExtractEmbeddedHtml(hInst, IDR_HTML, dir + L"\\" ENTRY_FILE);
    Log(L"[dir] 释放内嵌副本到: " + dir);
    return dir;
}

/* 转成 file:/// URL（WebView2 只接受正斜杠形式） */
static std::wstring PathToFileUrl(const std::wstring& p)
{
    std::wstring u = p;
    for (size_t i = 0; i < u.size(); ++i)
        if (u[i] == L'\\') u[i] = L'/';

    /* 路径里可能有对 URL 有特殊含义的字符（用户目录名含 # % ? 空格等），
       不编码会导致导航地址被截断或解析错误，这里逐个百分号编码。 */
    std::wstring out = L"file:///";
    for (size_t i = 0; i < u.size(); ++i) {
        const wchar_t c = u[i];
        if (c == L'%' || c == L'#' || c == L'?' || c == L' ' || c == L'"' || c == L'<' || c == L'>') {
            wchar_t buf[8] = {};
            swprintf_s(buf, L"%%%02X", static_cast<unsigned>(c));
            out += buf;
        } else {
            out += c;
        }
    }
    return out;
}

/* ------------------------------------------------------------------ */
/* 让用户指定项目根目录                                                */
/*                                                                     */
/* 浏览器不会把所选文件的绝对路径交给页面（<input webkitdirectory> 只给  */
/* 文件名和相对路径），所以绝对路径只能由宿主提供。这里弹一次原生目录    */
/* 选择框拿到根目录，回传页面后即可用「根目录 + 相对路径」拼出完整路径。  */
/* 弹模态对话框必须回到消息循环里执行，不能阻塞 WebView2 的事件回调。    */
/* ------------------------------------------------------------------ */

#define WM_APP_PICKROOT (WM_APP + 1)

static void PickFolderAndReply()
{
    std::wstring picked;

    ComPtr<IFileOpenDialog> dlg;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&dlg));
    if (SUCCEEDED(hr) && dlg) {
        DWORD opts = 0;
        if (SUCCEEDED(dlg->GetOptions(&opts)))
            dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        dlg->SetTitle(L"\u9009\u62e9\u9879\u76ee\u6839\u76ee\u5f55");
        if (SUCCEEDED(dlg->Show(g_hwnd))) {
            ComPtr<IShellItem> item;
            if (SUCCEEDED(dlg->GetResult(&item)) && item) {
                PWSTR psz = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &psz)) && psz) {
                    picked = psz;
                    CoTaskMemFree(psz);
                }
            }
        }
    } else {
        Log(L"[pickRoot] CoCreateInstance hr=0x" +
            std::to_wstring(static_cast<unsigned>(hr)));
    }

    Log(L"[pickRoot] " + (picked.empty() ? std::wstring(L"(\u53d6\u6d88)") : picked));
    /* 回填方式：用 ExecuteScript 写全局变量。
       不能用 PostWebMessageAsString —— 那需要页面侧注册 message 监听，
       而 window.chrome.webview.addEventListener 会让 WebView2 跳过整个页面脚本。 */
    if (g_wv) {
        std::wstring js;
        if (picked.empty()) {
            js = L"window.__hostRootAbs=null;";
        } else {
            std::wstring esc;
            for (size_t i = 0; i < picked.size(); ++i) {
                if (picked[i] == L'\\' || picked[i] == L'\'') esc += L'\\';
                esc += picked[i];
            }
            js = L"window.__hostRootAbs='" + esc + L"';";
        }
        g_wv->ExecuteScript(js.c_str(), nullptr);
        Log(L"[pickRoot] 回填 " + js);
    }
}

/* ------------------------------------------------------------------ */
/* 窗口过程                                                            */
/* ------------------------------------------------------------------ */

/* 取窗口所在显示器的 DPI（拿不到时退回 96） */
static UINT WindowDpi(HWND h)
{
    if (HMODULE u = GetModuleHandleW(L"user32.dll")) {
        typedef UINT (WINAPI *GetDpiForWindowFn)(HWND);
        if (auto fn = reinterpret_cast<GetDpiForWindowFn>(
                GetProcAddress(u, "GetDpiForWindow"))) {
            UINT d = fn(h);
            if (d) return d;
        }
    }
    return 96;
}

/* 最小窗口尺寸：逻辑 900x600，按 DPI 换算成物理像素 */
static int MinTrackW(HWND h) { return static_cast<int>(900 * WindowDpi(h) / 96); }
static int MinTrackH(HWND h) { return static_cast<int>(600 * WindowDpi(h) / 96); }

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_SIZE: {
        if (g_ctrl) {
            RECT rc; GetClientRect(h, &rc);
            g_ctrl->put_Bounds(rc);
        }
        /* 最大化/还原状态翻转时通知页面，切换窗口按钮的 □/❐ 图标 */
        {
            static bool s_wasMax = false;
            const bool nowMax = (IsZoomed(h) != FALSE);
            if (nowMax != s_wasMax) {
                s_wasMax = nowMax;
                if (g_wv) {
                    std::wstring js = L"(function(){document.documentElement.classList.toggle('wvmax',"
                                    + std::wstring(nowMax ? L"true" : L"false") + L");})();";
                    g_wv->ExecuteScript(js.c_str(), nullptr);
                }
            }
        }
        /* 兜底：SetWindowPos 之类的程序性缩放不受 WM_GETMINMAXINFO 约束，
           发现小于下限就拉回来（拉回后尺寸已合规，不会反复触发）。 */
        if (w != SIZE_MINIMIZED) {
            RECT wr = {};
            GetWindowRect(h, &wr);
            const int fw = wr.right - wr.left, fh = wr.bottom - wr.top;
            const int mw = MinTrackW(h);
            const int mh = MinTrackH(h);
            if (fw > 0 && fh > 0 && (fw < mw || fh < mh)) {
                SetWindowPos(h, nullptr, 0, 0,
                             fw < mw ? mw : fw, fh < mh ? mh : fh,
                             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        return 0;
    }

    case WM_ERASEBKGND: {
        RECT rc; GetClientRect(h, &rc);
        FillRect(reinterpret_cast<HDC>(w), &rc,
                 static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        return 1;
    }

    case WM_NCCALCSIZE: {
        if (!w) break;
        NCCALCSIZE_PARAMS* p = reinterpret_cast<NCCALCSIZE_PARAMS*>(l);
        const UINT dpi = WindowDpi(h);
        const int bx = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi)
                     + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        const int by = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi)
                     + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        const RECT win = p->rgrc[0];
        p->rgrc[0].left   = win.left   + bx;
        p->rgrc[0].right  = win.right  - bx;
        p->rgrc[0].bottom = win.bottom - by;
        /* 顶部完全并入客户区（消除系统刷白的边框带，菜单栏贴顶）。
           最大化时窗口矩形比屏幕大出一个边框带，top 加回 by 让内容顶到屏幕边。 */
        p->rgrc[0].top    = IsZoomed(h) ? (win.top + by) : win.top;
        return 0;
    }

    case WM_GETMINMAXINFO: {
        /* 限制最小尺寸：用户拖拽边框时生效 */
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(l);
        mmi->ptMinTrackSize.x = MinTrackW(h);
        mmi->ptMinTrackSize.y = MinTrackH(h);
        return 0;
    }

    case WM_DPICHANGED: {
        const RECT* pr = reinterpret_cast<const RECT*>(l);
        SetWindowPos(h, nullptr, pr->left, pr->top,
                     pr->right - pr->left, pr->bottom - pr->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    case WM_TIMER:
        if (w == 1) {
            KillTimer(h, 1);
            if (g_debug) CapturePreviewToFile();
        } else if (w == 3) {
            KillTimer(h, 3);
            RunSelfCheck();
        }
        return 0;

    case WM_APP_PICKROOT:
        PickFolderAndReply();
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

/* 页面自检脚本：把关键环境信息序列化成 JSON 返回 */
static const wchar_t* SELF_CHECK_JS =
    L"(function(){try{"
    L"var b=document.querySelector('#btnOpen')||document.querySelector('button');"
    L"var hd=document.querySelector('header');"
    L"var br=b?b.getBoundingClientRect():null;"
    L"var hr=hd?hd.getBoundingClientRect():null;"
    L"var bs=b?getComputedStyle(b):null;"
    L"return JSON.stringify({"
    L"href:location.href,"
    L"secure:window.isSecureContext,"
    L"sdp:typeof window.showDirectoryPicker,"
    L"sofp:typeof window.showOpenFilePicker,"
    L"dcs:typeof window.DecompressionStream,"
    L"fsh:typeof window.FileSystemHandle,"
    L"title:document.title,"
    L"domLen:document.documentElement.innerHTML.length,"
    L"els:document.querySelectorAll('*').length,"
    L"headerRect:hr?[hr.x,hr.y,hr.width,hr.height]:null,"
    L"btnRect:br?[br.x,br.y,br.width,br.height]:null,"
    L"btnBg:bs?bs.backgroundColor:null,"
    L"btnRadius:bs?bs.borderRadius:null,"
    L"bodyBg:getComputedStyle(document.body).backgroundColor,"
    L"vw:window.innerWidth,vh:window.innerHeight,"
    L"storage:window.__cmStorage||null,"
    L"btns:Array.from(document.querySelectorAll('button')).map(function(x){return x.textContent.trim();}).slice(0,10)"
    L"});}catch(e){return 'ERR:'+e.message;}})()";

/* 延后执行自检：导航刚完成时页面上下文可能仍在切换，立即 ExecuteScript 会拿到 null */
static void RunSelfCheck()
{
    if (!g_wv) return;
    g_wv->ExecuteScript(SELF_CHECK_JS,
        Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [](HRESULT hr, LPCWSTR json) -> HRESULT {
            Log(L"[selfcheck] hr=0x" + std::to_wstring(static_cast<unsigned>(hr)) +
                L"  " + std::wstring(json ? json : L"(null)"));
            return S_OK;
        }).Get());
    g_wv->ExecuteScript(L"(function(){return JSON.stringify(window.__cmErr||[]);})()",
        Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [](HRESULT, LPCWSTR json) -> HRESULT {
            Log(L"[jserrors] " + std::wstring(json ? json : L"(null)"));
            return S_OK;
        }).Get());
    /* 独立探针：不改动 SELF_CHECK_JS，单独查询页面渲染状态 */
    g_wv->ExecuteScript(
        L"(function(){try{"
        L"var a=[];"
        L"var sc=document.querySelectorAll('script');"
        L"a.push('scripts='+sc.length);"
        L"a.push('sc0len='+(sc[0]?sc[0].textContent.length:-1));"
        L"a.push('sc0head='+(sc[0]?sc[0].textContent.slice(0,30):'NONE'));"
        L"a.push('ready='+document.readyState);"
        L"a.push('jsStart='+(window.__jsStart?1:0));"
        L"a.push('total='+document.querySelectorAll('*').length);"
        L"a.push('skinCards='+document.querySelectorAll('.skin-card').length);"
        L"a.push('acc='+document.querySelectorAll('.acc').length);"
        L"var e=document.getElementById('empty');"
        L"a.push('emptyLen='+(e?e.innerHTML.length:-1));"
        L"a.push('h2='+(document.querySelector('#empty h2')?document.querySelector('#empty h2').textContent:'NONE'));"
        L"a.push('ctxMenu='+(document.getElementById('ctxMenu')?1:0));"
        L"var cm=document.getElementById('ctxMenu');"
        L"var cs=cm?getComputedStyle(cm):null;"
        L"a.push('ctxPos='+(cs?cs.position:'-'));"
        L"a.push('ctxRadius='+(cs?cs.borderTopLeftRadius:'-'));"
        L"var aa=document.querySelector('.acc-auto');"
        L"var as=aa?getComputedStyle(aa):null;"
        L"a.push('autoBg='+(as?(as.backgroundImage.indexOf('gradient')>=0?'rainbow':'plain'):'-'));"
        L"var ar=document.querySelector('.acc-row');"
        L"a.push('rowJustify='+(ar?getComputedStyle(ar).justifyContent:'-'));"
        L"a.push('rowW='+(ar?Math.round(ar.getBoundingClientRect().width):-1));"
        L"var pc=document.getElementById('skinPanel');"
        L"a.push('panelW='+(pc?Math.round(pc.getBoundingClientRect().width):-1));"
        L"a.push('labGone='+(document.querySelector('#accentRow .lab')?'NO':'YES'));"
        L"a.push('initDone='+(window.__initDone?1:0));"
        L"a.push('errs='+JSON.stringify((window.__cmErr||[]).slice(0,5)));"
        L"return a.join(' | ');"
        L"}catch(x){return 'PROBEERR:'+x.message;}})()",
        Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [](HRESULT, LPCWSTR s) -> HRESULT {
            Log(L"[probe] " + std::wstring(s ? s : L"(null)"));
            return S_OK;
        }).Get());
}

/* 页面内错误捕获 + 存储能力探测（在文档创建时注入，尽早生效） */
static const wchar_t* ERROR_TRAP_JS =
    L"(function(){"
    L"window.__cmErr=[];"
    L"window.addEventListener('error',function(e){window.__cmErr.push(String(e.message)+' @'+String(e.filename)+':'+String(e.lineno));});"
    L"window.addEventListener('unhandledrejection',function(e){window.__cmErr.push('rej:'+String(e.reason));});"
    /* opaque origin（file://）下没有存储配额，File System Access API 的目录授权
       无法持久化，遍历目录会报 'could not be modified ... underlying filesystem'。
       这里探测存储可用性，用于确认当前 origin 是否具备条件。 */
    L"try{"
    L"if(navigator.storage&&navigator.storage.estimate){"
    L"navigator.storage.estimate().then("
    L"function(e){window.__cmStorage='ok quota='+e.quota+' usage='+e.usage;},"
    L"function(err){window.__cmStorage='fail:'+String(err&&err.name);});"
    L"}else{window.__cmStorage='no navigator.storage';}"
    L"}catch(e){window.__cmStorage='throw:'+e.message;}"
    /* 页面主动上报：比宿主 ExecuteScript 去拉更可靠（不受导航上下文切换的影响） */
    L"setTimeout(function(){try{"
    L"var b=document.querySelector('#btnOpen')||document.querySelector('button');"
    L"var hd=document.querySelector('header');"
    L"var br=b?b.getBoundingClientRect():null;"
    L"var hr=hd?hd.getBoundingClientRect():null;"
    L"var bs=b?getComputedStyle(b):null;"
    L"window.chrome.webview.postMessage('SELFCHECK|'+JSON.stringify({"
    L"href:location.href,secure:window.isSecureContext,"
    L"sdp:typeof window.showDirectoryPicker,"
    L"dcs:typeof window.DecompressionStream,"
    L"domLen:document.documentElement.innerHTML.length,"
    L"headerRect:hr?[hr.width,hr.height]:null,"
    L"btnBg:bs?bs.backgroundColor:null,"
    L"storage:window.__cmStorage||null,"
    L"errs:window.__cmErr||[],"
    L"btns:Array.from(document.querySelectorAll('button')).map(function(x){return x.textContent.trim();}).slice(0,8)"
    L"}));}catch(e){try{window.chrome.webview.postMessage('SELFCHECK-ERR|'+e.message);}catch(_){}}},2000);"
    L"return 'trap-installed';})()";

/* ------------------------------------------------------------------ */
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    if (HMODULE u = GetModuleHandleW(L"user32.dll")) {
        typedef BOOL (WINAPI *SetDpiCtxFn)(DPI_AWARENESS_CONTEXT);
        if (auto fn = reinterpret_cast<SetDpiCtxFn>(
                GetProcAddress(u, "SetProcessDpiAwarenessContext")))
            fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    /* 每次启动清空日志 */
    {
        const std::wstring p = DataDir() + L"\\diagnostic.log";
        EnsureDir(DataDir());
        DeleteFileW(p.c_str());
    }
    Log(L"=== CodeMap 启动 ===");

    const std::wstring htmlDir = ResolveHtmlDir(hInst);
    /* 直接用 file:// 直载。
       曾经尝试给页面一个 https origin（虚拟主机映射 / 请求拦截）以便 File System
       Access API 的目录授权能够持久化，但两条路都不稳定：
         - SetVirtualHostNameToFolderMapping 走 Chromium 网络栈，系统代理存在时会被
           拦下挂起（实测 3 次里 1 次失败）；
         - WebResourceRequested 在 DNS 阶段就被绕过，处理函数根本不触发。
       现改为页面侧使用 <input type="file" webkitdirectory>，它不受 origin 限制，
       因此不需要 https origin。 */
    g_fileUrl = PathToFileUrl(htmlDir + L"\\" ENTRY_FILE);
    const std::wstring url = g_fileUrl;
    Log(L"[url] " + url);

    const int cxIcon = GetSystemMetrics(SM_CXICON);
    const int cyIcon = GetSystemMetrics(SM_CYICON);
    const int cxSm   = GetSystemMetrics(SM_CXSMICON);
    const int cySm   = GetSystemMetrics(SM_CYSMICON);
    Log(L"[icon] big=" + std::to_wstring(cxIcon) + L" small=" + std::to_wstring(cxSm));

    HICON hBig = static_cast<HICON>(LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APP),
                                              IMAGE_ICON, cxIcon, cyIcon, LR_DEFAULTCOLOR));
    if (!hBig)
        hBig = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APP));
    HICON hSm = static_cast<HICON>(LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APP),
                                              IMAGE_ICON, cxSm, cySm, LR_DEFAULTCOLOR));
    if (!hSm) hSm = hBig;
    Log(L"[icon] hBig=" + std::to_wstring(reinterpret_cast<UINT_PTR>(hBig)) +
        L" hSm=" + std::to_wstring(reinterpret_cast<UINT_PTR>(hSm)));

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hIcon         = hBig;
    wc.hIconSm       = hSm;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = WND_CLASS;
    if (!RegisterClassExW(&wc)) { Log(L"[win] RegisterClassEx 失败"); return 1; }

    RECT wa = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    const int availW = wa.right - wa.left;
    const int availH = wa.bottom - wa.top;

    /* 期望的**逻辑**尺寸 1500x940，按 DPI 换算为物理像素。
       本进程已 DPI 感知，SystemParametersInfo 返回的是物理像素；
       若直接用物理值做上限，在 200% 缩放屏上窗口会只剩一半逻辑宽度。 */
    HDC screenDc = GetDC(nullptr);
    const int dpiX = GetDeviceCaps(screenDc, LOGPIXELSX);
    ReleaseDC(nullptr, screenDc);
    const double dpiScale = (dpiX > 0 ? dpiX : 96) / 96.0;
    const int wantW = static_cast<int>(1500 * dpiScale);
    const int wantH = static_cast<int>(940  * dpiScale);
    const int wndW  = min(wantW, static_cast<int>(availW * 0.90));
    const int wndH  = min(wantH, static_cast<int>(availH * 0.92));
    const int wndX  = wa.left + (availW - wndW) / 2;
    const int wndY  = wa.top  + (availH - wndH) / 2;

    Log(L"[win] dpi=" + std::to_wstring(dpiX) +
        L" avail=" + std::to_wstring(availW) + L"x" + std::to_wstring(availH) +
        L" → " + std::to_wstring(wndW) + L"x" + std::to_wstring(wndH) +
        L" (逻辑 " + std::to_wstring(static_cast<int>(wndW / dpiScale)) + L"x" +
        std::to_wstring(static_cast<int>(wndH / dpiScale)) + L")");

    /* 保留 WS_CAPTION（拿到系统的最大化/还原动画、系统菜单与阴影），
       视觉标题栏在 WM_NCCALCSIZE 里隐藏；WS_THICKFRAME 保留四边 resize 边框。 */
    const DWORD wndStyle = WS_OVERLAPPEDWINDOW;
    HWND hwnd = CreateWindowExW(0, WND_CLASS, APP_TITLE, wndStyle,
                                wndX, wndY, wndW, wndH, nullptr, nullptr, hInst, nullptr);
    if (!hwnd) { Log(L"[win] CreateWindowEx 失败"); return 1; }
    /* DWM 边框扩展 1px：无边框窗口才能拿到系统投影阴影与 Win11 圆角 */
    {
        const MARGINS m = { 0, 0, 0, 1 };
        const HRESULT hrDwm = DwmExtendFrameIntoClientArea(hwnd, &m);
        Log(L"[win] DwmExtendFrameIntoClientArea hr=0x" +
            std::to_wstring(static_cast<unsigned>(hrDwm)));
    }
    /* 强制重算非客户区：窗口初次创建时 WM_NCCALCSIZE 的 wParam 为 FALSE（不重算客户区），
       标题栏不会被隐藏。主动发一次 SWP_FRAMECHANGED 触发 wParam=TRUE 的 WM_NCCALCSIZE，
       应用「去标题栏、保留边框」逻辑。 */
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    Log(L"[win] 窗口已创建 " + std::to_wstring(wndW) + L"x" + std::to_wstring(wndH));

    g_hwnd = hwnd;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    const std::wstring udf = DataDir() + L"\\WebView2";
    EnsureDir(udf);

    /* 浏览器参数。
       --no-proxy-server：本应用只加载本地 file:// 内容、不访问网络，
       显式禁代理可避免系统代理（如 Clash 的 127.0.0.1:8052）干扰启动。
       调试端口仅在设置了 CODEMAP_DEBUG_PORT 时附加。 */
    std::wstring extraArgs = L"--no-proxy-server";
    wchar_t dbg[16] = {};
    if (GetEnvironmentVariableW(L"CODEMAP_DEBUG_PORT", dbg, 16) && dbg[0]) {
        g_debug = true;
        extraArgs += L" --remote-debugging-port=";
        extraArgs += dbg;
        extraArgs += L" --remote-allow-origins=*";
    }
    ComPtr<CoreWebView2EnvironmentOptions> opts =
        Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    if (opts) {
        HRESULT hrOpt = opts->put_AdditionalBrowserArguments(extraArgs.c_str());
        Log(L"[browserargs] " + extraArgs + L"  hr=0x" +
            std::to_wstring(static_cast<unsigned>(hrOpt)));
    }

    const HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, udf.c_str(), opts.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
        [hwnd, htmlDir, url](HRESULT hrEnv, ICoreWebView2Environment* env) -> HRESULT
        {
            Log(L"[env] hr=0x" + std::to_wstring(static_cast<unsigned>(hrEnv)));
            g_env = env;
            if (FAILED(hrEnv) || !env) {
                MessageBoxW(hwnd,
                    L"无法初始化 WebView2 运行时。\n\n"
                    L"请先安装 Microsoft Edge WebView2 Runtime 后重试：\n"
                    L"https://developer.microsoft.com/microsoft-edge/webview2/",
                    APP_TITLE, MB_ICONERROR | MB_OK);
                return hrEnv;
            }

            return env->CreateCoreWebView2Controller(hwnd,
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [hwnd, htmlDir, url](HRESULT hrCtrl, ICoreWebView2Controller* ctrl) -> HRESULT
                {
                    Log(L"[ctrl] hr=0x" + std::to_wstring(static_cast<unsigned>(hrCtrl)));
                    if (FAILED(hrCtrl) || !ctrl) return hrCtrl;

                    g_ctrl = ctrl;
                    g_ctrl->get_CoreWebView2(&g_wv);
                    if (!g_wv) { Log(L"[ctrl] get_CoreWebView2 失败"); return E_FAIL; }

                    RECT rc = {};
                    GetClientRect(hwnd, &rc);
                    g_ctrl->put_Bounds(rc);
                    Log(L"[ctrl] bounds=" + std::to_wstring(rc.right) + L"x" +
                        std::to_wstring(rc.bottom));

                    ComPtr<ICoreWebView2Controller2> c2;
                    if (SUCCEEDED(g_ctrl.As(&c2)) && c2) {
                        /* 浅色底：便于分辨"没渲染"和"渲染了但空白" */
                        COREWEBVIEW2_COLOR bg = { 255, 0xf7, 0xf8, 0xfa };
                        c2->put_DefaultBackgroundColor(bg);
                    }

                    /* 事件：导航完成 */
                    g_wv->add_NavigationCompleted(
                        Callback<ICoreWebView2NavigationCompletedEventHandler>(
                        [hwnd](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* a) -> HRESULT
                        {
                            BOOL ok = FALSE;
                            a->get_IsSuccess(&ok);
                            COREWEBVIEW2_WEB_ERROR_STATUS st =
                                COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
                            a->get_WebErrorStatus(&st);
                            Log(L"[nav] success=" + std::to_wstring(ok) +
                                L" webErrorStatus=" + std::to_wstring(static_cast<int>(st)));

                            /* 加载失败（内嵌副本损坏、磁盘不可读等）时给出明确提示，
                               避免只留下一个白窗口让用户以为程序坏了 */
                            if (!ok) {
                                MessageBoxW(hwnd,
                                    L"界面加载失败（错误码见诊断日志）。\n\n"
                                    L"请重新运行程序；若问题持续，删除目录\n"
                                    L"%LOCALAPPDATA%\\CodeMap\\app 后重试。",
                                    APP_TITLE, MB_ICONERROR | MB_OK);
                            }

                            /* 自检：立即执行一次，再用定时器补一次。
                               上下文切换时立即执行可能拿到 null，所以两条都留着。 */
                            RunSelfCheck();
                            SetTimer(hwnd, 3, 2500, nullptr);
                            /* 调试模式下再晚一点截图（等示例数据渲染完） */
                            if (g_debug) SetTimer(hwnd, 1, 4500, nullptr);
                            return S_OK;
                        }).Get(), nullptr);

                    /* 事件：页面主动上报（自检结果走这条通道） */
                    g_wv->add_WebMessageReceived(
                        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* a) -> HRESULT
                        {
                            LPWSTR msg = nullptr;
                            if (SUCCEEDED(a->TryGetWebMessageAsString(&msg)) && msg) {
                                std::wstring s(msg);
                                CoTaskMemFree(msg);
                                /* 页面命令以 CMD: 开头；其余（自检上报）只记日志 */
                                if (s.rfind(L"CMD:pickRoot", 0) == 0)
                                    PostMessageW(g_hwnd, WM_APP_PICKROOT, 0, 0);
                                else if (s.rfind(L"CMD:min", 0) == 0)
                                    ShowWindow(g_hwnd, SW_MINIMIZE);
                                else if (s.rfind(L"CMD:max", 0) == 0) {
                                    if (IsZoomed(g_hwnd)) ShowWindow(g_hwnd, SW_RESTORE);
                                    else                  ShowWindow(g_hwnd, SW_MAXIMIZE);
                                }
                                else if (s.rfind(L"CMD:close", 0) == 0)
                                    PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
                                else
                                    Log(L"[page] " + s);
                            }
                            return S_OK;
                        }).Get(), nullptr);

                    /* 自动允许权限请求：本应用纯本地、不加载任何远程内容，
                        showDirectoryPicker 的目录读取权限直接放行，
                        避免 Chromium 的「是否允许查看文件」权限气泡打断操作流程。 */
                    g_wv->add_PermissionRequested(
                        Callback<ICoreWebView2PermissionRequestedEventHandler>(
                        [](ICoreWebView2*, ICoreWebView2PermissionRequestedEventArgs* a) -> HRESULT
                        {
                            a->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                            return S_OK;
                        }).Get(), nullptr);

                    /* 事件：标题变化 */
                    g_wv->add_DocumentTitleChanged(
                        Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
                        [hwnd](ICoreWebView2* sender, IUnknown*) -> HRESULT
                        {
                            LPWSTR t = nullptr;
                            if (SUCCEEDED(sender->get_DocumentTitle(&t)) && t) {
                                Log(L"[title] " + std::wstring(t));
                                SetWindowTextW(hwnd, t);
                                CoTaskMemFree(t);
                            }
                            return S_OK;
                        }).Get(), nullptr);

                    ComPtr<ICoreWebView2Settings> set;
                    if (SUCCEEDED(g_wv->get_Settings(&set)) && set) {
                        set->put_IsStatusBarEnabled(FALSE);
                        set->put_AreDefaultScriptDialogsEnabled(TRUE);
                        set->put_IsZoomControlEnabled(TRUE);
                        /* 桌面应用不应暴露浏览器能力：F12 开发者工具、WebView2 自带右键菜单、
                           F5/Ctrl+P 等浏览器快捷键都会导致误操作或出现与本应用无关的界面。
                           关掉后页面自己的 Ctrl+F（搜索）与自定义右键菜单不受影响。 */
                        set->put_AreDevToolsEnabled(FALSE);
                        set->put_AreDefaultContextMenusEnabled(FALSE);
                        /* 浏览器快捷键（F5/Ctrl+P 等）在 Settings3 才提供 */
                        ComPtr<ICoreWebView2Settings3> set3;
                        if (SUCCEEDED(set->QueryInterface(IID_PPV_ARGS(&set3))) && set3)
                            set3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
                        /* 启用非客户区支持：让页面 app-region: drag 区域可拖动窗口。
                           默认 FALSE，此时 app-region CSS 会被忽略（标题栏拖不动）。 */
                        ComPtr<ICoreWebView2Settings9> set9;
                        if (SUCCEEDED(set->QueryInterface(IID_PPV_ARGS(&set9)))) {
                            set9->put_IsNonClientRegionSupportEnabled(TRUE);
                            Log(L"[set] IsNonClientRegionSupportEnabled=true");
                        } else {
                            Log(L"[set] 本机 WebView2 运行时过旧，不支持非客户区拖动");
                        }
                    }

                    /* 先注入错误捕获，再导航 */
                    g_wv->AddScriptToExecuteOnDocumentCreated(ERROR_TRAP_JS, nullptr);

                    Log(L"[nav] Navigate → " + url);
                    /* 直接 file:// 加载，不需要超时降级与重试 */
                    g_wv->Navigate(url.c_str());
                    return S_OK;
                }).Get());
        }).Get());

    if (FAILED(hr))
        Log(L"[env] CreateCoreWebView2EnvironmentWithOptions hr=0x" +
            std::to_wstring(static_cast<unsigned>(hr)));

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Log(L"=== 退出 ===");
    g_wv.Reset();
    g_ctrl.Reset();
    CoUninitialize();
    return 0;
}
