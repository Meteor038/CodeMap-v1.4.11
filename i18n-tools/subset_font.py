# -*- coding: utf-8 -*-
"""
为繁体中文 / 日文生成点阵字体子集：
  * 字符集 = 该语言词典全部译文的字符 + 四种语言自称 + 语言包示例 + 假名/标点区段
  * 源字体 = Fusion Pixel 12px proportional 的 zh_tw / ja 变体
  * 输出 woff2 + base64，供内嵌 @font-face
"""
import os, re, json, base64, subprocess, sys
from fontTools import subset

ROOT = r'C:\Users\MR\WorkBuddy\2026-09-17-15-03-54'
FONT = os.path.join(ROOT, '_font')
HP = os.path.join(ROOT, 'CodeMap-source', 'CodeMap.html')

# ---- 1. 读取 node 提取的词典 JSON ----
import json as _json
I18N = _json.load(open(os.path.join(FONT, 'i18n.json'), encoding='utf-8'))
print('词典语言:', list(I18N.keys()))
for k, v in I18N.items():
    print('   %-6s 条目 %d' % (k, len(v.get('m', {}))))

# ---- 2. 通用附加字符 ----
ENDONYMS = '简体中文繁體中文日本語English'
EXAMPLE = '{"id":"ja-extra","name":"日本語 (拡張)","dict":{"打开文件夹":"フォルダを開く"}}'
FONT_NAMES = '系统默认微软雅黑微软正黑思源黑体等宽通用自定义恢复默认界面文字代码等宽数字与点阵混合文本'
def ranges(*rs):
    out = set()
    for a, b in rs:
        out.update(chr(c) for c in range(a, b + 1))
    return out
UNIVERSAL = ranges((0x3041, 0x309F), (0x30A0, 0x30FF), (0x3000, 0x303F), (0xFF00, 0xFFEF))
EXTRA = set(ENDONYMS + EXAMPLE + FONT_NAMES)

def charset_of(lang):
    cs = set()
    for v in I18N[lang]['m'].values():
        cs.update(v)
    cs |= EXTRA | UNIVERSAL
    # 只保留字体可能覆盖的范畴（去掉 ASCII，交给 DotMatrix）
    return {c for c in cs if ord(c) > 0x2000}

targets = [
    ('zh-TW', 'fusion-pixel-12px-proportional-zh_tw.ttf.woff2', 'FusionPixel12TW'),
    ('ja',    'fusion-pixel-12px-proportional-ja.ttf.woff2',    'FusionPixel12JA'),
]

os.makedirs(os.path.join(FONT, 'out'), exist_ok=True)
results = {}
for lang, fname, fam in targets:
    cs = charset_of(lang)
    src = os.path.join(FONT, fname)
    out = os.path.join(FONT, 'out', fam + '.woff2')
    text = ''.join(sorted(cs))
    subprocess.run([
        sys.executable, '-m', 'fontTools.subset', src,
        '--text=' + text,
        '--flavor=woff2',
        '--layout-features=*',
        '--no-hinting',
        '--desubroutinize',
        '--output-file=' + out,
    ], check=True, capture_output=True)
    raw = open(out, 'rb').read()
    b64 = base64.b64encode(raw).decode()
    results[lang] = {'fam': fam, 'chars': len(cs), 'woff2': len(raw), 'b64': len(b64), 'path': out}
    print()
    print('%s → %s' % (lang, fam))
    print('   字符数 %d   woff2 %d 字节   base64 %d 字节' % (len(cs), len(raw), len(b64)))

print()
tot = sum(r['b64'] for r in results.values())
print('两个子集 base64 合计: %d 字节 (%.0f KB)' % (tot, tot/1024))
json.dump({k: {kk: vv for kk, vv in v.items()} for k, v in results.items()},
          open(os.path.join(FONT, 'subset-info.json'), 'w', encoding='utf-8'),
          ensure_ascii=False, indent=1)
print('信息已写出 _font/subset-info.json')
