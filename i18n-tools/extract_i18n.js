const fs = require('fs');
const vm = require('vm');
const HP = 'C:/Users/MR/WorkBuddy/2026-09-17-15-03-54/CodeMap-source/CodeMap.html';
const OUT = 'C:/Users/MR/WorkBuddy/2026-09-17-15-03-54/_font/i18n.json';
const h = fs.readFileSync(HP, 'utf8');
const m = h.match(/const I18N = (\{[\s\S]*?\n\});/);
if (!m) { console.log('未找到 I18N'); process.exit(1); }
const I18N = vm.runInNewContext('(' + m[1] + ')');
const out = {};
for (const k of Object.keys(I18N)) out[k] = { n: I18N[k].n, m: I18N[k].m };
fs.writeFileSync(OUT, JSON.stringify(out), 'utf8');
console.log('已写出词典 JSON:', Object.keys(out).join(', '));
