# -*- coding: utf-8 -*-
"""构建 CodeMap.exe：准备资源 → 生成 manifest/rc → 编译链接"""
import os, glob, shutil, subprocess, sys

# 路径全部相对仓库根，便于他人克隆后直接构建
ROOT = os.path.dirname(os.path.abspath(__file__))
WORK = os.path.join(ROOT, 'build')
WV2  = os.path.join(WORK, 'webview2')
ICO_SRC  = os.path.join(ROOT, 'CodeMap-source', 'CodeMap.ico')
HTML_SRC = os.path.join(ROOT, 'CodeMap-source', 'CodeMap.html')

def _vs_roots():
    """定位 Visual Studio：VSINSTALLDIR → vswhere → 常见安装路径。"""
    cands = []
    envd = os.environ.get('VSINSTALLDIR')
    if envd:
        cands.append(envd.rstrip('\\/'))
    vswhere = os.path.join(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'),
                           'Microsoft Visual Studio', 'Installer', 'vswhere.exe')
    if os.path.isfile(vswhere):
        try:
            out = subprocess.run(
                [vswhere, '-latest', '-products', '*',
                 '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                 '-property', 'installationPath'],
                capture_output=True, text=True, timeout=30).stdout.strip()
            if out:
                cands.append(out)
        except Exception:
            pass
    for base in (os.environ.get('ProgramFiles', r'C:\Program Files'),
                 os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')):
        cands += glob.glob(os.path.join(base, 'Microsoft Visual Studio', '*', '*'))
        cands += glob.glob(os.path.join(base, 'Microsoft Visual Studio', '*'))
    return [c for c in cands if os.path.isdir(os.path.join(c, 'VC', 'Tools', 'MSVC'))]

def _pick_msvc(roots):
    best = None
    for r in roots:
        for d in glob.glob(os.path.join(r, 'VC', 'Tools', 'MSVC', '*')):
            if os.path.isdir(d) and (best is None or os.path.basename(d) > os.path.basename(best)):
                best = d
    return best

MSVCVER = _pick_msvc(_vs_roots())
if not MSVCVER:
    sys.exit('未找到 Visual Studio（需“使用 C++ 的桌面开发”工作负载）。可用环境变量 VSINSTALLDIR 指定。')

SDKROOT = os.environ.get('WindowsSdkDir') or os.path.join(
    os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'), 'Windows Kits', '10')
_sdks = sorted(glob.glob(os.path.join(SDKROOT, 'Include', '10.*')))
if not _sdks:
    sys.exit('未找到 Windows 10/11 SDK。')
SDKVER = os.path.basename(_sdks[-1])

if not os.path.isdir(os.path.join(WV2, 'build', 'native')):
    sys.exit('缺少 WebView2 SDK：请把 NuGet 包 Microsoft.Web.WebView2 解压到 build/webview2/')

CL   = os.path.join(MSVCVER, r'bin\Hostx64\x64\cl.exe')
RC   = os.path.join(SDKROOT, r'bin\%s\x64\rc.exe' % SDKVER)
DUMPBIN = os.path.join(MSVCVER, r'bin\Hostx64\x64\dumpbin.exe')

env = dict(os.environ)
env['INCLUDE'] = ';'.join([
    os.path.join(MSVCVER, 'include'),
    os.path.join(SDKROOT, r'Include\%s\ucrt'   % SDKVER),
    os.path.join(SDKROOT, r'Include\%s\shared' % SDKVER),
    os.path.join(SDKROOT, r'Include\%s\um'     % SDKVER),
    os.path.join(SDKROOT, r'Include\%s\winrt'  % SDKVER),   # wrl.h 在 winrt 目录下
    os.path.join(SDKROOT, r'Include\%s\cppwinrt' % SDKVER),
])
env['LIB'] = ';'.join([
    os.path.join(MSVCVER, r'lib\x64'),
    os.path.join(SDKROOT, r'Lib\%s\ucrt\x64' % SDKVER),
    os.path.join(SDKROOT, r'Lib\%s\um\x64'   % SDKVER),
])
env['PATH'] = ';'.join([os.path.dirname(CL), os.path.dirname(RC), env.get('PATH', '')])

print('=' * 60)
print('一、准备资源文件')
print('=' * 60)
shutil.copy2(ICO_SRC, os.path.join(WORK, 'CodeMap.ico'))
shutil.copy2(HTML_SRC, os.path.join(WORK, 'CodeMap.html'))
for f in ('CodeMap.ico', 'CodeMap.html'):
    p = os.path.join(WORK, f)
    print('  %-16s %8d 字节' % (f, os.path.getsize(p)))

MANIFEST = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity type="win32" name="CodeMap" version="1.4.11.0" processorArchitecture="amd64"/>
  <application xmlns="urn:schemas-microsoft-com:asm.v3">
    <windowsSettings>
      <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/pm</dpiAware>
      <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2,PerMonitor</dpiAwareness>
      <longPathAware xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">true</longPathAware>
      <activeCodePage xmlns="http://schemas.microsoft.com/SMI/2019/WindowsSettings">UTF-8</activeCodePage>
    </windowsSettings>
  </application>
  <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
    <application>
      <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/>
    </application>
  </compatibility>
</assembly>
'''
open(os.path.join(WORK, 'CodeMap.manifest'), 'w', encoding='utf-8').write(MANIFEST)

# 资源脚本：文件名一律用相对路径，规避 rc 把 '\A' 之类当转义序列
RC_TEXT = '''#include <windows.h>

1    ICON      "CodeMap.ico"
1    24        "CodeMap.manifest"
1001 RCDATA    "CodeMap.html"

VS_VERSION_INFO VERSIONINFO
 FILEVERSION    1,4,11,0
 PRODUCTVERSION 1,4,11,0
 FILEFLAGSMASK  0x3fL
 FILEFLAGS      0x0L
 FILEOS         0x40004L
 FILETYPE       0x1L
 FILESUBTYPE    0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "080404b0"
        BEGIN
            VALUE "CompanyName",      "Meteor\\0"
            VALUE "FileDescription",  "CodeMap - \\u4ee3\\u7801\\u5730\\u56fe\\0"
            VALUE "FileVersion",      "1.4.11.0\\0"
            VALUE "InternalName",     "CodeMap\\0"
            VALUE "OriginalFilename", "CodeMap.exe\\0"
            VALUE "ProductName",      "CodeMap\\0"
            VALUE "ProductVersion",   "1.4.11.0\\0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x804, 1200
    END
END
'''
# 版本信息里的中文改用真实汉字（配合 /c 65001），比转义更直观
RC_TEXT = RC_TEXT.replace('"CodeMap - \\u4ee3\\u7801\\u5730\\u56fe\\0"', '"CodeMap - \u4ee3\u7801\u5730\u56fe\\0"')
open(os.path.join(WORK, 'CodeMap.rc'), 'w', encoding='utf-8').write(RC_TEXT)
print('  已生成 CodeMap.manifest / CodeMap.rc')

print()
print('=' * 60)
print('二、WebView2 库文件')
print('=' * 60)
libs = glob.glob(os.path.join(WV2, r'build\native\x64\*.lib'))
for l in libs:
    print('  %-40s %8d 字节' % (os.path.basename(l), os.path.getsize(l)))
static_lib = os.path.join(WV2, r'build\native\x64\WebView2LoaderStatic.lib')
use_static = os.path.exists(static_lib)
print('  使用静态库(单文件 exe):', '✓ 是' if use_static else '✗ 否（需随附 WebView2Loader.dll）')


def run(argv, label):
    r = subprocess.run(argv, cwd=WORK, capture_output=True, text=True,
                       encoding='utf-8', errors='replace', env=env, timeout=600)
    print('\n--- %s ---' % label)
    print('  exit:', r.returncode)
    for tag, s in (('stdout', r.stdout), ('stderr', r.stderr)):
        s = (s or '').strip()
        if s:
            print('  %s:\n%s' % (tag, '\n'.join('    ' + x for x in s.splitlines()[:40])))
    return r.returncode


print()
print('=' * 60)
print('三、编译')
print('=' * 60)
if run([RC, '/nologo', '/c', '65001', '/fo', 'CodeMap.res', 'CodeMap.rc'],
       'rc.exe 编译资源') != 0:
    print('\n✗ 资源编译失败')
    sys.exit(1)

inc = os.path.join(WV2, r'build\native\include')
libdir = os.path.join(WV2, r'build\native\x64')
link_lib = 'WebView2LoaderStatic.lib' if use_static else 'WebView2Loader.dll.lib'

code = run([CL, '/nologo', '/std:c++20', '/utf-8', '/EHsc', '/O2', '/MT',
            '/I' + inc,
            'CodeMap.cpp', 'CodeMap.res',
            '/link', '/SUBSYSTEM:WINDOWS', '/OUT:CodeMap.exe',
            '/LIBPATH:' + libdir, link_lib,
            '/MANIFEST:NO'],
           'cl.exe 编译并链接')

exe = os.path.join(WORK, 'CodeMap.exe')
if code != 0:
    print('\n✗ 编译失败（exit %d），未生成新 exe' % code)
    sys.exit(1)
if not os.path.exists(exe):
    print('\n✗ 未生成 exe')
    sys.exit(1)

print()
print('=' * 60)
print('四、产物')
print('=' * 60)
print('  CodeMap.exe  %d 字节 (%.2f MB)' % (os.path.getsize(exe), os.path.getsize(exe) / 1024 / 1024))

r = subprocess.run([DUMPBIN, '/nologo', '/dependents', 'CodeMap.exe'], cwd=WORK,
                   capture_output=True, text=True, encoding='utf-8', errors='replace', env=env)
if r.returncode == 0:
    print('\n  依赖的 DLL:')
    inlist = False
    for line in r.stdout.splitlines():
        t = line.strip()
        if 'Image has the following dependencies' in t:
            inlist = True
            continue
        if inlist:
            if not t or t.startswith('Summary'):
                break
            print('    ', t)
elif r.stderr:
    print('  dumpbin 失败:', r.stderr.strip()[:300])

# 若用了动态 loader，把 DLL 一起放好
if not use_static:
    for cand in glob.glob(os.path.join(WV2, r'build\native\x64\WebView2Loader.dll')):
        shutil.copy2(cand, os.path.join(WORK, 'WebView2Loader.dll'))
        print('\n  已复制 WebView2Loader.dll（需与 exe 同目录）')
