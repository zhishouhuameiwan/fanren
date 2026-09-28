"""人物头部：脸、发型、头饰、胡须。三向（F 正 / L 左侧 / B 背），右侧由左侧镜像。

头框 12×10，默认放在帧的 (2, 1)；脸宽 8、眼 1×2px。
发型与头饰是区分人物的第一手段（施工要求：轮廓一眼可辨），所以这里的模板
比身体模板多得多——同一身长衫，戴斗笠的是船家，扎马尾的是少年刀客，白发束在
脑后的是墨大夫。

每个部件模板带一个原点偏移 (ox, oy)（相对头框左上角），允许伸出头框：
斗笠比头宽、披发垂到背上、高髻顶到头框上面。

码表：
  脸   Q S s K 皮肤 3/2/1/0 级，e 眼，v 眉（只有 brows=thick 时画），p 腮红（blush 时画），
       n 胡茬（stubble 时画，否则是皮肤）
  发   h i j k 头发 2/3/1/0 级，T t 发带，S s 露出的耳朵/后颈（皮肤）
  饰   A B C D 头饰主料 3/2/1/0 级，E F G 头饰副料 3/2/1 级，T t 系带（头饰副料），
       U u 发带（与发髻上的发带同色），Z 金，X 玉，r 红，q 帽兜底下的黑影（墨色），
       w 黑影里透出的下巴（皮肤暗面）
  须   h i j k 胡须（发色）
"""

from __future__ import annotations

from dataclasses import dataclass

from sprites_core import tpl


@dataclass(frozen=True)
class Part:
    rows: tuple[str, ...]
    ox: int = 0
    oy: int = 0


def part(text: str, ox: int = 0, oy: int = 0) -> Part:
    return Part(tuple(tpl(text)), ox, oy)


# ===========================================================================
# 脸
# ===========================================================================

FACES: dict[str, dict[str, Part]] = {
    # 圆眼：孩子、少女、年轻人
    "round": {
        "F": part("""
            ............
            ....SSSS....
            ...SSSSSS...
            ..QSSSSSSs..
            ..QSSSSSSs..
            ..SvSSSSvs..
            ..SSeSSeSs..
            ..SpeSSeps..
            ..SnSSSSns..
            ...snnnss...
            """),
        "L": part("""
            ............
            .....SSSS...
            ....SSSSSS..
            ...SSSSSSSs.
            ...QSSSSSSs.
            ..QvSSSSSSs.
            ..SeSSSSSSs.
            .SSepSSSSs..
            ..SnnSSSss..
            ...snnSs....
            """),
    },
    # 细眼：成年男子——一道眉压着一个像素的眼，比圆眼老成
    "narrow": {
        # 眉只画外侧一格、斜压在眼角上：两格宽的眉压一格的眼，放大看像戴了墨镜
        "F": part("""
            ............
            ....SSSS....
            ...SSSSSS...
            ..QSSSSSSs..
            ..QSSSSSSs..
            ..SkSSSSks..
            ..SSeSSeSs..
            ..SSeSSeSs..
            ..SnSSSSns..
            ...snnnss...
            """),
        "L": part("""
            ............
            .....SSSS...
            ....SSSSSS..
            ...SSSSSSSs.
            ...QSSSSSSs.
            ..QkSSSSSSs.
            ..SeSSSSSSs.
            .SSeSSSSSs..
            ..SnnSSSss..
            ...snnSs....
            """),
    },
    # 老者：眼更细，眼下一道纹
    "old": {
        "F": part("""
            ............
            ....SSSS....
            ...SSSSSS...
            ..QSSSSSSs..
            ..QSsSSsSs..
            ..SSSSSSSs..
            ..SkkSSkks..
            ..SSeSSeSs..
            ..SnsSSsns..
            ...snnnss...
            """),
        "L": part("""
            ............
            .....SSSS...
            ....SSSSSS..
            ...SSsSSSSs.
            ...QSSSSSSs.
            ..QSSSSSSSs.
            ..kkSSSSSSs.
            .SSeSSSSSs..
            ..SnsSSSss..
            ...snnSs....
            """),
    },
    # 凶相：眉梢上挑，眼压得低
    "fierce": {
        "F": part("""
            ............
            ....SSSS....
            ...SSSSSS...
            ..QSSSSSSs..
            ..QSSSSSSs..
            ..SkSSSSks..
            ..SSkSSkSs..
            ..SSeSSeSs..
            ..SnSSSSns..
            ...snnnss...
            """),
        "L": part("""
            ............
            .....SSSS...
            ....SSSSSS..
            ...SSSSSSSs.
            ...QSSSSSSs.
            ..QkSSSSSSs.
            ..SkSSSSSSs.
            .SSeSSSSSs..
            ..SnnSSSss..
            ...snnSs....
            """),
    },
    # 空眼：傀儡、尸傀——眼是一道暗缝，没有眼珠
    "blank": {
        "F": part("""
            ............
            ....SSSS....
            ...SSSSSS...
            ..QSSSSSSs..
            ..QSSSSSSs..
            ..SSSSSSSs..
            ..SkkSSkks..
            ..SSSSSSSs..
            ..SSSSSSss..
            ...sSSSss...
            """),
        "L": part("""
            ............
            .....SSSS...
            ....SSSSSS..
            ...SSSSSSSs.
            ...QSSSSSSs.
            ..QSSSSSSSs.
            ..kkSSSSSSs.
            .SSSSSSSSs..
            ..SSSSSSss..
            ...sSSSs....
            """),
    },
}
FACE_BACK = part("""
    ............
    ....SSSS....
    ...SSSSSS...
    ..SSSSSSSs..
    ..SSSSSSSs..
    ..SSSSSSSs..
    ..SSSSSSSs..
    ..SSSSSSSs..
    ..sSSSSSss..
    ...sssss....
    """)


