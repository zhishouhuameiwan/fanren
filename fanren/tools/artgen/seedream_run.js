#!/usr/bin/env node
/*
 * 用本机装的火山方舟 AgentPlan Seedream skill 出图（用户 2026-11-20 定：以后出图都走这个 skill，模型 Seedream 5.5）。
 *
 * skill 的 generate.js 只认命令行里的 URL / Base64 Data URI 参考图；我们的参考图（地图底稿、三视图）是几 MB 的本地 PNG，
 * 塞进命令行会超过 Windows 的 32767 字符上限。这里在进程内把本地图转成 Data URI，再把 generate.js 当主模块跑，
 * skill 本身一字不改。
 *
 *   node tools/artgen/seedream_run.js --prompt-file <提示词.txt> [--ref <图>]... [--size 2048x1152]
 *        [--background transparent] [--out <复制到这里.png>] [--dry-run]
 *
 * 固定参数（见记忆 seedream-skill-setup）：--model doubao-seedream-5-5-261028 --offline-models
 * --optimize false（不让本地追加「电影质感」之类）--watermark false。
 * skill 位置：环境变量 SEEDREAM_SKILL_DIR，缺省 <仓库外层>/.agents/skills/byted-ark-seedream-skill。
 * stdout 照 generate.js 原样输出 JSON；给了 --out 时另把第一张图复制过去，并在最后一行打印 SEEDREAM_OUT <路径>。
 */
'use strict';
const fs = require('fs');
const path = require('path');
const Module = require('module');

const MODEL = 'doubao-seedream-5-5-261028';
const MAX_REF_BYTES = 10 * 1024 * 1024;

function parse(argv) {
  const o = { refs: [], pass: [] };
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    const next = () => argv[++i];
    if (a === '--prompt-file') o.promptFile = next();
    else if (a === '--prompt') o.prompt = next();
    else if (a === '--ref') o.refs.push(next());
    else if (a === '--size') o.size = next();
    else if (a === '--background') o.background = next();
    else if (a === '--out') o.out = next();
    else if (a === '--dry-run') o.dryRun = true;
    else throw new Error(`seedream_run: 认不出的参数 ${a}`);
  }
  if (!o.prompt && !o.promptFile) throw new Error('seedream_run: 要 --prompt 或 --prompt-file');
  return o;
}

function dataUri(file) {
  const buf = fs.readFileSync(file);
  if (buf.length > MAX_REF_BYTES) throw new Error(`seedream_run: 参考图 ${file} 有 ${buf.length} 字节，超过 10MB，先缩小`);
  const ext = path.extname(file).toLowerCase();
  const mime = ext === '.jpg' || ext === '.jpeg' ? 'image/jpeg' : ext === '.webp' ? 'image/webp' : 'image/png';
  return `data:${mime};base64,${buf.toString('base64')}`;
}

const opts = parse(process.argv.slice(2));
if (opts.out) opts.out = path.resolve(opts.out);   // 下面会 chdir 到 skill 目录，先把相对路径钉死
const skillDir = process.env.SEEDREAM_SKILL_DIR ||
  path.resolve(__dirname, '..', '..', '..', '.agents', 'skills', 'byted-ark-seedream-skill');
const generate = path.join(skillDir, 'scripts', 'generate.js');
if (!fs.existsSync(generate)) throw new Error(`seedream_run: 找不到 ${generate}（设 SEEDREAM_SKILL_DIR）`);

const prompt = opts.prompt || fs.readFileSync(opts.promptFile, 'utf8').trim();
const args = ['--model', MODEL, '--offline-models', '--optimize', 'false', '--watermark', 'false', '--prompt', prompt];
if (opts.refs.length) args.push('--reference_images', JSON.stringify(opts.refs.map(dataUri)));
if (opts.size) args.push('--size', opts.size);
if (opts.background) args.push('--background', opts.background);
if (opts.dryRun) args.push('--dry-run');

if (opts.out) {
  // 截下 generate.js 打的 JSON，取第一张图复制到 --out
  let captured = '';
  const write = process.stdout.write.bind(process.stdout);
  process.stdout.write = (chunk, ...rest) => { captured += chunk; return write(chunk, ...rest); };
  process.on('exit', () => {
    try {
      const start = captured.indexOf('{');
      const result = JSON.parse(captured.slice(start));
      const images = result.images || (result.data && result.data.images) || [];
      const local = images.map((im) => im.local_path || im.path || im.file).find(Boolean);
      if (local) {
        fs.mkdirSync(path.dirname(opts.out), { recursive: true });
        fs.copyFileSync(local, opts.out);
        write(`\nSEEDREAM_OUT ${opts.out}\n`);
      } else if (!opts.dryRun) {
        write('\nSEEDREAM_OUT_NONE（结果里没有本地文件，见上面的 JSON）\n');
      }
    } catch (e) {
      write(`\nSEEDREAM_OUT_NONE（解析结果失败：${e.message}）\n`);
    }
  });
}

// generate.js 按开始那一秒给下载文件命名，同一秒开始的并行调用会写到同一个文件、互相覆盖（2026-11-20 并行出图时
// 钟吾的一张就被盖掉过）。每次调用各用一个下载目录：<ARK_SEEDREAM_SAVE_PATH 或缺省>/run_<毫秒>_<pid>。
{
  const base = process.env.ARK_SEEDREAM_SAVE_PATH || path.resolve(__dirname, '..', '..', 'build-s7', 'seedream-out');
  process.env.ARK_SEEDREAM_SAVE_PATH = path.join(path.resolve(base), `run_${Date.now()}_${process.pid}`);
}

process.argv = [process.argv[0], generate, ...args];
process.chdir(skillDir);
Module.runMain();
