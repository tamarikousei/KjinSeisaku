#pragma once
// ============================================================
//  Render3D.h : 2.5D 表示のための 3D 描画の土台
//
//  座標の決まり
//   ・ゲームの座標（x 右, y 下, 1タイル48）はそのまま 3D の単位に使う
//   ・3D では Y が上なので、変換は (x, y) → (x, -y, z)
//   ・z = 0 が「ゲームが動く面」。z がマイナスで手前、プラスで奥
//   ・カメラは z = -dist から真っすぐ奥を向く。距離を調整して、
//     z = 0 の面がちょうど 1280×720 に映るようにしてある
//     → 照準・吹き出し・HUD など、2D の座標計算はそのまま使える
// ============================================================
#include "Common.h"

struct Theme;

namespace R3 {
    constexpr float kFovDeg = 50.0f;     // 縦の画角（大きいほど遠近感が強い）
    float CamDist();                     // z=0 の面が高さ720に映るカメラ距離

    inline VECTOR P(float x, float y, float z = 0.0f) { return VGet(x, -y, z); }
    inline COLOR_U8 C8(const ColorF& c, float a = 1.0f) {
        return GetColorU8(int(Clamp01(c.r) * 255), int(Clamp01(c.g) * 255), int(Clamp01(c.b) * 255), int(Clamp01(a) * 255));
    }

    // camDraw：2D と同じ「画面座標 + camDraw = ワールド座標」のカメラ。vy, vh は分割画面の縦範囲（論理座標）
    void ApplyCamera(const Vec2& camDraw, int vy = 0, int vh = SCREEN_H);
    Vec2 ToScreen(float x, float y, float z);   // ワールド → 画面（論理座標）
    float DepthScale(float z);                  // 奥行き z にある物が画面上で何倍に見えるか

    void Begin3D(bool lighting = true);         // Zバッファ・ライト ON
    void End3D();                               // 2D 描画の状態に戻す
    void SetupLights(const Theme& t);           // テーマに合わせた平行光・環境光・霧
    void SetFog(bool on, const ColorF& c, float start, float end);
    void SetPointLight(int slot, bool on, const Vec2& pos, float z, const ColorF& c, float range);   // slot 0～2

    // ---------- メッシュ（静的な形をまとめて描く） ----------
    struct Mesh {
        std::vector<VERTEX3D> v;
        std::vector<unsigned short> idx;
        void Clear() { v.clear(); idx.clear(); }
        bool Full() const { return v.size() > 60000; }
        // 4頂点の面（a→b→c→d の順に外周）
        void Quad(VECTOR a, VECTOR b, VECTOR c, VECTOR d, VECTOR n, COLOR_U8 col, float u0, float v0, float u1, float v1);
        void Tri(VECTOR a, VECTOR b, VECTOR c, COLOR_U8 col, float u, float vv);
        void Draw(int tex, bool trans = false) const;
    };
    // 箱を足す（x0..x1, y0..y1 は2D座標、z0..z1 は奥行き）。faces はビット：1前 2上 4下 8左 16右 32奥
    void AddBox(Mesh& m, float x0, float y0, float x1, float y1, float z0, float z1, COLOR_U8 col, int faces,
                const float* uvFront, const float* uvTop = nullptr, const float* uvSide = nullptr);

    // ---------- その場で描く形 ----------
    void DrawBox(float x0, float y0, float x1, float y1, float z0, float z1, COLOR_U8 col, int tex = -1, const float* uv = nullptr);
    void DrawDiscZ(const Vec2& c, float r, float z0, float z1, int seg, COLOR_U8 side, COLOR_U8 face, float rot = 0, int teeth = 0, float toothLen = 0);
    void DrawTorus(const Vec2& c, float R, float r, int seg, int side, COLOR_U8 col, const MATRIX& rot, float z = 0);
    void DrawOcta(const Vec2& c, float rx, float ry, float rot, COLOR_U8 col, float z = 0);
    void DrawRope(const Vec2& a, const Vec2& b, float slack, float r, unsigned int col);
}
