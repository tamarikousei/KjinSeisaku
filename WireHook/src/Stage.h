#pragma once
// ============================================================
//  Stage.h : タイルマップ・ギミック・レイキャスト
//
//  タイルの種類（文字）
//   ' ' 空間
//   '#' 足場（ワイヤーが刺さる）
//   'X' 金属（ワイヤーが刺さらない）
//   '^' トゲ
//   'c' 崩れる床（乗ると少しして崩れ、しばらくすると戻る）
//   'b' ジャンプ台（乗ると大きく跳ね上がる）
// ============================================================
#include "Common.h"
#include "Render3D.h"

enum class HitKind { None, Solid, Metal, Ring, Mover, Player };

struct RayHit {
    HitKind kind = HitKind::None;
    Vec2 point;        // 当たった位置
    float dist = 0;
    int index = -1;    // リング／動く足場の番号
    int tx = 0, ty = 0;// 刺さったタイル
};

struct Saw { Vec2 center, axis, pos; float amp = 0, speed = 1, phase = 0, radius = 26, rot = 0; };
struct Ring { Vec2 pos; float flash = 0; };
struct Crystal { Vec2 pos; bool got = false; float t = 0; };
struct Checkpoint { Vec2 pos; bool active = false; float anim = 0; };
struct Hint { Vec2 pos; std::wstring kb, pad; float alpha = 0; };

// 動く足場（ワイヤーも刺さる）
struct Mover {
    Vec2 base;         // 往復の中心（左上）
    Vec2 axis;         // 動く向き
    float amp = 0, speed = 1, phase = 0;
    float w = 144, h = 24;
    Vec2 pos, delta;   // 現在の左上と、このフレームの移動量
};

// レーザー（一定間隔でON/OFF）
struct Laser {
    Vec2 a, b;         // 発射口と受け口（縦か横）
    float period = 3, onRatio = 0.5f, phase = 0;
    bool on = false;
    float warn = 0;    // 点灯直前の予告（0～1）
    bool alwaysOn = false;
};

struct StageInfo {
    std::wstring name;     // 日本語名
    std::wstring sub;      // 英語名
    int theme;
    float parTime;         // ランクSの目安タイム（秒）
};

class Stage {
public:
    static int Count() { return 9; }        // 1人用ステージ数
    static int ArenaCount() { return 3; }   // 対戦アリーナ数
    static const StageInfo& Info(int index);
    static const StageInfo& ArenaInfo(int index);

    void Load(int index);          // 1人用ステージ
    void LoadArena(int index);     // 対戦アリーナ
    void Update(float dt);
    void Draw(const Vec2& cam, float time);
    void DrawGlow(const Vec2& cam, float time);
    void DrawHints(const Vec2& cam, const Vec2& playerPos, float dt);

    // タイル問い合わせ
    char At(int tx, int ty) const;
    bool IsSolid(int tx, int ty) const;
    bool IsHookTile(int tx, int ty) const;
    bool OverlapSolid(float l, float t, float r, float b) const;       // タイル＋動く足場
    bool OverlapTiles(float l, float t, float r, float b) const;      // タイルだけ
    int  OverlapMover(float l, float t, float r, float b) const;      // 重なった動く足場の番号（-1なし）
    bool OverlapSpike(float l, float t, float r, float b) const;
    bool OverlapLaser(float l, float t, float r, float b) const;
    void TouchCrumble(int tx, int ty);                                // 崩れる床に乗った
    bool IsBounce(int tx, int ty) const { return At(tx, ty) == 'b'; }

    // from から dir 方向へ maxLen までワイヤーを飛ばしたときの当たり
    RayHit Raycast(const Vec2& from, const Vec2& dir, float maxLen) const;
    int FindAssistRing(const Vec2& from, const Vec2& dir, float maxLen, float maxAngle) const;

    float WorldW() const { return w_ * TILE; }
    float WorldH() const { return h_ * TILE; }
    int theme() const { return theme_; }
    bool IsArena() const { return arena_; }

    int index = 0;
    Vec2 start, goal;
    Vec2 spawn[2];                  // 対戦の出現位置
    std::vector<Saw> saws;
    std::vector<Ring> rings;
    std::vector<Crystal> crystals;
    std::vector<Checkpoint> checkpoints;
    std::vector<Hint> hints;
    std::vector<Mover> movers;
    std::vector<Laser> lasers;
    ~Stage();
    // 3D 用：地面の高さ（x の真下で一番近い足場の上面。なければ -1）
    float GroundBelow(float x, float y, float maxDist) const;
private:
    void Clear(int w, int h);
    void BuildSegments(int idx);
    void DrawTile(int tx, int ty, float sx, float sy, float time);
    // ---- 3D ----
    void Build3D();                  // テクスチャとメッシュを作る（Load の最後に呼ぶ）
    void Release3D();
    void Draw3D(const Vec2& cam, float time);
    void DrawGlow3D(const Vec2& cam, float time);
    static constexpr int kChunk = 16;       // メッシュを何列ごとに分けるか
    std::vector<R3::Mesh> chunks_;          // 足場の面（テクスチャ付き）
    std::vector<R3::Mesh> edges_;           // 光る縁（ライトなしで描く）
    int atlas_ = -1, atlasEmis_ = -1;       // 面の模様／窓明かりだけの模様
    int w_ = 0, h_ = 0;
    int theme_ = 0;
    bool arena_ = false;
    std::vector<char> tiles_;
    std::vector<float> crumble_;    // 崩れる床の状態（0=通常 >0=崩れるまでの経過 <0=復活までの残り）
};
