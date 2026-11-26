// ============================================================
//  StageData.cpp : ステージとアリーナの中身
//  ・ステージ1～3：1マスずつ手で配置
//  ・ステージ4～9：「区間（セグメント）」を左からつなげて作る
//    区間はどれも「地面の高さ G」から始まり、次の区間へ G を引き継ぐ
// ============================================================
#include "Stage.h"

namespace {
    Vec2 Center(int tx, int ty) { return Vec2(tx * TILE + TILE / 2, ty * TILE + TILE / 2); }
    Vec2 Bottom(int tx, int ty) { return Vec2(tx * TILE + TILE / 2, (ty + 1) * TILE); }
}

const StageInfo& Stage::Info(int index) {
    static const StageInfo infos[9] = {
        { L"夕焼けの摩天楼", L"SUNSET SKYLINE", 0, 75.0f },
        { L"ネオン・ナイト", L"NEON NIGHT", 1, 95.0f },
        { L"嵐の塔", L"STORM SPIRE", 2, 110.0f },
        { L"夜明けのハーバー", L"DAWN HARBOR", 3, 110.0f },
        { L"雪のビル街", L"SNOWY BLOCKS", 4, 125.0f },
        { L"サイバー・ハイウェイ", L"CYBER HIGHWAY", 5, 125.0f },
        { L"黄昏の鉄橋", L"TWILIGHT BRIDGE", 0, 140.0f },
        { L"雷鳴の摩天楼", L"THUNDER TOWERS", 2, 150.0f },
        { L"オーロラの頂", L"AURORA SUMMIT", 6, 185.0f },
    };
    return infos[index < 0 ? 0 : (index > 8 ? 8 : index)];
}

const StageInfo& Stage::ArenaInfo(int index) {
    static const StageInfo infos[3] = {
        { L"屋上リング", L"ROOFTOP RING", 0, 0 },
        { L"ネオン・ケージ", L"NEON CAGE", 1, 0 },
        { L"嵐の鉄塔", L"STORM PYLON", 2, 0 },
    };
    return infos[index < 0 ? 0 : (index > 2 ? 2 : index)];
}

void Stage::Clear(int w, int h) {
    saws.clear(); rings.clear(); crystals.clear(); checkpoints.clear(); hints.clear(); movers.clear(); lasers.clear();
    w_ = w; h_ = h;
    tiles_.assign(w_ * h_, ' ');
    crumble_.assign(w_ * h_, 0.0f);
}

