Texture2D    srcTex : register(t0);
SamplerState mySampler : register(s0);

uniform float4 viewportSize;
uniform float4 colour;
uniform float2 offset;

struct PS_INPUT
{
	float2 uv0 : TEXCOORD0;
};

float4 main
(
	PS_INPUT inPs
) : SV_Target
{
	float4 primary = srcTex.Sample( mySampler, inPs.uv0 );
	float shadowA = srcTex.Sample( mySampler, inPs.uv0 - offset * viewportSize.zw ).a;
	return primary + colour * shadowA * (1 - primary.a);
}


