# -*- coding: utf-8 -*-
"""
生成 i18n 词典 JS 块：
  * 简体：空表（原文即译文，天然回退）
  * 繁体：OpenCC(s2twp) 转换 + 术语覆盖
  * 英文 / 日文：来自 _d_en.py / _d_ja.py
  * 新增 UI（语言/字体/首次启动）：四语原生编写
"""
import json, sys, os
ROOT = r'C:\Users\MR\WorkBuddy\2026-09-17-15-03-54'
sys.path.insert(0, os.path.join(ROOT, 'i18n-tools'))   # 词典源文件放在 i18n-tools/ 下
from opencc import OpenCC
from d_en import EN
from d_ja import JA

cc = OpenCC('s2twp')

# ---------- 新增 UI：语言 / 字体 / 首次启动 ----------
# 结构：简体原文 -> {tw, en, ja}
NEW_UI = {
    # 语言
    '语言': {'tw': '語言', 'en': 'Language', 'ja': '言語'},
    '界面语言': {'tw': '介面語言', 'en': 'Interface language', 'ja': '表示言語'},
    '语言包': {'tw': '語言包', 'en': 'Language packs', 'ja': '言語パック'},
    '导入语言包…': {'tw': '匯入語言包…', 'en': 'Import language pack…', 'ja': '言語パックを読み込む…'},
    '导出模板': {'tw': '匯出範本', 'en': 'Export template', 'ja': 'テンプレートを書き出す'},
    '内置': {'tw': '內建', 'en': 'Built-in', 'ja': '内蔵'},
    '已安装': {'tw': '已安裝', 'en': 'Installed', 'ja': 'インストール済み'},
    '卸载': {'tw': '卸載', 'en': 'Remove', 'ja': '削除'},
    '管理语言包': {'tw': '管理語言包', 'en': 'Manage language packs', 'ja': '言語パックを管理'},
    '选择界面语言': {'tw': '選擇介面語言', 'en': 'Choose your language', 'ja': '表示言語を選択'},
    '随时可以在「外观」面板里更改语言、安装语言包或调整字体。':
        {'tw': '隨時可以在「外觀」面板裡更改語言、安裝語言包或調整字型。',
         'en': 'You can change the language, install language packs or adjust fonts any time from the Appearance panel.',
         'ja': '言語の変更・言語パックの導入・フォントの調整は、いつでも「外観」パネルから行えます。'},
    '开始使用': {'tw': '開始使用', 'en': 'Get started', 'ja': 'はじめる'},
    '语言包格式不正确：需包含 name 与 dict 字段':
        {'tw': '語言包格式不正確：需包含 name 與 dict 欄位',
         'en': 'Invalid language pack: "name" and "dict" fields are required',
         'ja': '言語パックの形式が正しくありません：「name」と「dict」が必要です'},
    '语言包加载失败：': {'tw': '語言包載入失敗：', 'en': 'Failed to load language pack: ', 'ja': '言語パックの読み込みに失敗：'},
    '已安装语言包：': {'tw': '已安裝語言包：', 'en': 'Installed language pack: ', 'ja': '言語パックをインストールしました：'},
    '已卸载语言包：': {'tw': '已卸載語言包：', 'en': 'Removed language pack: ', 'ja': '言語パックを削除しました：'},
    '语言包已导出到下载目录': {'tw': '語言包已匯出到下載目錄', 'en': 'Language pack exported to your downloads folder', 'ja': '言語パックをダウンロードフォルダに書き出しました'},
    '该语言包缺少部分译文，缺失处将回退为简体中文':
        {'tw': '該語言包缺少部分譯文，缺失處將回退為簡體中文',
         'en': 'This pack is missing some entries; missing text falls back to Simplified Chinese',
         'ja': '一部の訳が欠けています。欠けた箇所は簡体字中国語で表示されます'},
    '语言包文件（*.json）': {'tw': '語言包檔案（*.json）', 'en': 'Language pack (*.json)', 'ja': '言語パック (*.json)'},
    # 字体
    '字体': {'tw': '字型', 'en': 'Fonts', 'ja': 'フォント'},
    '界面文字': {'tw': '介面文字', 'en': 'Interface text', 'ja': 'UI テキスト'},
    '代码等宽': {'tw': '程式碼等寬', 'en': 'Code (monospace)', 'ja': 'コード（等幅）'},
    '数字与点阵': {'tw': '數字與點陣', 'en': 'Numerals & dot matrix', 'ja': '数字・ドット'},
    '混合文本': {'tw': '混合文字', 'en': 'Mixed text', 'ja': '混在テキスト'},
    '恢复默认': {'tw': '恢復預設', 'en': 'Restore defaults', 'ja': '既定に戻す'},
    '自定义…': {'tw': '自訂…', 'en': 'Custom…', 'ja': 'カスタム…'},
    '输入字体名，多个用逗号分隔': {'tw': '輸入字型名稱，多個用逗號分隔', 'en': 'Font names, comma separated', 'ja': 'フォント名（カンマ区切りで複数可）'},
    '字体已更新': {'tw': '字型已更新', 'en': 'Fonts updated', 'ja': 'フォントを更新しました'},
    '预览': {'tw': '預覽', 'en': 'Preview', 'ja': 'プレビュー'},
    '字体设置只影响本机显示，语言包作者可在包内附带推荐字体。':
        {'tw': '字型設定只影響本機顯示，語言包作者可在包內附帶推薦字型。',
         'en': 'Font settings only affect this machine. Language packs may ship recommended fonts.',
         'ja': 'フォント設定はこの PC の表示にのみ影響します。言語パックに推奨フォントを含めることもできます。'},
}


