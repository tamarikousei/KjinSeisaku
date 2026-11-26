// ============================================================
//  down.hlsl : ブルーム用の縮小（13タップ）
//  1回目だけ「発光レイヤー＋通常レイヤーの明るい部分」を足して取り出す
// ============================================================
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 dif : COLOR0;
    float4 spc : COLOR1;
    float2 uv  : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};
cbuffer cbDown : register(b4) {
    float4 g_P;   // x,y = 元テクスチャの1テクセル  z = 明るさのしきい値  w = 1なら1回目（通常レイヤーも使う）
};
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);
Texture2D t0 : register(t0);   // 発光レイヤー（2回目以降は1つ上の段）
Texture2D t1 : register(t1);   // 通常レイヤー（1回目だけ）

float3 Src(float2 uv) {
    float3 c = t0.Sample(s0, uv).rgb;
    if (g_P.w > 0.5) {
        float3 s = t1.Sample(s1, uv).rgb;
        float l = max(s.r, max(s.g, s.b));
        c += s * saturate((l - g_P.z) * 4.0) * 0.25;   // 通常レイヤーのとても明るい所だけ少し光らせる
    }
    return c;
}

float4 main(PS_INPUT i) : SV_TARGET0 {
    float2 t = g_P.xy;
    float2 uv = i.uv;
    // Call of Duty 方式の13タップ（ちらつきの少ない縮小）
    float3 a = Src(uv + t * float2(-2, -2));
    float3 b = Src(uv + t * float2( 0, -2));
    float3 c = Src(uv + t * float2( 2, -2));
    float3 d = Src(uv + t * float2(-2,  0));
    float3 e = Src(uv);
    float3 f = Src(uv + t * float2( 2,  0));
    float3 g = Src(uv + t * float2(-2,  2));
    float3 h = Src(uv + t * float2( 0,  2));
    float3 k = Src(uv + t * float2( 2,  2));
    float3 j = Src(uv + t * float2(-1, -1));
    float3 l = Src(uv + t * float2( 1, -1));
    float3 m = Src(uv + t * float2(-1,  1));
    float3 n = Src(uv + t * float2( 1,  1));
    float3 o = e * 0.125;
    o += (a + c + g + k) * 0.03125;
    o += (b + d + f + h) * 0.0625;
    o += (j + l + m + n) * 0.125;
    return float4(o, 1.0);
}