// ============================================================
//  1人用ステージ
// ============================================================
void Stage::Load(int idx) {
    index = idx;
    arena_ = false;
    theme_ = Info(idx).theme;
    if (idx >= 3) { BuildSegments(idx); Build3D(); return; }

    const int widths[3] = { 110, 130, 150 };
    Clear(widths[idx], 20);
    auto Box = [&](int x0, int y0, int x1, int y1, char c) {
        for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x)
            if (x >= 0 && x < w_ && y >= 0 && y < h_) tiles_[y * w_ + x] = c;
    };
    auto AddRing = [&](int tx, int ty) { rings.push_back({ Center(tx, ty) }); };
    auto AddCrystal = [&](int tx, int ty) { crystals.push_back({ Center(tx, ty), false, RandRange(0, 6.28f) }); };
    auto AddCheck = [&](int tx, int ty) { checkpoints.push_back({ Bottom(tx, ty) }); };
    auto AddSaw = [&](int tx, int ty, bool horizontal, float ampTiles, float speed) {
        Saw s; s.center = Center(tx, ty); s.axis = horizontal ? Vec2(1, 0) : Vec2(0, 1);
        s.amp = ampTiles * TILE; s.speed = speed; s.phase = RandRange(0, 6.28f); s.pos = s.center;
        saws.push_back(s);
    };
    auto AddHint = [&](int tx, int ty, const wchar_t* kb, const wchar_t* pad) { hints.push_back({ Center(tx, ty), kb, pad }); };
    auto Start = [&](int tx, int ty) { start = Vec2(tx * TILE + TILE / 2, (ty + 1) * TILE - 20.5f); };
    auto Goal = [&](int tx, int ty) { goal = Bottom(tx, ty); };

    if (idx == 0) {
        // ===================== STAGE 1 : 夕焼けの摩天楼（チュートリアル） =====================
        Box(0, 16, 16, 19, '#'); Start(3, 15);
        AddHint(5, 12, L"A / D で移動　　SPACE でジャンプ", L"左スティックで移動　　A でジャンプ");
        Box(9, 14, 10, 15, '#');
        Box(18, 7, 22, 8, '#');
        AddHint(14, 11, L"マウスで狙って 左クリック長押し でワイヤー！　離すと切り離し", L"右スティックで狙って RT 長押し でワイヤー！　離すと切り離し");
        AddCrystal(20, 12);
        Box(23, 16, 34, 19, '#'); AddCheck(26, 15);
        Box(31, 15, 32, 15, '^'); AddCrystal(31, 12);
        AddHint(33, 11, L"振り子の勢いが乗ったところで離すと、遠くまで飛べる", L"振り子の勢いが乗ったところで離すと、遠くまで飛べる");
        Box(37, 6, 39, 7, '#'); Box(44, 5, 46, 6, '#'); Box(51, 6, 53, 7, '#');
        AddCrystal(41, 10); AddCrystal(48, 9); AddCrystal(55, 10);
        Box(59, 16, 65, 19, '#'); AddCheck(60, 15);
        Box(61, 5, 66, 6, '#');
        AddHint(62, 12, L"W / S でワイヤーを巻き取る・伸ばす", L"左スティック 上下 でワイヤーを巻き取る・伸ばす");
        Box(66, 9, 76, 19, '#'); AddCheck(68, 8);
        AddHint(72, 6, L"光るリングは狙いが吸いつく。空中でもつかまれる", L"光るリングは狙いが吸いつく。空中でもつかまれる");
        AddRing(80, 5); AddRing(86, 4); AddRing(92, 5);
        AddCrystal(83, 8); AddCrystal(89, 8);
        Box(96, 12, 109, 19, '#');
        Box(100, 4, 102, 5, '#'); AddCrystal(101, 8);
        AddHint(98, 9, L"右クリック でジップ！ 刺さった所へ一気に飛ぶ", L"LT（または X）でジップ！ 刺さった所へ一気に飛ぶ");
        Goal(106, 11);
    } else if (idx == 1) {
        // ===================== STAGE 2 : ネオン・ナイト =====================
        Box(0, 16, 12, 19, '#'); Start(2, 15);
        AddHint(5, 12, L"灰色の金属ブロックにはワイヤーが刺さらない", L"灰色の金属ブロックにはワイヤーが刺さらない");
        Box(13, 6, 28, 7, 'X'); Box(17, 6, 18, 7, '#'); Box(23, 6, 24, 7, '#');
        Box(13, 19, 28, 19, '^');
        AddCrystal(20, 11);
        Box(29, 16, 48, 19, '#'); AddCheck(31, 15);
        AddSaw(36, 15, true, 2.0f, 1.6f);
        AddHint(34, 11, L"ノコギリに触れるとアウト！", L"ノコギリに触れるとアウト！");
        Box(41, 3, 42, 12, 'X'); Box(45, 3, 48, 15, 'X');
        AddHint(39, 9, L"壁に向かってジャンプ → 反対へジャンプで 壁ジャンプ", L"壁に向かってジャンプ → 反対へジャンプで 壁ジャンプ");
        AddCrystal(43, 6);
        Box(49, 3, 58, 4, 'X'); AddCheck(53, 2);
        AddRing(63, 3); AddRing(69, 4); AddRing(75, 3); AddRing(81, 4);
        AddSaw(72, 10, false, 3.0f, 1.3f);
        AddCrystal(66, 8); AddCrystal(78, 9);
        Box(59, 19, 84, 19, '^');
        Box(85, 8, 94, 19, '#'); AddCheck(87, 7);
        Box(97, 1, 99, 2, '#'); Box(100, 1, 106, 2, 'X'); Box(107, 1, 109, 2, '#'); Box(115, 1, 117, 2, '#');
        AddSaw(103, 9, true, 2.0f, 1.8f); AddSaw(112, 8, false, 2.5f, 1.5f);
        AddCrystal(104, 6); AddCrystal(113, 4);
        Box(95, 19, 119, 19, '^');
        Box(120, 10, 129, 19, '#');
        Goal(126, 9);
    } else {
        // ===================== STAGE 3 : 嵐の塔 =====================
        Box(0, 16, 10, 19, '#'); Start(2, 15);
        AddRing(14, 9); AddRing(20, 7); AddRing(26, 9); AddRing(32, 7);
        AddSaw(23, 13, true, 2.0f, 1.7f);
        AddCrystal(23, 10);
        Box(11, 19, 35, 19, '^');
        Box(36, 12, 44, 19, '#'); AddCheck(38, 11);
        Box(43, 1, 46, 2, '#');
        Box(45, 6, 47, 19, 'X');
        AddCrystal(44, 5);
        Box(48, 16, 75, 19, '#'); Box(48, 15, 75, 15, '^');
        Box(50, 3, 75, 4, '#'); Box(54, 3, 56, 4, 'X'); Box(64, 3, 66, 4, 'X');
        AddSaw(56, 10, true, 2.0f, 1.9f); AddSaw(66, 9, true, 2.0f, 2.1f);
        AddCrystal(60, 12); AddCrystal(70, 8);
        Box(76, 10, 84, 19, '#'); AddCheck(78, 9);
        AddRing(88, 4); AddRing(94, 3); AddRing(100, 4);
        AddSaw(91, 8, false, 2.5f, 1.6f); AddSaw(97, 8, false, 2.5f, 1.9f);
        AddCrystal(94, 8);
        Box(85, 19, 102, 19, '^');
        Box(103, 8, 110, 19, '#'); AddCheck(105, 7);
        Box(114, 0, 115, 1, '#'); Box(124, 0, 125, 1, '#'); Box(134, 0, 135, 1, '#');
        AddSaw(119, 9, true, 2.0f, 1.5f); AddSaw(129, 9, true, 2.0f, 1.8f);
        AddCrystal(119, 5); AddCrystal(129, 5);
        Box(111, 19, 138, 19, '^');
        Box(139, 7, 149, 19, '#');
        Goal(146, 6);
    }
    Build3D();
}

