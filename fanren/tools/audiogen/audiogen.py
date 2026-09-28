# -*- coding: utf-8 -*-
"""程序合成音频生成器：全部 BGM 与音效，确定性，同样的输入逐样本相同。

    python tools/audiogen/audiogen.py                     # 全部生成到 assets/bgm、assets/sfx，并自检
    python tools/audiogen/audiogen.py --only bgm_village,break
    python tools/audiogen/audiogen.py --list              # 列出全部 id
    python tools/audiogen/audiogen.py --out D:/tmp/audio  # 生成到别处（下面照样分 bgm/ sfx/）
    python tools/audiogen/audiogen.py --verify            # 不生成，只检查已有文件
    python tools/audiogen/audiogen.py --verify --plots D:/tmp/plots   # 顺带出波形/响度/声谱图
    python tools/audiogen/audiogen.py --determinism       # 两个独立进程各渲染一遍，逐样本比对
    python tools/audiogen/audiogen.py --report --only bgm_town   # 打印声部配平记录

退出码：自检有任何一项 FAIL，或确定性比对不一致，返回 1。

依赖：numpy、scipy、soundfile（libsndfile ≥ 1.1，带 Vorbis 编码）。matplotlib 只在 --plots 时用。
为什么输出 .ogg：引擎按 .ogg/.mp3/.wav/.flac 的顺序找资源（src/engine/Engine.cpp 的 audioFor），
十八首 BGM 若是 WAV 要两百多 MB；Vorbis 质量 0.42 时全部 BGM 在 25 MB 以内。
"""
from __future__ import annotations

import argparse
import os
import sys
import time
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

REPO = HERE.parents[1]
BGM_QUALITY = 0.42
SFX_QUALITY = 0.5


def _build(item_id: str, out_dir: str, dry: bool = False) -> dict:
    """在工作进程里渲染一个条目；dry=True 只渲染不写盘（确定性比对用）。"""
    import numpy as np  # noqa: F401  工作进程里按需导入，主进程 --list 不必加载 scipy

    import catalog
    from encode import pcm_hash, write_ogg
    from sfx_common import finalize_sfx

    t0 = time.time()
    if item_id in catalog.BGM:
        import mixer
        song = catalog.BGM[item_id]()
        pcm, info = mixer.render_song(song)
        sub, quality = "bgm", BGM_QUALITY
    elif item_id in catalog.SFX:
        from score import seed_of
        sfx = catalog.SFX[item_id]
        raw = sfx.render(np.random.default_rng(seed_of("sfx", item_id)))
        pcm = finalize_sfx(raw, sfx.target)
        info = {}
        sub, quality = "sfx", SFX_QUALITY
    else:
        raise KeyError(item_id)
    result = {"id": item_id, "kind": sub, "render_s": time.time() - t0, **info}
    if dry:
        result["pcm_sha256"] = pcm_hash(pcm)
        return result
    result.update(write_ogg(os.path.join(out_dir, sub, item_id + ".ogg"), pcm, quality,
                            circular=(sub == "bgm")))
    return result


def _selected(only: str | None) -> list[str]:
    import catalog
    everything = list(catalog.BGM) + list(catalog.SFX)
    if not only:
        return everything
    wanted = [s.strip() for s in only.split(",") if s.strip()]
    unknown = [w for w in wanted if w not in catalog.BGM and w not in catalog.SFX]
    if unknown:
        raise SystemExit(f"未知的 id：{', '.join(unknown)}（--list 看全部）")
    return wanted


def _run_pool(ids: list[str], out_dir: str, jobs: int, dry: bool) -> list[dict]:
    # 慢的（BGM）先派，免得最后一首长曲子独自拖尾。
    import catalog
    order = sorted(ids, key=lambda i: 0 if i in catalog.BGM else 1)
    with ProcessPoolExecutor(max_workers=max(1, min(jobs, len(order)))) as pool:
        futures = {i: pool.submit(_build, i, out_dir, dry) for i in order}
        return [futures[i].result() for i in ids]


def cmd_list() -> int:
    import catalog
    print(f"{'类别':<5}{'id':<20}{'时长':>7}  用途")
    for sid, fn in catalog.BGM.items():
        song = fn()
        print(f"{'bgm':<6}{sid:<20}{song.length / 44100:>6.1f}s  {song.title}（{song.mood}；{song.mode}，"
              f"每分钟 {song.bpm:g} 拍）")
    for sid, sfx in catalog.SFX.items():
        print(f"{'sfx':<6}{sid:<20}{'':>7}  {sfx.title}")
    return 0


