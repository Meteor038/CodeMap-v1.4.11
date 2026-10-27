# -*- coding: utf-8 -*-
"""英文词典：简体原文 -> English"""

EN = {
# ---- 菜单栏 / 通用动作 ----
'文件': 'File', '打开文件夹': 'Open folder', '重新载入': 'Reload', '导出为图片': 'Export as image',
'视图': 'View', '地图': 'Map', '列表': 'List', '左侧面板': 'Left panel', '详情面板': 'Details panel',
'度量': 'Metric', '代码行': 'Lines of code', '体积': 'Size', '文件数': 'Files',
'外观': 'Appearance', '帮助': 'Help', '快捷键': 'Shortcuts', '关于 CodeMap': 'About CodeMap',
'搜索文件…': 'Search files…', '最小化': 'Minimize', '最大化 / 还原': 'Maximize / restore', '关闭': 'Close',
'皮肤': 'Skin', '语言配色': 'Language palette', '强调色': 'Accent colour', '色板': 'Palette',
'拖动调整宽度': 'Drag to resize', '收起左侧面板': 'Collapse left panel', '展开左侧面板': 'Expand left panel',
'收起面板': 'Collapse panel', '展开详情面板': 'Expand details panel',
'自动换行': 'Wrap lines', '知道了': 'Got it', '正在扫描…': 'Scanning…', '准备中': 'Preparing',
'准备中…': 'Preparing…', '忽略': 'Ignore', '开': 'On', '关': 'Off', '行': 'lines',
'点击切换忽略规则': 'Toggle ignore rules', '点击切换度量': 'Switch metric', '点击切换视图': 'Switch view',
'跟随皮肤': 'Follow skin', '总行数': 'Total lines', '自动': 'Auto',

# ---- 面板 / 状态栏 ----
'概览': 'Overview', '目录树': 'Tree', '详情': 'Details',
'未载入项目': 'No project', '尚未载入项目': 'No project loaded', '项目': 'Project',
'当前项目（点击打开文件夹）': 'Current project (click to open a folder)',
'已忽略依赖与构建产物目录': 'Dependency and build folders ignored',
'已包含全部目录（大项目会明显变慢）': 'All folders included (large projects will be slower)',
'已忽略依赖与产物目录，重新扫描中…': 'Ignoring dependency and build folders, rescanning…',
'已包含全部目录，重新扫描中…': 'Including all folders, rescanning…',

# ---- 检视器 / 统计 ----
'构成': 'Composition', '子目录': 'Subfolders', '文件': 'Files', '注释率': 'Comment ratio',
'最大的子项': 'Largest children', '规模': 'Scale', '占根目录': 'Of root', '注释行': 'Comment lines',
'未统计': 'Not counted', '名称': 'Name', '类型': 'Type', '修改时间': 'Modified', '目录': 'Folder',
'其他': 'Other', '根目录': 'Root', '代码': 'Code', '注释': 'Comment', '空行': 'Blank',
'选中一个矩形或文件查看详情': 'Select a rectangle or file for details',
'没有匹配的内容': 'No matches', '这个目录是空的': 'This folder is empty',
'文件超过 ': 'File over ', '，未逐行统计': ', not counted line by line',
'二进制': 'Binary', ' 文件，不参与行数统计': ' file, excluded from line counts',

# ---- 空态 ----
'把文件夹变成一张可下钻的地图': 'Turn a folder into a drillable map',
'打开任意代码目录，按代码量、体积或文件数渲染成可下钻的矩形地图：面积越大体量越大，颜色代表语言。单击看详情，双击进入目录。':
    'Open any code folder and it becomes a drillable rectangle map sized by lines, size or file count: bigger area means bigger weight, colour means language. Click for details, double-click to enter a folder.',
'被分析的代码全程留在本机，不上传任何内容': 'Everything stays on this machine — nothing is uploaded',
'把代码文件夹直接拖到这里即可渲染成矩形地图：面积越大体量越大，颜色代表语言。单击看详情，双击进入目录。':
    'Drop a code folder here to render it as a rectangle map: bigger area means bigger weight, colour means language. Click for details, double-click to enter a folder.',
'打开文件夹': 'Open folder',

# ---- 拖拽 / 扫描 ----
'这个浏览器不支持拖拽目录，请改用「打开文件夹」': 'This browser cannot drop folders — use “Open folder” instead',
'已读取 ': 'Read ', ' 个文件': ' files', '扫描失败：': 'Scan failed: ', '[CodeMap] 扫描失败': '[CodeMap] scan failed',
'没有读到内容': 'Nothing was read', '这个目录里没有可统计的文本文件（试试关闭忽略规则）':
    'No countable text files in this folder (try turning ignore rules off)',
'请先用「打开文件夹」载入一个目录': 'Load a folder with “Open folder” first',
' 个文件 · ': ' files · ', ' · 用时 ': ' · took ', ' 秒': 's',

# ---- 外观面板 ----
'经典': 'Classic', '柔和': 'Soft', '霓虹': 'Neon', '单色': 'Mono', '暖调': 'Warm', '极光': 'Aurora',
'无印': 'Nothing', '极简白': 'Plain white', '午夜紫': 'Midnight purple', '赛博': 'Cyber',
'终端': 'Terminal', '纸感': 'Paper', '落日': 'Dusk',

# ---- 右键菜单 / 复制 ----
'进入此目录': 'Enter this folder', '预览此文件': 'Preview this file',
'复制文件名': 'Copy file name', '复制相对路径': 'Copy relative path', '复制绝对路径': 'Copy absolute path',
'已复制文件名：': 'Copied file name: ', '已复制相对路径：': 'Copied relative path: ',
'已复制绝对路径：': 'Copied absolute path: ', '复制失败': 'Copy failed',
'未取得项目根目录，无法拼接绝对路径': 'Project root unknown — absolute path unavailable',
'双击进入': 'Double-click to enter', '双击打开': 'Double-click to open',

# ---- 导出 ----
'请先切换到「地图」视图再导出': 'Switch to the Map view before exporting',
'导出失败': 'Export failed', '已导出 PNG：': 'Exported PNG: ', '导出失败：图片渲染错误': 'Export failed: image render error',

# ---- 预览器 ----
'预览': 'Preview', '代码预览': 'Code', '图片': 'Image', '视频': 'Video', '音频': 'Audio',
'文档正文': 'Document text', '表格': 'Table', '压缩包': 'Archive', '十六进制': 'Hex',
'全屏展开': 'Expand', '读取中…': 'Loading…', '读取失败': 'Read failed',
'无可读内容': 'Nothing to show', '这个文件没有可供预览的源文本': 'This file has no previewable source text',
'点「打开文件夹」载入本地目录后，即可查看文件正文。': 'Open a folder to view file contents.',
'(空文件)': '(empty file)', '仅显示前 400 行': 'First 400 lines only',
'仅显示前 ': 'First ', ' 行，共 ': ' lines shown of ', '行': 'lines',
'文件较大，仅读取前 ': 'Large file — read the first ', ' 行': ' lines',
'这类文件不按文本解析': 'This type is not parsed as text',
'压缩包内含 ': 'Archive contains ', ' 个条目': ' entries',
'仅列出清单，不解压全部内容': 'Entries listed only — contents are not extracted',
'路径': 'Path', '原始大小（字节）': 'Raw size (bytes)',
'已从文档 XML 中提取纯文本，样式、图片与分页不在此显示':
    'Plain text extracted from the document XML; styling, images and pagination are not shown',
'无法解析该文档正文：': 'Could not parse the document body: ',
'文件大小 ': 'File size ', '，可下载后用对应软件打开。': ' — download it and open with the right app.',
'—— 第 ': '— page ', ' 页 ——': ' —',
'文档里没有提取到可见文本': 'No visible text found in the document',
'不是有效的 ZIP 容器': 'Not a valid ZIP container', '条目头损坏': 'Corrupt entry header',
'不支持的压缩方式': 'Unsupported compression method', '当前浏览器不支持解压': 'Decompression is not supported here',
'容器里找不到正文部件': 'No body part found in the container',
'十六进制视图 · 显示前 ': 'Hex view · showing first ', ' 字节（共 ': ' bytes of ',
' 字节）': ' bytes)',

# ---- 文件类型（根据扩展名显示的名称） ----
'C/C++ 头': 'C/C++ header', 'C++ 头': 'C++ header', '配置': 'Config', '纯文本': 'Plain text',
'汇编': 'Assembly', 'Word 文档': 'Word document', 'ODF 文本': 'ODF text', 'RTF 文档': 'RTF document',
'Excel 表格': 'Excel spreadsheet', 'ODF 表格': 'ODF spreadsheet', 'ODF 演示': 'ODF presentation',
'PDF 文档': 'PDF document', '电子书': 'E-book', 'PNG 图片': 'PNG image', 'JPEG 图片': 'JPEG image',
'GIF 图片': 'GIF image', 'BMP 图片': 'BMP image', 'WebP 图片': 'WebP image', '图标': 'Icon',
'AVIF 图片': 'AVIF image', 'TIFF 图片': 'TIFF image', 'SVG 矢量图': 'SVG vector',
'ZIP 压缩包': 'ZIP archive', 'RAR 压缩包': 'RAR archive', '7-Zip 压缩包': '7-Zip archive',
'tar 归档': 'tar archive', 'gzip 归档': 'gzip archive', 'bzip2 归档': 'bzip2 archive',
'xz 归档': 'xz archive', 'JAR 包': 'JAR package', 'Android 包': 'Android package',
'VS Code 插件': 'VS Code extension', 'Rhino 插件包': 'Rhino plug-in package',
'浏览器扩展': 'Browser extension', 'Windows 安装包': 'Windows installer',
'光盘镜像': 'Disc image', 'macOS 镜像': 'macOS image', 'MP4 视频': 'MP4 video',
'WebM 视频': 'WebM video', 'QuickTime 视频': 'QuickTime video', 'Matroska 视频': 'Matroska video',
'AVI 视频': 'AVI video', 'WMV 视频': 'WMV video', 'Flash 视频': 'Flash video', 'M4V 视频': 'M4V video',
'MP3 音频': 'MP3 audio', 'WAV 音频': 'WAV audio', 'FLAC 音频': 'FLAC audio', 'AAC 音频': 'AAC audio',
'OGG 音频': 'OGG audio', 'M4A 音频': 'M4A audio', 'TrueType 字体': 'TrueType font',
'OpenType 字体': 'OpenType font', 'WOFF 字体': 'WOFF font', 'WOFF2 字体': 'WOFF2 font',
'EOT 字体': 'EOT font', 'HEX 固件': 'HEX firmware', '二进制镜像': 'Binary image',
'ELF 可执行': 'ELF executable', 'Windows 可执行': 'Windows executable',
'Windows 动态库': 'Windows DLL', '共享库': 'Shared library', '静态库': 'Static library',
'调试符号': 'Debug symbols', '目标文件': 'Object file', '数据库': 'Database',
'SQLite 数据库': 'SQLite database', 'Access 数据库': 'Access database', 'Keil 工程': 'Keil project',
'Keil 选项': 'Keil options', 'STM32CubeMX 配置': 'STM32CubeMX config', 'Rhino 模型': 'Rhino model',
'AutoCAD 图纸': 'AutoCAD drawing', 'CAD 交换文件': 'CAD exchange file', 'STL 模型': 'STL model',
'STEP 模型': 'STEP model', 'IGES 模型': 'IGES model', 'CSV 表格': 'CSV table',
'TSV 表格': 'TSV table', '日志': 'Log', '依赖锁定': 'Dependency lockfile',

# ---- 关于 / 帮助 ----
'把任意本地代码目录渲染成': 'Renders any local code folder as a ',
'可下钻的矩形地图': 'drillable rectangle map',
'：面积越大体量越大，颜色代表语言，双击即可逐层深入。': ': bigger area means bigger weight, colour means language, double-click to go deeper.',
'· 地图 / 列表双视图，代码行 · 体积 · 文件数三种度量':
    '· Map and list views; lines, size and file count metrics',
'· 60+ 语言识别，6 套皮肤 × 6 套语言配色，强调色自定义':
    '· 60+ languages; 6 skins × 6 language palettes; custom accent colour',
'· 内置预览：代码高亮 / 图片 / 音视频 / PDF / Office 文档 / CSV / 压缩包':
    '· Built-in preview: highlighted code, images, audio/video, PDF, Office, CSV, archives',
'· 右键复制路径 · 导出 PNG · 一键忽略 node_modules 等依赖目录':
    '· Copy paths from the context menu · export PNG · ignore node_modules and friends',
'隐私承诺：全程在本机运行，': 'Privacy: everything runs locally — ',
'不联网、不上传任何文件、无遥测、无常驻进程': 'no network, no uploads, no telemetry, no background process',
'© 2026 Meteor · 保留所有权利': '© 2026 Meteor · All rights reserved',
'原生 Win32 + WebView2': 'Native Win32 + WebView2',
'版本 1.1.2 (x64) · 单文件 · 免安装': 'Version 1.1.2 (x64) · single file · no install',
'快捷键': 'Shortcuts', '聚焦搜索框': 'Focus search', '关闭弹层 / 预览': 'Close overlay / preview',
'单击': 'Click', '双击': 'Double-click', '查看详情': 'See details', '进入目录': 'Enter folder',
'文件菜单': 'File menu', '视图菜单': 'View menu', '度量菜单': 'Metric menu', '帮助菜单': 'Help menu',
}
