Texture2D<float4> tex : register(t0);	// 通常テクスチャ用
Texture2D<float4> sph : register(t1);	// sphテクスチャ用
Texture2D<float4> spa : register(t2);	// spaテクスチャ用
Texture2D<float4> toon : register(t3);	// toonテクスチャ用
Texture2D<float4> clut : register(t4);
SamplerState smp : register(s0);	// サンプラー用レジスタ
SamplerState toonSmp : register(s1);	// toon用サンプラー
// WVPコンスタントバッファー
cbuffer mat : register(b0) {
	matrix world;
	matrix view;
	matrix proj;
	matrix wvp;
    matrix lvp;
	float3 eye;
    matrix shadowMat;
    matrix instancePos[100];
}
// マテリアルコンスタントバッファー
cbuffer material : register(b1) {
	float4 diffuse;
	float4 specular;
	float4 ambient;
}

cbuffer bones : register(b2) {
	matrix boneMats[512];
}

// 出力用構造体
struct Out {
	float4 svpos : SV_POSITION;
	float4 pos : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL0;
	float4 vnormal : NORMAL1;
	float3 ray : VECTOR;
	min16uint2 boneno : BONENO;
	min16uint weight : WEIGHT;
    uint instNo : SV_InstanceID;
    float4 posSM : POSITIONSM;
};

struct PixelOutput{
    float4 col : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 highLum : SV_TARGET2;
};

Out vs(float4 pos : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT, uint instNo: SV_InstanceID)
{
	Out o;
	float w = weight / 100.f;
	matrix m = boneMats[boneno.x] * w + boneMats[boneno.y] * (1 - w);
    if (instNo % 2 == 1)
    {
        m = mul(shadowMat, m);
    }
    pos = mul(m, pos);
    pos = mul(instancePos[instNo], pos);
    pos = mul(wvp, pos);
    o.uv = uv;
	o.normal = mul(world, normal);
	o.vnormal = mul(view, o.normal);
	o.ray = normalize(pos.xyz - eye);
	o.boneno = boneno;
	o.weight = weight;
    o.svpos = o.pos = pos;
    o.instNo = instNo;
    o.posSM = mul(lvp, o.normal);
    return o;
}

PixelOutput ps(Out o) : SV_Target
{
    PixelOutput pixOut;
	// lightの設定
	float3 light = float3(0,20,-15);
	light = normalize(light);

	float brightness = saturate(dot(light, o.normal));
	// トゥーン
	float4 toonDif = toon.Sample(toonSmp, float2(0, 1.0f - brightness));
	// 反射光
	float3 refLight = normalize(reflect(light, o.normal.xyz));
	// キュピーン
	float specularB = pow(saturate(dot(refLight, -o.ray)), specular.a);
	// スフィアマップ用UV
	float2 sphereMapUV = (o.vnormal.xy + float2(1, -1)) * float2(0.5f, 0.5f);
	float4 color = tex.Sample(smp, o.uv);
    
    float2 shadowUV = float2((1.f + o.posSM.x) / 2.f, (1.f - o.posSM.y) / 2.f);
    float sm = pow(clut.Sample(smp, shadowUV), 100);
    float sma = (o.posSM.z + 0.0005f < sm) ? 1.f : 0.5f;
	//return (0, 0, 0, 1);
	//return float4(float3(float3(diffuse.rgb * brightness + specular.rgb * specular.a + ambient.rgb) * color.rgb),color.w);
	//return float4(brightness, brightness, brightness, 1) * diffuse * tex.Sample(smp,o.uv) * sph.Sample(smp, normalUV) + spa.Sample(smp, normalUV) + float4(color * ambient.xyz, 1);
    if (o.instNo % 2 == 1)
    {
        pixOut.col = float4(0.3f, 0.3f, 0.3f, 1);
        pixOut.normal.rgb = float3(0, 0, 0);
        return pixOut;
    }
        // フォワードレンダリング
    pixOut.col = toonDif * diffuse * color * sma * sph.Sample(smp, sphereMapUV) + spa.Sample(smp, sphereMapUV) * color + float4(specularB * specular.rgb, 1) + float4(color.rgb * ambient.xyz * 0.5f, 1);
    // ディファードレンダリング
    //pixOut.col = float4(spa.Sample(smp,sphereMapUV) + sph.Sample(smp, sphereMapUV) * color * diffuse);
    float y = dot(float3(0.299f, 0.587f, 0.114f), pixOut.col);
    pixOut.normal.rgb = float3((o.normal.xyz + 1.0f) / 2.0f);
    pixOut.normal.a = 1;
    // テクスチャカラーの１以上の色を抽出
    //pixOut.highLum = (pixOut.col > 1.0f);
    // 白色を抽出
    pixOut.highLum = y > 0.99f ? pixOut.col : 0.0f;
    return pixOut;
    //return toonDif * diffuse * color * sma * sph.Sample(smp, sphereMapUV) + spa.Sample(smp, sphereMapUV) * color + float4(specularB * specular.rgb, 1) + float4(color.rgb * ambient.xyz * 0.5f, 1);
	//return float4(saturate(diffuse.rgb * brightness + specular.rgb * spec + ambient.rgb), 1) * color * sph.Sample(smp, sphereMapUV) + spa.Sample(smp, sphereMapUV) + float4(color * ambient.xyz, 1);
}

float4 shadowVs(float4 pos : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT) : SV_POSITION
{
    float w = weight / 100.f;
    matrix n = boneMats[boneno.x] * w + boneMats[boneno.y] * (1 - w);
    pos = mul(n, pos);
    float4 Pos = mul(world, pos);
    Pos = mul(lvp, Pos);
    return Pos;
}