// ============================================================
//  ステージ4～9：区間をつなげて作る
// ============================================================
void Stage::BuildSegments(int idx) {
    Clear(420, 20);
    int x = 0;        // 次の区間を置く左端（タイル）
    int G = 16;       // 今の地面の高さ（この段から下が地面）

    auto Box = [&](int x0, int y0, int x1, int y1, char c) {
        for (int y = y0; y <= y1; ++y) for (int xx = x0; xx <= x1; ++xx)
            if (xx >= 0 && xx < w_ && y >= 0 && y < h_) tiles_[y * w_ + xx] = c;
    };
    auto Crystal_ = [&](int tx, int ty) { crystals.push_back({ Center(tx, ty), false, RandRange(0, 6.28f) }); };
    auto Saw_ = [&](int tx, int ty, bool horizontal, float ampTiles, float speed) {
        Saw s; s.center = Center(tx, ty); s.axis = horizontal ? Vec2(1, 0) : Vec2(0, 1);
        s.amp = ampTiles * TILE; s.speed = speed; s.phase = RandRange(0, 6.28f); s.pos = s.center;
        saws.push_back(s);
    };
    auto Hint_ = [&](int tx, int ty, const wchar_t* kb, const wchar_t* pad) { hints.push_back({ Center(tx, ty), kb, pad ? pad : kb }); };
    auto Ceil = [&]() { return (std::max)(1, G - 9); };   // 天井ブロックの下段

    // 平地（check=true でチェックポイント）
    auto Flat = [&](int len, bool check = false) {
        Box(x, G, x + len - 1, 19, '#');
        if (check) checkpoints.push_back({ Bottom(x + 1, G - 1) });
        x += len;
    };
    // 天井ブロックを振り子で渡る
    auto CeilSwing = [&](int n, bool metal = false, bool saw = false) {
        const int sp = 7;
        int W = 2 + n * sp;
        int c = Ceil();
        for (int i = 0; i < n; ++i) {
            int bx = x + 2 + i * sp;
            Box(bx, c - 1, bx + 2, c, '#');
            if (metal && i < n - 1) Box(bx + 3, c - 1, bx + sp - 1, c, 'X');
            if (saw && i < n - 1) Saw_(bx + 5, c + 7, false, 1.5f, 1.5f);
            Crystal_(bx + 1, c + 5);
        }
        Box(x, 19, x + W - 1, 19, '^');
        x += W;
    };
    // リングの連続
    auto RingChain = [&](int n, bool saw = false) {
        const int sp = 6;
        int W = 3 + n * sp;
        int r = (std::max)(1, G - 8);
        for (int i = 0; i < n; ++i) {
            rings.push_back({ Center(x + 3 + i * sp, r + (i % 2)) });
            if (saw && i < n - 1) Saw_(x + 6 + i * sp, r + 7, false, 2.0f, 1.4f);
        }
        Crystal_(x + 3 + (n / 2) * sp - 3, r + 5);
        Box(x, 19, x + W - 1, 19, '^');
        x += W;
    };
    // 横に動く足場で渡る
    auto MoverCross = [&](int pitW) {
        Mover m;
        m.w = 3 * TILE; m.h = 24;
        m.base = Vec2((x + pitW / 2.0f) * TILE - m.w / 2, G * TILE);
        m.axis = Vec2(1, 0);
        m.amp = (pitW / 2.0f - 2.0f) * TILE;
        m.speed = 0.9f; m.phase = -PI / 2;     // 左端から動き始める
        m.pos = m.base + m.axis * (std::sin(m.phase) * m.amp);
        movers.push_back(m);
        int mid = x + pitW / 2, c = Ceil();
        Box(mid - 1, c - 1, mid + 1, c, '#');     // 落ちそうなとき用の天井
        Crystal_(mid, G - 3);
        Box(x, 19, x + pitW - 1, 19, '^');
        x += pitW;
    };
    // 縦に動く足場で上の段へ
    auto Lift = [&](int up) {
        Mover m;
        m.w = 3 * TILE; m.h = 24;
        m.base = Vec2((x + 1) * TILE, (G - up / 2.0f) * TILE);
        m.axis = Vec2(0, 1);
        m.amp = up / 2.0f * TILE;
        m.speed = 0.85f; m.phase = PI / 2;      // 下端から
        m.pos = m.base + m.axis * (std::sin(m.phase) * m.amp);
        movers.push_back(m);
        Box(x, 19, x + 4, 19, '^');
        Crystal_(x + 2, G - up - 2);
        x += 5;
        G -= up;
    };
    // 崩れる床の橋
    auto Crumble = [&](int len) {
        Box(x, G, x + len - 1, G, 'c');
        int c = Ceil();
        Box(x + len / 2 - 1, c - 1, x + len / 2 + 1, c, '#');
        Crystal_(x + len / 2, G - 2);
        Box(x, 19, x + len - 1, 19, '^');
        x += len;
    };
    // ジャンプ台で高い段へ
    auto Bounce = [&](int up) {
        Box(x, G, x + 4, 19, '#');
        Box(x + 2, G, x + 2, G, 'b');
        Crystal_(x + 2, G - 5);
        x += 5;
        G -= up;
    };
    // 段を下りる
    auto Drop = [&](int d) { G = (std::min)(16, G + d); };
    // レーザーの通路（低い金属の天井つき）
    auto LaserRun = [&](int len) {
        Box(x, G, x + len - 1, 19, '#');
        Box(x, 0, x + len - 1, G - 6, '#');        // 上はビル（通路の上を通って抜けられないように）
        Box(x, G - 5, x + len - 1, G - 4, 'X');
        int k = 0;
        for (int lx = x + 3; lx < x + len - 2; lx += 4, ++k) {
            Laser L;
            L.a = Vec2(lx * TILE + TILE / 2, (G - 3) * TILE);
            L.b = Vec2(lx * TILE + TILE / 2, G * TILE);
            L.period = 2.4f; L.onRatio = 0.5f; L.phase = k * 0.22f;
            lasers.push_back(L);
        }
        Crystal_(x + len / 2, G - 1);
        x += len;
    };
    // 壁ジャンプの縦穴で上の段へ
    auto WallClimb = [&](int up) {
        Box(x, G, x + 3, 19, '#');
        Box(x, G - up, x + 1, G - 4, 'X');
        Crystal_(x + 2, G - up + 1);
        x += 4;
        G -= up;
    };
    // ノコギリが往復する平地（上に逃げ用の天井）
    auto SawRun = [&](int len) {
        Box(x, G, x + len - 1, 19, '#');
        int c = Ceil();
        for (int sx = x + 5; sx < x + len - 3; sx += 6) {
            Saw_(sx, G - 1, true, 2.0f, 1.8f);
            Box(sx - 1, c - 1, sx + 1, c, '#');
            Crystal_(sx, G - 4);
        }
        x += len;
    };
    // 金属の天井に、刺さる島がところどころ
    auto MetalCeiling = [&](int len) {
        int c = Ceil();
        Box(x, c - 1, x + len - 1, c, 'X');
        for (int ix = x + 3; ix < x + len - 2; ix += 6) {
            Box(ix, c - 1, ix + 1, c, '#');
            if (ix + 6 < x + len - 2) Saw_(ix + 4, c + 6, true, 1.2f, 1.6f);
        }
        Crystal_(x + len / 2, c + 4);
        Box(x, 19, x + len - 1, 19, '^');
        x += len;
    };
    auto StartHere = [&]() { start = Vec2((x + 2) * TILE + TILE / 2, G * TILE - 20.5f); };
    auto GoalFlat = [&]() { Box(x, G, x + 11, 19, '#'); goal = Bottom(x + 8, G - 1); x += 12; };

    switch (idx) {
    case 3:   // 夜明けのハーバー：動く足場
        StartHere(); Flat(10);
        Hint_(x - 4, G - 4, L"動く足場に乗って渡ろう。足場にもワイヤーが刺さる", nullptr);
        MoverCross(12); Flat(6, true);
        CeilSwing(2); Flat(5);
        Hint_(x - 3, G - 4, L"上下する足場で上の段へ", nullptr);
        Lift(5); Flat(6, true);
        MoverCross(14); RingChain(3); Flat(6, true);
        SawRun(14); MoverCross(12);
        CeilSwing(3, true); GoalFlat();
        break;
    case 4:   // 雪のビル街：崩れる床・ジャンプ台
        StartHere(); Flat(10);
        Hint_(x - 4, G - 4, L"ひびの入った床は乗ると崩れる。止まらずに走り抜けよう", nullptr);
        Crumble(8); Flat(6, true);
        Hint_(x + 1, G - 4, L"黄色いジャンプ台に乗ると大ジャンプ！", nullptr);
        Bounce(6); Flat(6);
        CeilSwing(2); Flat(4);
        Drop(6); Flat(5, true);
        Crumble(10); RingChain(3); Flat(5, true);
        WallClimb(6); Flat(5);
        Crumble(8); Flat(4, true);
        RingChain(2); Drop(6); Flat(4);
        Bounce(5); GoalFlat();
        break;
    case 5:   // サイバー・ハイウェイ：レーザー
        StartHere(); Flat(10);
        Hint_(x - 4, G - 3, L"赤いレーザーは点いたり消えたりする。消えた瞬間に駆け抜けろ", nullptr);
        LaserRun(14); Flat(4, true);
        CeilSwing(3, true); Flat(4);
        LaserRun(16); MoverCross(12); Flat(4, true);
        SawRun(16); RingChain(4, true); Flat(5, true);
        LaserRun(18); CeilSwing(2); GoalFlat();
        break;
    case 6:   // 黄昏の鉄橋：いろいろ
        StartHere(); Flat(8);
        MetalCeiling(20); Flat(4, true);
        Lift(5); Flat(4);
        MoverCross(12); Flat(4, true);
        Crumble(10); CeilSwing(2);
        Drop(5); Flat(4, true);
        WallClimb(7); Flat(4);
        RingChain(3); Drop(7); Flat(4, true);
        SawRun(14); Bounce(6); GoalFlat();
        break;
    case 7:   // 雷鳴の摩天楼：高難度
        StartHere(); Flat(8);
        CeilSwing(4, true, true); Flat(4, true);
        LaserRun(16); MoverCross(14); RingChain(4, true); Flat(4, true);
        Crumble(12); WallClimb(7); Flat(3);
        CeilSwing(3); Drop(7); Flat(4, true);
        MetalCeiling(24); LaserRun(14); GoalFlat();
        break;
    default:  // オーロラの頂：最終ステージ
        StartHere(); Flat(8);
        RingChain(4, true); Flat(3, true);
        MoverCross(14); Flat(3); Lift(6); Flat(4);
        Crumble(10); CeilSwing(3, true); Flat(4, true);
        Drop(6); Flat(3);
        LaserRun(16); SawRun(14); Flat(3, true);
        WallClimb(7); Flat(3);
        RingChain(3); Drop(7); Flat(3, true);
        MoverCross(14); Crumble(10); MetalCeiling(20); Flat(3, true);
        CeilSwing(4, true, true); GoalFlat();
        break;
    }

    // 使った幅に切り詰める
    int newW = x;
    std::vector<char> t(newW * h_, ' ');
    for (int y = 0; y < h_; ++y) for (int xx = 0; xx < newW; ++xx) t[y * newW + xx] = tiles_[y * w_ + xx];
    tiles_ = t;
    w_ = newW;
    crumble_.assign(w_ * h_, 0.0f);
}

