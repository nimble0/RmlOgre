Texture2D    srcTex : register(t0);
SamplerState mySampler : register(s0);

uniform float4x4 colourMatrix;

float4 main
(
	float2 uv : TEXCOORD0
) : SV_Target
{
	float4 src = srcTex.Sample( mySampler, uv );
	return mul(colourMatrix, src);
}