# ===========================================================================
# 发型
# ===========================================================================

HAIRS: dict[str, dict[str, Part]] = {}

# 男子发髻：头顶一个髻，髻根扎发带。韩立就是这一种，髻根的发带颜色是他的记号。
HAIRS["topknot"] = {
    "F": part("""
        .....hih....
        .....jhj....
        ....hTTTh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .ih......hj.
        .h........j.
        """),
    "L": part("""
        ......hih...
        ......jhj...
        .....hTTTh..
        ....hiihhhj.
        ...hiihhhhhj
        ..hhhhhhhhhj
        .....hhhhhhj
        ......Shhhj.
        ......Shhj..
        .......hj...
        """),
    "B": part("""
        .....hih....
        .....jhj....
        ....hTTTh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .hihhhhhhhj.
        .hhhhhhhhjj.
        .jhhhhhhhjj.
        ..jhhhhhjj..
        ...jjjjjj...
        """),
}
# 额前留一绺碎发的发髻：少年气一些（厉飞雨、吴剑鸣这类年轻武人）
HAIRS["topknot_fringe"] = {
    "F": part("""
        .....hih....
        .....jhj....
        ....hTTTh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .ihh.h...hj.
        .h...h....j.
        """),
    "L": part("""
        ......hih...
        ......jhj...
        .....hTTTh..
        ....hiihhhj.
        ...hiihhhhhj
        ..hhhhhhhhhj
        ..hh.hhhhhhj
        ..h...Shhhj.
        ......Shhj..
        .......hj...
        """),
    "B": HAIRS["topknot"]["B"],
}
# 马尾：髻上垂下一束，侧面与背面看得见那一束垂到肩
HAIRS["ponytail"] = {
    "F": part("""
        .....hih....
        .....jhj....
        ....hTTTh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .ihh.h...hj.
        .h...h....jj
        ..........jj
        ...........j
        """),
    "L": part("""
        ......hihh..
        ......jhjhh.
        .....hTTTjhh
        ....hiihhhjh
        ...hiihhhhhj
        ..hhhhhhhhhj
        ..hh.hhhhhhj
        ..h...Shhhjh
        ......Shhj.h
        .......hj..j
        """),
    "B": part("""
        .....hih....
        .....jhj....
        ....hTTTh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .hihhhhhhhj.
        .hhhhhhhhjj.
        .jhhhhhhhjj.
        ..jhhhhhjj..
        ...jjhhjj...
        .....hj.....
        .....hj.....
        .....j......
        """),
}
# 脑后松束：头发往后拢，在后颈松松束一截（墨大夫：「头发全白了，在脑后松松束着一截」）
HAIRS["bun_low"] = {
    "F": part("""
        ............
        ....hhhh....
        ...hiihhh...
        ..hiihjhhh..
        .hiih..hhhj.
        .ih......hj.
        .h........j.
        .h........j.
        """),
    "L": part("""
        ............
        .....hhhh...
        ....hiihhh..
        ...hiihhhhj.
        ..hhhhhhhhhj
        ....hhhhhhhj
        ......Shhhhj
        ......Shhhj.
        .......hhhj.
        ........hTh.
        ........hhj.
        .........j..
        """),
    "B": part("""
        ............
        ....hhhh....
        ...hiihhh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .hihhhhhhhj.
        .hhhhhhhhjj.
        .jhhhhhhhjj.
        ..jhhhhhjj..
        ...jjhhjj...
        .....TT.....
        .....hj.....
        .....j......
        """),
}
# 披发：头顶分开，两侧垂过下巴，背面盖住上背
HAIRS["loose"] = {
    "F": part("""
        ............
        ....hhhh....
        ...hiihhh...
        ..hiihjhhh..
        .hiih..hhhj.
        .ih......hj.
        .h........j.
        .h........j.
        .hj......jj.
        .hj......jj.
        .j........j.
        """),
    "L": part("""
        ............
        .....hhhh...
        ....hiihhh..
        ...hiihhhhj.
        ..hhhhhhhhhj
        ....hhhhhhhj
        ......Shhhhj
        ......hhhhhj
        ......hhhhhj
        .......hhhhj
        .......hhhj.
        ........hjj.
        ........jj..
        """),
    "B": part("""
        ............
        ....hhhh....
        ...hiihhh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .hihhhhhhhj.
        .hhhhhhhhjj.
        .hhhhhhhhjj.
        .hhhhhhhhjj.
        .jhhhhhhhjj.
        .jhhhhhhhjj.
        ..jhhhhhjj..
        ..jjhhhhjj..
        ...jjjjjj...
        """),
}
# 乱发：短而翘，马贼、打手、脚夫
HAIRS["wild"] = {
    "F": part("""
        ............
        ...h.hh.h...
        ..hhhihhhh..
        .hiihhhhhhj.
        .hiihhhhhhhj
        .ih.h..h.hj.
        .h........j.
        """),
    "L": part("""
        ............
        ....h.hh.h..
        ....hhihhhhh
        ...hiihhhhhj
        ..hhhhhhhhhj
        ..h.hhhhhhhj
        ......Shhhhj
        ......Shhhj.
        .......hhj..
        """),
    "B": part("""
        ............
        ...h.hh.h...
        ..hhhihhhh..
        .hiihhhhhhj.
        .hiihhhhhhhj
        .hihhhhhhhj.
        .hhhhhhhhjj.
        .jhhhhhhhjj.
        ..jhhhhhjj..
        ...jjjjjj...
        """),
}
# 光头
HAIRS["bald"] = {
    "F": part("""
        ............
        ....QSSS....
        ...QQSSSS...
        ..QQSSSSSs..
        ..............
        """),
    "L": part("""
        ............
        .....QSSS...
        ....QQSSSs..
        ...QQSSSSSs.
        ..........s.
        ......S...s.
        """),
    "B": part("""
        ............
        ....QSSS....
        ...QQSSSS...
        ..QSSSSSSs..
        ..SSSSSSSs..
        ..SSSSSSss..
        ..SSSSSSss..
        ..sSSSSSss..
        ...sSSSss...
        """),
}
# 总角：男童头顶两撮
HAIRS["tufts"] = {
    "F": part("""
        ..hh....hh..
        ..hih..hij..
        ...hhhhhhj..
        ..hiihhhhh..
        .hiihhhhhhj.
        .ihh.hh..hj.
        .h........j.
        """),
    "L": part("""
        ....hh..hh..
        ....hih.hij.
        .....hhhhhj.
        ....hiihhhj.
        ...hiihhhhhj
        ..hhhhhhhhhj
        ..hh.hhhhhhj
        ......Shhhj.
        ......Shhj..
        .......hj...
        """),
    "B": part("""
        ..hh....hh..
        ..hih..hij..
        ...hhhhhhj..
        ..hiihhhhh..
        .hiihhhhhhj.
        .hihhhhhhhj.
        .hhhhhhhhjj.
        .jhhhhhhhjj.
        ..jhhhhhjj..
        ...jjjjjj...
        """),
}
# 双髻：少女头顶两侧各一个髻，额前刘海
HAIRS["twin_buns"] = {
    "F": part("""
        .hih....hih.
        .hhjTT.Thhj.
        ..jhhhhhhj..
        ..hiihhhhh..
        .hiihhhhhhj.
        .ihhhhhhhhj.
        .hh......hj.
        .h........j.
        .h........j.
        """),
    "L": part("""
        ....hih.hih.
        ....hhjThhhj
        .....hhhhhj.
        ....hiihhhj.
        ...hiihhhhhj
        ..hhhhhhhhhj
        ..hhhhhhhhhj
        ..h...Shhhj.
        ......hhhhj.
        .......hhj..
        """),
    "B": part("""
        .hih....hih.
        .hhjT..Thhj.
        ..jhhhhhhj..
        ..hiihhhhh..
        .hiihhhhhhj.
        .hihhhhhhhj.
        .hhhhhhhhjj.
        .jhhhhhhhjj.
        ..jhhhhhjj..
        ...jjhhjj...
        ....jhhj....
        """),
}
# 高髻：妇人。髻高出头顶两行，两鬓抿得齐
HAIRS["high_bun"] = {
    "F": part("""
        ....hiih....
        ...hiihhj...
        ....jhhj....
        ...hhTThh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .ihhh..hhhj.
        .hh......hj.
        .h........j.
        .h........j.
        """, oy=-2),
    "L": part("""
        ......hiih..
        .....hiihhj.
        ......jhhj..
        .....hhTTh..
        ....hiihhhj.
        ...hiihhhhhj
        ..hhhhhhhhhj
        ..hhh.hhhhhj
        ......Shhhhj
        ......Shhhj.
        .......hhj..
        """, oy=-2),
    "B": part("""
        ....hiih....
        ...hiihhj...
        ....jhhj....
        ...hhTThh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .hihhhhhhhj.
        .hhhhhhhhjj.
        .jhhhhhhhjj.
        ..jhhhhhjj..
        ...jhhhhj...
        ....jjjj....
        """, oy=-2),
}
# 垂髻：少女，脑后一个小髻，余发垂到背上
HAIRS["maiden"] = {
    "F": part("""
        ............
        ....hhhh....
        ...hiihhh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .ihhhh.hhhj.
        .hh......hj.
        .h........j.
        .hj......jj.
        .hj......jj.
        """),
    "L": part("""
        ........hh..
        .....hhhhij.
        ....hiihhhj.
        ...hiihhhhj.
        ..hhhhhhhhhj
        ..hhhhhhhhhj
        ..h...Shhhhj
        ......Shhhhj
        .......hhhhj
        .......hhhj.
        ........hhj.
        ........jj..
        """),
    "B": part("""
        ............
        ....hhhh....
        ...hiTThh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .hihhhhhhhj.
        .hhhhhhhhjj.
        .hhhhhhhhjj.
        .jhhhhhhhjj.
        ..jhhhhhjj..
        ..jhhhhhjj..
        ...jhhhhj...
        ....jjjj....
        """),
}
# 农妇：中分，脑后挽个低髻
HAIRS["woman_bun"] = {
    "F": part("""
        ............
        ....hhhh....
        ...hiihhh...
        ..hiih.hhh..
        .hiih...hhj.
        .ih......hj.
        .h........j.
        .h........j.
        """),
    "L": part("""
        ............
        .....hhhh...
        ....hiihhh..
        ...hiihhhhj.
        ..hhhhhhhhhj
        ....hhhhhhhj
        ......Shhhhj
        ......Shhhhhj
        .......hhhhij
        ........hjjj.
        """),
    "B": part("""
        ............
        ....hhhh....
        ...hiihhh...
        ..hiihhhhh..
        .hiihhhhhhj.
        .hihhhhhhhj.
        .hhhhhhhhjj.
        .jhhhiihhjj.
        ..jhhhhhjj..
        ...jhhhjj...
        """),
}


