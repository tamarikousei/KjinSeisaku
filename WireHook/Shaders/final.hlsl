// ============================================================
//  final.hlsl : 最終合成
//   ・高解像度で描いた絵を画面サイズへ縮小（スーパーサンプリング）
//   ・衝撃波のゆがみ／高速時の放射ブラー／色収差
//   ・ネオンの光で周りを照らすライティング
//   ・HDR ブルーム＋光の筋（ゴッドレイ）
//   ・ACES トーンマップ、色調補正、ビネット、フィルムノイズ
// ============================================================
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 dif : COLOR0;
    float4 spc : COLOR1;
    float2 uv  : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};
cbuffer cbPost : register(b4) {
    float4 g_Res;      // x,y = 出力の1ピクセル(UV)  z = 縦横比  w = 時間
    float4 g_Grade;    // 露出, コントラスト, 彩度, ビネット
    float4 g_Tint;     // 色味 rgb, フィルムノイズ
    float4 g_Fx;       // 色収差, ブルーム強さ, ライティング強さ, 環境光
    float4 g_Sun;      // 太陽のUV x,y, 光の筋の強さ, 減衰
    float4 g_Speed;    // 放射ブラーの中心UV x,y, 強さ, フラッシュ量
    float4 g_Flash;    // フラッシュ色 rgb, 発光レイヤーの強さ
    float4 g_Shock[4]; // 衝撃波：中心UV x,y, 半径, 強さ
};
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);
SamplerState s2 : register(s2);
SamplerState s3 : register(s3);
Texture2D tScene : register(t0);
Texture2D tGlow  : register(t1);
Texture2D tBloom : register(t2);
Texture2D tLight : register(t3);

float3 ACES(float3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}
float Hash(float2 p) { return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453); }

// 高解像度の絵を4点で平均して取る（縮小時のギザギザ・ちらつき防止）
float3 Scene(float2 uv) {
    float2 o = g_Res.xy * 0.25;
    float3 c = tScene.Sample(s0, uv + float2(-o.x, -o.y)).rgb;
    c += tScene.Sample(s0, uv + float2( o.x, -o.y)).rgb;
    c += tScene.Sample(s0, uv + float2(-o.x,  o.y)).rgb;
    c += tScene.Sample(s0, uv + float2( o.x,  o.y)).rgb;
    return c * 0.25;
}
float3 Glow(float2 uv) {
    float2 o = g_Res.xy * 0.25;
    float3 c = tGlow.Sample(s1, uv + float2(-o.x, -o.y)).rgb;
    c += tGlow.Sample(s1, uv + float2( o.x,  o.y)).rgb;
    return c * 0.5;
}

float4 main(PS_INPUT i) : SV_TARGET0 {
    float2 uv = i.uv;
    float2 asp = float2(g_Res.z, 1.0);

    // ---- 衝撃波（輪の部分だけ画面をゆがませる） ----
    [unroll] for (int k = 0; k < 4; ++k) {
        float4 s = g_Shock[k];
        if (s.w > 0.0) {
            float2 d = (uv - s.xy) * asp;
            float dist = length(d);
            float ring = exp(-pow((dist - s.z) / 0.035, 2.0));
            uv -= (d / max(dist, 1e-4)) / asp * ring * s.w * 0.025;
        }
    }

    // ---- 色収差＋高速時の放射ブラー ----
    float2 dc = uv - 0.5;
    float ca = g_Fx.x * dot(dc, dc) * 2.0;
    float3 col = 0;
    float amt = g_Speed.z;
    const int N = 6;
    [unroll] for (int n = 0; n < N; ++n) {
        float t = (n / (N - 1.0)) * amt * 0.05;
        float2 su = lerp(uv, g_Speed.xy, t);
        float3 c;
        c.r = Scene(su + dc * ca).r;
        c.g = Scene(su).g;
        c.b = Scene(su - dc * ca).b;
        col += c;
    }
    col /= N;

    // ---- ライティング：ぼかした発光で周りを照らす ----
    float3 light = tLight.Sample(s3, uv).rgb;
    col *= g_Fx.w + light * g_Fx.z;

    // ---- 発光とブルーム ----
    float3 glow = Glow(uv);
    float3 bloom = tBloom.Sample(s2, uv).rgb;
    col += glow * g_Flash.w + bloom * g_Fx.y;

    // ---- 光の筋（太陽・月から放射状に） ----
    if (g_Sun.z > 0.0) {
        float2 delta = (uv - g_Sun.xy) / 32.0 * 0.85;
        float2 p = uv;
        float illum = 1.0;
        float3 rays = 0;
        [loop] for (int r = 0; r < 32; ++r) {
            p -= delta;
            rays += tBloom.SampleLevel(s2, p, 0).rgb * illum;
            illum *= g_Sun.w;
        }
        col += rays / 32.0 * g_Sun.z;
    }

    // ---- トーンマップと色調 ----
    col *= g_Grade.x;
    col = ACES(col);
    col = saturate((col - 0.5) * g_Grade.y + 0.5);
    float luma = dot(col, float3(0.2126, 0.7152, 0.0722));
    col = lerp(luma.xxx, col, g_Grade.z);
    col *= g_Tint.rgb;

    // ---- ビネット ----
    float v = 1.0 - g_Grade.w * pow(saturate(length(dc * float2(1.0, 0.85)) * 1.3), 2.4);
    col *= v;

    // ---- フラッシュ ----
    col = lerp(col, g_Flash.rgb, g_Speed.w);

    // ---- フィルムノイズ＋ディザ（グラデーションの縞を消す） ----
    float h = Hash(i.pos.xy + frac(g_Res.w) * 113.0);
    col += (h - 0.5) * g_Tint.w;
    col += (Hash(i.pos.yx * 1.7 + 3.1) - 0.5) / 255.0;
    return float4(saturate(col), 1.0);
}
