# 第三方组件与许可声明

本文档列明 CodeMap 仓库中包含或依赖的第三方资源、其许可条款，以及再分发时的注意事项。

- **适用版本**：CodeMap v1.4.11
- **本文档范围**：仓库内实际分发的资源（源码、内嵌资源、素材文件）
- **不适用**：用户自行安装的软件、操作系统组件

---

## 1. 随仓库分发的第三方资源

### 1.1 内嵌字体

CodeMap 的界面使用像素字体渲染。为保持单文件形态，字体以 **base64 编码的 WOFF2 子集**内嵌在
`CodeMap-source/CodeMap.html` 的 `@font-face` 规则中；同时，用于重新生成子集的源文件位于
`i18n-tools/font-subset/`。

| 内嵌字族名 | 用途 | 上游项目 | 上游许可 |
|---|---|---|---|
| `FusionPixel12` | 简体中文界面 | [fusion-pixel-font](https://github.com/TakWolf/fusion-pixel-font)（12 px proportional 变体） | SIL Open Font License 1.1 |
| `FusionPixel12TW` | 繁体中文界面（`zh_tw` 字形） | 同上 | SIL Open Font License 1.1 |
| `FusionPixel12JA` | 日文界面（`ja` 字形） | 同上 | SIL Open Font License 1.1 |
| `DotMatrix` | 数字与点阵装饰（拉丁字符） | [Doto](https://fonts.google.com/specimen/Doto) | SIL Open Font License 1.1 |

**对本项目所做的修改**：仅做**字形子集化**（保留实际用到的字符，删除其余字形）与格式转换（TTF → WOFF2），
**未改动任何字形轮廓**。CSS 中的字族名（如 `DotMatrix`）是应用内部的引用别名，不修改字体文件本身的命名表。
子集化脚本见 `i18n-tools/subset_font.py`。

**分发的具体文件**

| 文件 | 内容 |
|---|---|
| `CodeMap-source/CodeMap.html` | 四套字体的 base64 WOFF2 子集（内嵌，不可单独提取为字体文件使用） |
| `i18n-tools/font-subset/FusionPixel12TW.woff2` | 繁体中文子集（独立文件，供重新生成使用） |
| `i18n-tools/font-subset/FusionPixel12JA.woff2` | 日文子集（同上） |

### 1.2 其他素材

| 文件 | 说明 | 来源 |
|---|---|---|
| `CodeMap-source/CodeMap.ico` | 应用图标 | 本项目原创（点阵构图） |
| `docs/screenshot.png` | 界面截图 | 本项目运行截图 |

---

## 2. 构建期依赖（不随仓库分发）

以下资源仅在**从源码构建**时需要，仓库内不包含，因此不构成再分发：

| 组件 | 权限方式 | 说明 |
|---|---|---|
| **Microsoft Edge WebView2 SDK**（NuGet：`Microsoft.Web.WebView2`） | Microsoft 软件许可条款 | 提供头文件与 `WebView2LoaderStatic.lib`。使用者需自行下载并放入 `build/webview2/` |
| **Microsoft Visual C++ 运行库**（静态链接） | Microsoft 可再分发组件条款 | 以 `/MT` 静态链接进 exe，最终产物不含外部 DLL |
| **Windows SDK**（`rc.exe`、头文件与库） | Microsoft SDK 许可 | 构建期工具 |

> 最终用户运行 `CodeMap.exe` 时，唯一的外部依赖是系统已安装的
> **Microsoft Edge WebView2 Runtime**（由微软分发，非本项目分发）。

---

## 3. 参考实现（非代码依赖）

| 项目 | 与本项目的关系 |
|---|---|
| [voidtools Everything](https://www.voidtools.com/)（作者 David Carpenter） | **仅参考其用户可见的搜索查询语法**（`ext:`、`path:`、`size:`、`dm:`、`!`、`\|` 等表达形式）。 |

**重要说明**

- Everything 是**免费软件，但其源代码未公开**（其二进制按类 MIT 许可分发）。
- 本项目的搜索内核（索引结构、查询解析、匹配求值）为**独立实现**，未使用、未修改、未反编译其任何代码。
- 本项目不含 Everything 的任何文件，也不链接其组件。
- 之所以在文档中注明，是因为查询语法的**表达形式**与其保持一致，便于熟悉 Everything 的用户直接迁移使用习惯。

---

## 4. 许可合规说明

### 4.1 本项目自身代码

以 [MIT 许可证](LICENSE) 发布。允许商业使用、修改、再分发，需保留版权声明与许可声明。

### 4.2 内嵌字体（SIL OFL 1.1）

SIL Open Font License 1.1 允许字体被**嵌入文档或软件**并在满足以下条件时再分发：

1. 保留字体的版权声明与许可声明；
2. 不得单独销售字体文件本身；
3. 若对字体做了修改，修改后的字体不得使用上游声明的 Reserved Font Name（如有）。

**本项目已采取的措施**

- 在本文档中列明字体的名称、上游项目与许可，作为版权与许可声明的载体；
- 子集化未改动字形轮廓；
- 字体仅用于界面渲染，未作为独立商品分发。

**完整的许可文本**请见上游仓库中的 `OFL.txt`：
<https://github.com/TakWolf/fusion-pixel-font> 与 <https://github.com/googlefonts/doto>。

> 若你计划**再分发本项目的 exe 或内嵌字体的页面**，建议一并附带上述 OFL 文本，以满足条款 1 的形式要求。

### 4.3 构建期依赖

WebView2 SDK 与 Visual C++ 运行库的使用受 Microsoft 许可条款约束；
若你重新分发构建产物，请自行确认所使用工具链版本的再分发条款。

---

## 5. 如何更新或替换内嵌字体

1. 从上游项目下载完整字体（TTF）；
2. 修改 `i18n-tools/subset_font.py` 中的字符集与输出路径；
3. 运行脚本生成 WOFF2 子集；
4. 将生成文件转 base64 并替换 `CodeMap-source/CodeMap.html` 中对应的 `@font-face` 数据；
5. 同步更新本文档 1.1 节的表格。

---

## 6. 联系方式

若你认为本仓库的使用方式侵犯了你的权益，请通过仓库的 Issue 提出，作者会及时处理。
