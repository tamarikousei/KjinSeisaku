// ============================================================
//  up.hlsl : ブルーム用の拡大（3x3 テント）＋同じ段の縮小結果を足す
// ============================================================
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 dif : COLOR0;
    float4 spc : COLOR1;
    float2 uv  : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};
cbuffer cbUp : register(b4) {
    float4 g_P;   // x,y = 小さい段の1テクセル  z = 広がり  w = 足し合わせの強さ
};
SamplerState s0 : register(s0);
SamplerState s1 : register(s1);
Texture2D t0 : register(t0);   // 1つ下（小さい）の段
Texture2D t1 : register(t1);   // この段の縮小結果

float4 main(PS_INPUT i) : SV_TARGET0 {
    float2 t = g_P.xy * g_P.z;
    float2 uv = i.uv;
    float3 s = t0.Sample(s0, uv + t * float2(-1, -1)).rgb;
    s += t0.Sample(s0, uv + t * float2( 0, -1)).rgb * 2.0;
    s += t0.Sample(s0, uv + t * float2( 1, -1)).rgb;
    s += t0.Sample(s0, uv + t * float2(-1,  0)).rgb * 2.0;
    s += t0.Sample(s0, uv).rgb * 4.0;
    s += t0.Sample(s0, uv + t * float2( 1,  0)).rgb * 2.0;
    s += t0.Sample(s0, uv + t * float2(-1,  1)).rgb;
    s += t0.Sample(s0, uv + t * float2( 0,  1)).rgb * 2.0;
    s += t0.Sample(s0, uv + t * float2( 1,  1)).rgb;
    s /= 16.0;
    float3 cur = t1.Sample(s1, uv).rgb;
    return float4(cur + s * g_P.w, 1.0);
}