# ---------- 补充：字体条目名与语言包 UI（英文模式残留缺口） ----------
NEW_UI.update({
    '系统默认':   {'tw': '系統預設', 'en': 'System default',    'ja': 'システム既定'},
    '微软雅黑':   {'tw': '微軟雅黑', 'en': 'Microsoft YaHei',   'ja': 'Microsoft YaHei'},
    '微软正黑':   {'tw': '微軟正黑', 'en': 'Microsoft JhengHei','ja': 'Microsoft JhengHei'},
    '思源黑体':   {'tw': '思源黑體', 'en': 'Source Han Sans',   'ja': 'Source Han Sans'},
    '等宽通用':   {'tw': '等寬通用', 'en': 'Generic monospace', 'ja': '汎用等幅'},
    '安装':       {'tw': '安裝',     'en': 'Install',           'ja': 'インストール'},
    '字体设置':   {'tw': '字型設定', 'en': 'Font settings',     'ja': 'フォント設定'},
})


# ---------- 补充：Everything 风格搜索 ----------
NEW_UI.update({
    '匹配 {n} 项':            {'tw': '符合 {n} 項', 'en': 'Matches: {n}', 'ja': '一致 {n} 件'},
    '仅显示前 {n} 项，共 {m} 项': {'tw': '僅顯示前 {n} 項，共 {m} 項', 'en': 'Showing first {n} of {m}',
                                  'ja': '先頭 {n} 件を表示（全 {m} 件）'},
    '匹配项均为文件夹，请切换到列表视图查看': {
        'tw': '符合項均為資料夾，請切換到清單檢視查看',
        'en': 'All matches are folders — switch to the list view',
        'ja': '一致した項目はすべてフォルダです。リスト表示に切り替えてください'},
    '搜索语法':               {'tw': '搜尋語法', 'en': 'Search syntax', 'ja': '検索構文'},
    '搜索语义对齐 Everything：空格 = 并且，| 表示或者，! 或 - 表示排除，"" 包住含空格的短语；* 与 ? 为通配符。': {
        'tw': '搜尋語意對齊 Everything：空格 = 並且，| 表示或者，! 或 - 表示排除，"" 包住含空格的詞組；* 與 ? 為萬用字元。',
        'en': 'Search semantics match Everything: space = AND, | = OR, ! or - = NOT, "" groups a phrase; * and ? are wildcards.',
        'ja': '検索の意味論は Everything に準拠：スペース = AND、| = OR、! または - = NOT、"" でフレーズを囲む、* と ? はワイルドカード。'},
    '操作符':                 {'tw': '運算子', 'en': 'Operator', 'ja': '演算子'},
    '说明':                   {'tw': '說明', 'en': 'Description', 'ja': '説明'},
    '按扩展名筛选（可多值：js;ts）': {'tw': '依副檔名篩選（可多值：js;ts）', 'en': 'Filter by extension (multiple: js;ts)',
                                    'ja': '拡張子で絞り込み（複数可：js;ts）'},
    '路径包含该文本':          {'tw': '路徑包含該文字', 'en': 'Path contains text', 'ja': 'パスに含まれる'},
    '上级文件夹包含该文本':     {'tw': '上層資料夾包含該文字', 'en': 'Parent folder contains text', 'ja': '親フォルダに含まれる'},
    '只匹配文件':             {'tw': '只符合檔案', 'en': 'Files only', 'ja': 'ファイルのみ'},
    '只匹配文件夹':            {'tw': '只符合資料夾', 'en': 'Folders only', 'ja': 'フォルダのみ'},
    '按体积：>1mb、<10kb、1mb..10mb': {'tw': '依體積：>1mb、<10kb、1mb..10mb', 'en': 'By size: >1mb, <10kb, 1mb..10mb',
                                      'ja': 'サイズ指定：>1mb、<10kb、1mb..10mb'},
    '按修改时间：today、>2026-01-01、2026-01-01..2026-06-30': {
        'tw': '依修改時間：today、>2026-01-01、2026-01-01..2026-06-30',
        'en': 'By date modified: today, >2026-01-01, 2026-01-01..2026-06-30',
        'ja': '更新日時指定：today、>2026-01-01、2026-01-01..2026-06-30'},
    '区分大小写（作为前缀开关）': {'tw': '區分大小寫（前置開關）', 'en': 'Case-sensitive (prefix switch)',
                                  'ja': '大文字と小文字を区別（接頭辞スイッチ）'},
    '正则匹配文件名':          {'tw': '以正規表達式比對檔名', 'en': 'Match filename by regex', 'ja': '正規表現でファイル名を照合'},
    '整个文件名匹配（含通配符）': {'tw': '整個檔名比對（含萬用字元）', 'en': 'Whole filename match (wildcards allowed)',
                                  'ja': 'ファイル名全体で照合（ワイルドカード可）'},
    '以…开头 / 以…结尾 / 整词匹配': {'tw': '以…開頭 / 以…結尾 / 整詞比對', 'en': 'Starts with / ends with / whole word',
                                    'ja': '〜で始まる / 〜で終わる / 単語単位'},
    '支持 ext: path: size: ! | * 等语法，详见「帮助 → 搜索语法」': {
        'tw': '支援 ext: path: size: ! | * 等語法，詳見「說明 → 搜尋語法」',
        'en': 'Supports ext:, path:, size:, !, |, * … see Help → Search syntax',
        'ja': 'ext: path: size: ! | * などの構文に対応。詳細は「ヘルプ → 検索構文」'},
})

