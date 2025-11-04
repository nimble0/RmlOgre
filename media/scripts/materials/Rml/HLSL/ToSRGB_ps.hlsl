Texture2D    srcTex : register(t0);
SamplerState mySampler : register(s0);

struct PS_INPUT
{
	float2 uv0 : TEXCOORD0;
};

static const float gamma = 2.2;

float4 main
(
	PS_INPUT inPs
) : SV_Target
{
	float4 src = srcTex.Sample( mySampler, inPs.uv0 );
	return float4(pow(src.rgb, 1.0 / gamma), src.a);
}