// ============================================================
//  対戦アリーナ（27×15タイル＝ほぼ1画面）
// ============================================================
void Stage::LoadArena(int idx) {
    index = idx;
    arena_ = true;
    theme_ = ArenaInfo(idx).theme;
    Clear(27, 15);
    auto Box = [&](int x0, int y0, int x1, int y1, char c) {
        for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x)
            if (x >= 0 && x < w_ && y >= 0 && y < h_) tiles_[y * w_ + x] = c;
    };
    auto Spawn = [&](int i, int tx, int ty) { spawn[i] = Vec2(tx * TILE + TILE / 2, (ty + 1) * TILE - 20.5f); };
    auto Saw_ = [&](int tx, int ty, bool horizontal, float ampTiles, float speed) {
        Saw s; s.center = Center(tx, ty); s.axis = horizontal ? Vec2(1, 0) : Vec2(0, 1);
        s.amp = ampTiles * TILE; s.speed = speed; s.phase = 0; s.pos = s.center;
        saws.push_back(s);
    };

    if (idx == 0) {
        // 屋上リング：中央にトゲの穴、浮き島とリング
        Box(0, 11, 7, 14, '#'); Box(19, 11, 26, 14, '#');
        Box(8, 14, 18, 14, '^');
        Box(11, 8, 15, 8, '#');
        Box(4, 2, 6, 3, '#'); Box(20, 2, 22, 3, '#');
        rings.push_back({ Center(13, 3) });
        Saw_(13, 12, true, 3.0f, 1.2f);
        Spawn(0, 3, 10); Spawn(1, 23, 10);
    } else if (idx == 1) {
        // ネオン・ケージ：中央の穴の上を動く足場、金属の柱
        Box(0, 13, 26, 14, '#'); Box(10, 13, 16, 14, ' '); Box(10, 14, 16, 14, '^');
        Box(5, 6, 6, 10, 'X'); Box(20, 6, 21, 10, 'X');
        Box(9, 2, 17, 3, '#');
        rings.push_back({ Center(3, 4) }); rings.push_back({ Center(23, 4) });
        Mover m; m.w = 3 * TILE; m.h = 24; m.base = Vec2(13.5f * TILE - m.w / 2, 9 * TILE);
        m.axis = Vec2(1, 0); m.amp = 5 * TILE; m.speed = 0.8f; m.phase = 0; m.pos = m.base;
        movers.push_back(m);
        Spawn(0, 2, 12); Spawn(1, 24, 12);
    } else {
        // 嵐の鉄塔：崩れる橋、レーザー、ジャンプ台
        Box(0, 12, 5, 14, '#'); Box(21, 12, 26, 14, '#');
        Box(6, 12, 11, 12, 'c'); Box(15, 12, 20, 12, 'c');
        Box(12, 9, 14, 14, '#');
        Box(6, 14, 11, 14, '^'); Box(15, 14, 20, 14, '^');
        Box(2, 12, 2, 12, 'b'); Box(24, 12, 24, 12, 'b');
        Box(4, 2, 6, 3, '#'); Box(20, 2, 22, 3, '#');
        rings.push_back({ Center(13, 3) });
        for (int i = 0; i < 2; ++i) {
            Laser L;
            float lx = (i == 0 ? 9 : 17) * TILE + TILE / 2;
            L.a = Vec2(lx, 4 * TILE); L.b = Vec2(lx, 12 * TILE);
            L.period = 3.0f; L.onRatio = 0.4f; L.phase = i * 0.5f;
            lasers.push_back(L);
        }
        Spawn(0, 3, 11); Spawn(1, 23, 11);
    }
    start = spawn[0];
    goal = Vec2(-9999, -9999);   // アリーナにゴールはない
    Build3D();
}