def cmd_verify(out_dir: str, ids: list[str], plots: str | None) -> int:
    import catalog
    import verify
    rows = []
    for i in ids:
        sub = "bgm" if i in catalog.BGM else "sfx"
        path = os.path.join(out_dir, sub, i + ".ogg")
        if not os.path.exists(path):
            print(f"缺文件：{path}")
            rows.append({"path": path, "problems": ["缺文件"]})
            continue
        rows.append(verify.analyze(path, catalog.spec_of(i)))
    present = [r for r in rows if "dur" in r]
    print(verify.table(present))
    bgm_bytes = sum(os.path.getsize(r["path"]) for r in present if os.sep + "bgm" + os.sep in r["path"])
    sfx_bytes = sum(os.path.getsize(r["path"]) for r in present if os.sep + "sfx" + os.sep in r["path"])
    print(f"\nBGM 合计 {bgm_bytes / 1e6:.2f} MB，音效合计 {sfx_bytes / 1e6:.2f} MB")
    failed = [r for r in rows if r["problems"]]
    if plots:
        bgm_paths = [r["path"] for r in present if os.sep + "bgm" + os.sep in r["path"]]
        sfx_paths = [r["path"] for r in present if os.sep + "sfx" + os.sep in r["path"]]
        for k in range(0, len(bgm_paths), 6):
            verify.plot(bgm_paths[k:k + 6], os.path.join(plots, f"bgm_{k // 6 + 1}.png"), "BGM")
        for k in range(0, len(sfx_paths), 12):
            verify.plot(sfx_paths[k:k + 12], os.path.join(plots, f"sfx_{k // 12 + 1}.png"), "SFX")
        print(f"图已写到 {plots}")
    print(f"\n自检：{len(rows) - len(failed)} / {len(rows)} 通过" + ("" if not failed else "，有 FAIL"))
    return 1 if failed else 0


def main(argv: list[str] | None = None) -> int:
    # 中文 Windows 控制台是 GBK：个别字符编不出来时替换掉，而不是整个命令崩在 print 上。
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(errors="replace")
    ap = argparse.ArgumentParser(description="程序合成 BGM 与音效（确定性）")
    ap.add_argument("--only", help="逗号分隔的 id；缺省为全部")
    ap.add_argument("--list", action="store_true", help="列出全部 id")
    ap.add_argument("--out", default=str(REPO / "assets"), help="输出根目录（缺省 assets/）")
    ap.add_argument("--verify", action="store_true", help="只检查已生成的文件")
    ap.add_argument("--plots", help="--verify 时把波形/响度/声谱图写到这个目录")
    ap.add_argument("--determinism", action="store_true", help="两个独立进程池各渲染一遍，比 PCM 哈希")
    ap.add_argument("--report", action="store_true", help="打印 BGM 的声部配平记录")
    ap.add_argument("--jobs", type=int, default=max(1, min(12, (os.cpu_count() or 2) - 2)))
    args = ap.parse_args(argv)

    if args.list:
        return cmd_list()
    ids = _selected(args.only)
    if args.verify:
        return cmd_verify(args.out, ids, args.plots)
    if args.report:
        import catalog
        import mixer
        for i in ids:
            if i not in catalog.BGM:
                continue
            rep: list = []
            _, info = mixer.render_song(catalog.BGM[i](), rep)
            print(f"{i}: {info}")
            for name, inst, measured, level in rep:
                print(f"    {name:<14}{inst:<18}自身 {measured:7.1f} LUFS → 相对 {level:+.1f} LU")
        return 0
    if args.determinism:
        first = _run_pool(ids, args.out, args.jobs, dry=True)
        second = _run_pool(ids, args.out, args.jobs, dry=True)
        bad = [a["id"] for a, b in zip(first, second) if a["pcm_sha256"] != b["pcm_sha256"]]
        for a in first:
            print(f"{a['id']:<20}{a['pcm_sha256'][:16]}")
        if bad:
            print(f"确定性：不一致 {bad}")
            return 1
        print(f"确定性：{len(first)} 条，两个独立进程池渲染的 PCM 逐样本一致")
        return 0

    t0 = time.time()
    results = _run_pool(ids, args.out, args.jobs, dry=False)
    for r in results:
        extra = f"  LUFS {r['lufs']:.2f}  限幅 {r['limit_db']:.1f} dB" if r["kind"] == "bgm" else ""
        print(f"{r['id']:<20}{r['bytes'] / 1024:8.0f} KB  渲染 {r['render_s']:5.1f}s  解码峰值 "
              f"{r['decoded_peak_db']:6.2f}  压低 {r['trim_db']:5.2f} dB{extra}")
    print(f"生成 {len(results)} 条，用时 {time.time() - t0:.1f}s\n")
    return cmd_verify(args.out, ids, None)


if __name__ == "__main__":
    sys.exit(main())
