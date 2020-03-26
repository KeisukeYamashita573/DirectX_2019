Texture2D<float> shadowMap : register(t0);
SamplerState smp : register(s0);
cbuffer mat : register(b0) {
	matrix world;
	matrix viewProj;
    matrix lvp;
};

struct PrimOutput {
	float4 svPos : SV_POSITION;
	float4 posSM : POSITION_SM;
	float2 uv : TEXCOORD;
};

PrimOutput vs(float4 pos : POSITION, float3 normal : NORMAL, float3 color : COLOR, float2 uv : TEXCOORD) {
	PrimOutput output;
	output.svPos = mul(mul(viewProj, world), pos);
	float4 Pos = pos;
	Pos = mul(world, Pos);
    Pos = mul(lvp, Pos);
	output.posSM = Pos;
	return output;
}

float4 ps(PrimOutput input) : SV_Target{
    float2 shadowUV = float2((1.f + input.posSM.x) / 2.f, (1.f - input.posSM.y) / 2.f);
    float sm = shadowMap.Sample(smp, shadowUV);
    float smp = pow(sm,100);
    return float4(1, 1, 1, 1);
    float sma = (input.posSM.z - 0.005f < smp) ? 1.f : 0.3f;
    //return float4(float3(1.f, 1.f, 1.f) * sma, 1.f);
}