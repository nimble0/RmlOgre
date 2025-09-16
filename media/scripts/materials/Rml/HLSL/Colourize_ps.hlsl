Texture2D    srcTex : register(t0);
SamplerState mySampler : register(s0);

uniform float4 colour;

struct PS_INPUT
{
	float2 uv0 : TEXCOORD0;
};

float4 main
(
	PS_INPUT inPs
) : SV_Target
{
	float a = srcTex.Sample( mySampler, inPs.uv0 ).a;
	return colour * a;
}