# ===========================================================================
# 头饰
# ===========================================================================

@dataclass(frozen=True)
class Headwear:
    views: dict[str, Part]
    clip: int = 0            # 头发在头框第 clip 行以上的像素被帽子罩住，不画


HEADWEAR: dict[str, Headwear] = {}

# 发带飘尾：髻根的带子多垂下两截，侧面、背面飘在脑后。韩立专用的记号。
HEADWEAR["ribbon"] = Headwear({
    # 正面也要看得见：两截带尾从髻根飘到画面右侧，全员只有他这一处浅色飘带
    "F": part("""
        ............
        ............
        ........UU..
        .........Uu.
        ..........u.
        ..........u.
        """),
    "L": part("""
        ............
        ............
        .........UU.
        ..........Uu
        ...........u
        """),
    "B": part("""
        ............
        ............
        ............
        ....u..u....
        ....U..U....
        ....u...u...
        """),
})
# 头巾：软布裹住头顶，脑后打个小结。村民、脚夫、伙计。
# 贴着头皮画（与头发同宽）：比头宽一圈的话，放大看就是一顶蘑菇帽。
HEADWEAR["scarf"] = Headwear({
    "F": part("""
        ............
        ....ABBB....
        ...ABBBBBC..
        ..ABBBBBBBC.
        .ABBBBBBBBC.
        .DCCCCCCCCD.
        """),
    "L": part("""
        ............
        .....ABBB...
        ....ABBBBBC.
        ...ABBBBBBBC
        ..ABBBBBBBCC
        ..DCCCCCCCDCD
        ...........D.
        """),
    "B": part("""
        ............
        ....ABBB....
        ...ABBBBBC..
        ..ABBBBBBBC.
        .ABBBBBBBBC.
        .DCCCBCCCCD.
        .....CD.....
        """),
}, clip=5)
# 马贼头巾：勒在额上，结打在一侧，两根带头垂着
HEADWEAR["bandana"] = Headwear({
    "F": part("""
        ............
        ....ABBB....
        ...ABBBBBC..
        ..ABBBBBBBC.
        .DCCCCCCCCDC
        ..........DCD
        ...........D.
        """),
    "L": part("""
        ............
        .....ABBB...
        ....ABBBBBC.
        ...ABBBBBBCC
        ..DCCCCCCCDCD
        ...........CD
        ............D
        """),
    "B": part("""
        ............
        ....ABBB....
        ...ABBBBBC..
        ..ABBBBBBBC.
        .DCCCCBCCCCD
        .....CD.....
        ....CD.D....
        ....D...D...
        """),
}, clip=4)
# 斗笠：竹编的尖顶大笠，比头宽出一圈。船家、行路人、曲魂。
HEADWEAR["bamboo_hat"] = Headwear({
    "F": part("""
        ......AB......
        .....ABBC.....
        ....ABBBCC....
        ...ABABBCBC...
        ..ABBBBBBCCC..
        DCCCCCCCCCCCCD
        .DDDDDDDDDDDD.
        """, ox=-1, oy=0),
    "L": part("""
        .......AB.....
        ......ABBC....
        .....ABBBCC...
        ....ABABBCBC..
        ...ABBBBBBCCC.
        .DCCCCCCCCCCCD
        ..DDDDDDDDDDD.
        """, ox=-2, oy=0),
    "B": part("""
        ......AB......
        .....ABBC.....
        ....ABBBCC....
        ...ABABBCBC...
        ..ABBBBBBCCC..
        DCCCCCCCCCCCCD
        .DDDDDDDDDDDD.
        """, ox=-1, oy=0),
}, clip=6)
# 草帽：圆顶宽檐，药圃管事
HEADWEAR["straw_hat"] = Headwear({
    "F": part("""
        ..............
        .....ABBC.....
        ....ABBBCC....
        ....TTTTTt....
        .ABBBBBBBBBCC.
        DCCCCCCCCCCCCD
        """, ox=-1, oy=0),
    "L": part("""
        ..............
        ......ABBC....
        .....ABBBCC...
        .....TTTTTt...
        ..ABBBBBBBBBC.
        .DCCCCCCCCCCCD
        """, ox=-2, oy=0),
    "B": part("""
        ..............
        .....ABBC.....
        ....ABBBCC....
        ....TTTTTt....
        .ABBBBBBBBBCC.
        DCCCCCCCCCCCCD
        """, ox=-1, oy=0),
}, clip=5)
# 帽兜：整块布从头顶罩下来，只露脸
HEADWEAR["hood"] = Headwear({
    "F": part("""
        ....ABBB....
        ...ABBBBC...
        ..ABBBBBBC..
        .ABBCCCCBCC.
        .ABC....CCD.
        .BB......CD.
        .BC......CD.
        .BC......DD.
        .CD......DD.
        ..D......D..
        """),
    "L": part("""
        .....ABBB...
        ....ABBBBCC.
        ...ABBBBBBCC
        ..ABBBBBBBCC
        ..BCC.BBBBCD
        ......BBBBCD
        ......BBBBCD
        ......BBBCCD
        .......BCCD.
        .......CDD..
        """),
    "B": part("""
        ....ABBB....
        ...ABBBBC...
        ..ABBBBBBC..
        .ABBBBBBBCC.
        .ABBBBBBBCD.
        .BBBBBBBBCD.
        .BBBBBBBCCD.
        .BBBBBBBCCD.
        .CBBBBBCCDD.
        ..DCCCCCDD..
        """),
}, clip=10)
# 低兜帽：斗篷连着的帽兜压得很低，帽檐的影子盖住眉眼，只露出下巴。
# 曲魂：「帽兜压得很低」（ch03.mogui.giant、ch03.miji.mingdao_2），第 4 章「斗篷底下那位」。
# 码 q 是帽兜底下的一团黑（墨色，不是皮肤暗面：死人皮的暗面是灰绿，压不住），
# w 是黑影里透出来的一点下巴（皮肤暗面）——「帽兜底下是什么」（ch03.quhun.gate）本来就不该看清。
HEADWEAR["cowl"] = Headwear({
    "F": part("""
        ....ABBB....
        ...ABBBBC...
        ..ABBBBBBC..
        .ABBBBBBBBC.
        .ABBBBBBBBC.
        .ABBBBBBBCD.
        .ABCCCCCCCD.
        .BCqqqqqqDD.
        .BCqqqqqqCD.
        .CDqwwwwqDD.
        .CD......DD.
        """),
    "L": part("""
        .....ABBB...
        ....ABBBBC..
        ...ABBBBBBC.
        ..ABBBBBBBCC
        .ABBBBBBBBCC
        .ABBBBBBBBCD
        .BCCCCBBBBCD
        ..qqqCBBBBCD
        ..qqqCBBBCDD
        ...wwCBBBCD.
        .....CBBCDD.
        """),
    "B": part("""
        ....ABBB....
        ...ABBBBC...
        ..ABBBBBBC..
        .ABBBBBBBBC.
        .ABBBBBBBBC.
        .ABBBBBBBCD.
        .ABBBBBBBCD.
        .BBBBBBBBCD.
        .BBBBBBBBCD.
        .CBBBBBBCCD.
        .CCBBBBCCDD.
        """),
}, clip=10)
# 簪：插在髻上的一根金簪 / 玉簪
HEADWEAR["hairpin"] = Headwear({
    "F": part("""
        ............
        ..ZZ........
        ...Z........
        """, oy=-2),
    "L": part("""
        ............
        ..........ZZ
        ..........Z.
        """, oy=-2),
    "B": part("""
        ............
        ........ZZ..
        ........Z...
        """, oy=-2),
})
# 小冠：束在髻上的一顶小冠。门主、长者、有身份的人
HEADWEAR["crown"] = Headwear({
    "F": part("""
        ....ABBC....
        ....BZZC....
        ...DDDDDD...
        """),
    "L": part("""
        .....ABBC...
        .....BZZC...
        ....DDDDDD..
        """),
    "B": part("""
        ....ABBC....
        ....BBBC....
        ...DDDDDD...
        """),
})
# 方巾：读书人、掌柜。软布方帽，脑后垂两片巾脚
HEADWEAR["scholar_cap"] = Headwear({
    "F": part("""
        ............
        ...ABBBBC...
        ...ABBBBC...
        ..ABBBBBBC..
        .ABBBBBBBCC.
        .DCCCCCCCCD.
        """),
    "L": part("""
        ............
        ....ABBBBC..
        ....ABBBBCC.
        ...ABBBBBBC.
        ..ABBBBBBBCC
        ..DCCCCCCCCD
        ..........CD
        ..........DD
        """),
    "B": part("""
        ............
        ...ABBBBC...
        ...ABBBBC...
        ..ABBBBBBC..
        .ABBBBBBBCC.
        .DCCCCCCCCD.
        ...CD..CD...
        ...D....D...
        """),
}, clip=5)
# 头花：鬓边一朵花（潇湘院）
HEADWEAR["flower"] = Headwear({
    "F": part("""
        ............
        ............
        .rr.........
        rZrr........
        .rr.........
        """, oy=0),
    "L": part("""
        ............
        ............
        .........rr.
        ........rrZr
        .........rr.
        """),
    "B": part("""
        ............
        ............
        .........rr.
        ........rZrr
        .........rr.
        """),
})
# 包头巾：妇人干活时把头发整个包起来
HEADWEAR["kerchief"] = Headwear({
    "F": part("""
        ............
        ....ABBB....
        ...ABBBBC...
        ..ABBBBBBC..
        .ABBBBBBBCC.
        .ABC....CCD.
        .BC......CD.
        """),
    "L": part("""
        ............
        .....ABBB...
        ....ABBBBCC.
        ...ABBBBBBCC
        ..ABBBBBBBCCD
        ..BC..BBBBCDC
        ......BBBCCDD
        .......CCD.D.
        """),
    "B": part("""
        ............
        ....ABBB....
        ...ABBBBC...
        ..ABBBBBBC..
        .ABBBBBBBCC.
        .ABBBBBBBCD.
        .BBBBCBBBCD.
        ..CBCDCBCD..
        .....CD.....
        """),
}, clip=10)


# ===========================================================================
# 胡须
# ===========================================================================

BEARDS: dict[str, dict[str, Part]] = {
    "mustache": {
        "F": part("""
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ...jhhhhj...
            """),
        "L": part("""
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ..jhhh......
            """),
    },
    "goatee": {
        "F": part("""
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ...jh..hj...
            .....ih.....
            .....hj.....
            """),
        "L": part("""
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ..jhh.......
            ...ihh......
            ...hj.......
            """),
    },
    # 络腮胡：从鬓角一路连到下巴
    "full": {
        "F": part("""
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ..j......j..
            ..hjhhhhjh..
            ...hhiihh...
            ....hhhj....
            """),
        "L": part("""
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ......hj....
            ..jhhhhj....
            ...hhihh....
            ....hhj.....
            """),
    },
    # 长须：垂到胸前，长者
    "long": {
        "F": part("""
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ...jhhhhj...
            ....hiih....
            ....hiih....
            .....hh.....
            .....hj.....
            """),
        "L": part("""
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ............
            ..jhhh......
            ..hiih......
            ..hih.......
            ...hj.......
            ...j........
            """),
    },
}
