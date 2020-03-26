#define FXAA_GRAY_AS_LUMA 1
#define FXAA_PC 1
#define FXAA_HLSL_5 1
#include "FXAA.hlsl"

Texture2D<float4> tex : register(t0);	// 通常テクスチャ用
Texture2D<float4> texNormal : register(t1);	// 通常ノーマルテクスチャ用
Texture2D<float4> texHighLum : register(t2);	// 通常ノーマルテクスチャ用
SamplerState smp : register(s0);	// サンプラー用レジスタ

cbuffer RMMovement : register(b0)
{
    float3 pos;
};

struct Out {
	float4 svpos : SV_POSITION;
    float4 pos : POSITIONT;
	float2 uv : TEXCOORD;
};

float SDFCircle2D(float2 xy, float2 center, float r)
{
    return length(center - xy) - r;
}

float SDFLatticeCircle2D(float2 xy, float divider)
{
    return length(fmod(xy, divider) - divider / 2) - divider / 2;

}

float SDFSphere3D(float3 xyz, float r)
{
    return length(xyz - float3(0,0,0)) - r;
}

float SDFLatticeCircle3D(float3 xyz, float divider,float r)
{
    return length(fmod(xyz, divider) - divider / 2.f) - r;

}


float4 GetGaussianBlur(Texture2D texN, SamplerState samp, float2 inuv)
{
    float4 col = texN.Sample(smp, inuv);
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float x = 1.0f / w;
    float y = 1.0f / h;
    col = col * 36 / 256;
    x *= 2;
    y *= 2;
    col += texN.Sample(samp, inuv + float2(-2 * x, 2 * y)) * 1 / 256;
    col += texN.Sample(samp, inuv + float2(-1 * x, 2 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(0 * x, 2 * y)) * 6 / 256;
    col += texN.Sample(samp, inuv + float2(1 * x, 2 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(2 * x, 2 * y)) * 1 / 256;
    
    col += texN.Sample(samp, inuv + float2(-2 * x, 1 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(-1 * x, 1 * y)) * 16 / 256;
    col += texN.Sample(samp, inuv + float2(0 * x, 1 * y)) * 24 / 256;
    col += texN.Sample(samp, inuv + float2(1 * x, 1 * y)) * 16 / 256;
    col += texN.Sample(samp, inuv + float2(2 * x, 1 * y)) * 4 / 256;
    
    col += texN.Sample(samp, inuv + float2(-2 * x, 0 * y)) * 6 / 256;
    col += texN.Sample(samp, inuv + float2(-1 * x, 0 * y)) * 24 / 256;
    
    col += texN.Sample(samp, inuv + float2(1 * x, 0 * y)) * 24 / 256;
    col += texN.Sample(samp, inuv + float2(2 * x, 0 * y)) * 6 / 256;
    
    col += texN.Sample(samp, inuv + float2(-2 * x, -1 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(-1 * x, -1 * y)) * 16 / 256;
    col += texN.Sample(samp, inuv + float2(0 * x, -1 * y)) * 24 / 256;
    col += texN.Sample(samp, inuv + float2(1 * x, -1 * y)) * 16 / 256;
    col += texN.Sample(samp, inuv + float2(2 * x, -1 * y)) * 4 / 256;
    
    col += texN.Sample(samp, inuv + float2(-2 * x, -2 * y)) * 1 / 256;
    col += texN.Sample(samp, inuv + float2(-1 * x, -2 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(0 * x, -2 * y)) * 6 / 256;
    col += texN.Sample(samp, inuv + float2(1 * x, -2 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(2 * x, -2 * y)) * 1 / 256;
    return col;
}

float4 GetGaussianBlurWH(Texture2D texN, SamplerState samp, float2 inuv,float inw, float inh)
{
    float4 col = texN.Sample(smp, inuv);
    float x = 1.0f / inw;
    float y = 1.0f / inh;
    col = col * 36 / 256;
    x *= 2;
    y *= 2;
    col += texN.Sample(samp, inuv + float2(-2 * x, 2 * y)) * 1 / 256;
    col += texN.Sample(samp, inuv + float2(-1 * x, 2 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(0 * x, 2 * y)) * 6 / 256;
    col += texN.Sample(samp, inuv + float2(1 * x, 2 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(2 * x, 2 * y)) * 1 / 256;
    
    col += texN.Sample(samp, inuv + float2(-2 * x, 1 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(-1 * x, 1 * y)) * 16 / 256;
    col += texN.Sample(samp, inuv + float2(0 * x, 1 * y)) * 24 / 256;
    col += texN.Sample(samp, inuv + float2(1 * x, 1 * y)) * 16 / 256;
    col += texN.Sample(samp, inuv + float2(2 * x, 1 * y)) * 4 / 256;
    
    col += texN.Sample(samp, inuv + float2(-2 * x, 0 * y)) * 6 / 256;
    col += texN.Sample(samp, inuv + float2(-1 * x, 0 * y)) * 24 / 256;
    
    col += texN.Sample(samp, inuv + float2(1 * x, 0 * y)) * 24 / 256;
    col += texN.Sample(samp, inuv + float2(2 * x, 0 * y)) * 6 / 256;
    
    col += texN.Sample(samp, inuv + float2(-2 * x, -1 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(-1 * x, -1 * y)) * 16 / 256;
    col += texN.Sample(samp, inuv + float2(0 * x, -1 * y)) * 24 / 256;
    col += texN.Sample(samp, inuv + float2(1 * x, -1 * y)) * 16 / 256;
    col += texN.Sample(samp, inuv + float2(2 * x, -1 * y)) * 4 / 256;
    
    col += texN.Sample(samp, inuv + float2(-2 * x, -2 * y)) * 1 / 256;
    col += texN.Sample(samp, inuv + float2(-1 * x, -2 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(0 * x, -2 * y)) * 6 / 256;
    col += texN.Sample(samp, inuv + float2(1 * x, -2 * y)) * 4 / 256;
    col += texN.Sample(samp, inuv + float2(2 * x, -2 * y)) * 1 / 256;
    return col;
}

Out vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Out o;
	o.uv = uv;
	o.svpos = o.pos = pos;
	return o;
}

float4 ps(Out o) : SV_Target
{
    float2 aspect = float2(1280.0f / 720.0f, 1);
    //float sdf = SDFCircle2D(o.pos.xy * aspect, float2(0, 0), 0.1);
    
    //3D
    float3 eye = float3(0, 0, -2.5f);
    float3 tpos = float3(o.pos.xy * aspect, 0);
    float3 ray = normalize(tpos - eye);
    float rsph = 4.f;
    for (int i = 0; i < 64; i++)
    {
        float len = SDFLatticeCircle3D(abs(eye + pos), rsph * 8.f, rsph);
        eye += ray * len;
        if (len < 0.0001f)
        {
            float3 texcolor = tex.Sample(smp, o.uv).rgb;
            if (texcolor.r <= 0.f && texcolor.g <= 0.f && texcolor.b <= 0.f)
            {
                return float4((float) (64.f - i) / 64.0f, (float) (64.f - i) / 64.0f, (float) (64.f - i) / 64.0f, 1);
            }
            else
            {
                return tex.Sample(smp, o.uv) + GetGaussianBlur(texHighLum, smp, o.uv) * 5;
            }
        }
        
    }
    return tex.Sample(smp, o.uv) + GetGaussianBlur(texHighLum, smp, o.uv) * 5;
    
    //float sdf = SDFLatticeCircle2D(o.uv * aspect,0.1);
    ////sdf = length(fmod(o.uv, 0.1)) - 0.1f;
    //if (sdf < 0)
    //{
    //    //2D
    //    sdf = abs(sdf) * (1.0 / 0.1);
    //    float2 xy = (fmod(o.uv * aspect, 0.1) - (0.1 / 2)) * (1.0 / 0.1) * float2(1, -1);
    //    float3 sdfNormal = normalize(float3(xy, -sdf));
    //    sdf = max(saturate(dot(normalize(float3(-1, 1, -1)), sdfNormal)), 0.15f);
    //    return float4(sdf, sdf, sdf, 1);
    //}
    //else
    //{
    //    float4 normal = texNormal.Sample(smp, o.uv);
    //    normal = normal * 2.f - 1.f;
    //    float3 light = normalize(float3(1.f, -1.f, 1.f));
    //    const float ambient = 0.25f;
    //    float diffB = max(saturate(dot(normal.xyz, -light)), ambient);
    //    return tex.Sample(smp, o.uv) + GetGaussianBlur(texHighLum, smp, o.uv) * 5;
    //    FxaaTex InputFXAATex = { smp, tex };
    //// ディファードレンダリング
    ////return tex.Sample(smp, o.uv) * float4(diffB, diffB, diffB, 1);
    //// FXAAレンダリング
    //    return float4(FxaaPixelShader(o.uv,
    //FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),
    //InputFXAATex,
    //InputFXAATex,
    //InputFXAATex,
    //float2(1.0f, 1.0f) / float2(1280, 760),
    //FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),
    //FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),
    //FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),
    //0.75f,
    //0.166f,
    //0.0833f,
    //8.0f,
    //0.125f,
    //0.05f,
    //FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f)).rgb, 1);
    //}
    
}

float4 BlurPS(Out input) : SV_Target
{
    float w, h, mipLevel;
    tex.GetDimensions(0, w, h, mipLevel);
    return GetGaussianBlurWH(tex, smp, input.uv,1.0f / w,1.0f / h);
}