# ---------- 组装 ----------
keys = sorted(set(list(EN.keys()) + list(JA.keys()) + list(NEW_UI.keys())))
print('词典键数:', len(keys))

TW_OVERRIDE = {
    '打开文件夹': '開啟資料夾', '文件夹': '資料夾', '文件': '檔案', '文件数': '檔案數',
    '视频': '影片', '音频': '音訊', '信息': '資訊', '默认': '預設', '网络': '網路',
    '原文件': '原始檔案', '文本': '文字', '日志': '日誌',
}

def tw_of(zh):
    if zh in TW_OVERRIDE:
        return TW_OVERRIDE[zh]
    return cc.convert(zh)

tw, en, ja, cn = {}, {}, {}, {}
for k in keys:
    # 简体：原文即译文，不建表（用空表 + 回退）
    if k in NEW_UI:
        tw[k] = NEW_UI[k]['tw']
        en[k] = NEW_UI[k]['en']
        ja[k] = NEW_UI[k]['ja']
    else:
        tw[k] = tw_of(k)
        if k in EN: en[k] = EN[k]
        if k in JA: ja[k] = JA[k]

missing_en = [k for k in keys if k not in en]
missing_ja = [k for k in keys if k not in ja]
print('缺英文:', len(missing_en), missing_en[:10])
print('缺日文:', len(missing_ja), missing_ja[:10])

# ---------- 输出 JS 块 ----------
def js_obj(d):
    return '{' + ','.join('%s:%s' % (json.dumps(k, ensure_ascii=False), json.dumps(v, ensure_ascii=False))
                          for k, v in sorted(d.items())) + '}'

block = (
    '/* ---------------- 多语言内核 ---------------- */\n'
    '/* 词典以**简体原文**为键：简体模式零词典、天然回退；语言包作者只需\n'
    '   提供 { "中文原文": "译文" } 即可，门槛最低。 */\n'
    'const I18N = {\n'
    "  'zh-CN': { n:'简体中文', m:{} },\n"
    "  'zh-TW': { n:'繁體中文', m:" + js_obj(tw) + ' },\n'
    "  'en':    { n:'English',  m:" + js_obj(en) + ' },\n'
    "  'ja':    { n:'日本語',   m:" + js_obj(ja) + ' }\n'
    '};\n'
)
out = r'C:\Users\MR\WorkBuddy\2026-09-17-15-03-54\_i18n_block.js'
open(out, 'w', encoding='utf-8').write(block)
print()
print('已写出', out, os.path.getsize(out), '字节')
print('繁体样例:', list(tw.items())[:6])
