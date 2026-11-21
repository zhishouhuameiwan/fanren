#!/bin/sh
# 截图口拍不到「施法、受击」这种一瞬间的姿势，所以只为拍照临时把八个姿势都指向同一帧 k，
# 在深处蛇之战的「中途」时刻（她 0 血、没有菜单、两只像素敌人还在）各拍一张；拍完把正式索引拷回去。
cd E:/fanren/fanren/build-s7/sprite-test/root
EXE=E:/fanren/.claude/worktrees/happy-maxwell-8427c8/fanren/build-int/fanren.exe
O=E:/fanren/fanren/docs/heroine-art/samples/round4/shots
for k in 0 1 2 3 4 5 6 7; do
  python - $k <<'PY'
import json, sys
k = int(sys.argv[1])
d = json.load(open('../index.trial4.json', encoding='utf-8'))
b = d['sheets']['chen_q_hd']['battle']
for p in b: b[p] = [k]
json.dump(d, open('assets/art/sprites/hd/index.json', 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
PY
  "$EXE" --assets . --load saves/ch07-jindi.sav --scene battle:be07_shenchu_she:mid --screenshot $O/forced_$k.png
done
cp ../index.trial4.json assets/art/sprites/hd/index.json
