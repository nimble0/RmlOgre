Texture2D    dstTex : register(t0);
Texture2D    srcTex : register(t1);
SamplerState mySampler : register(s0);

float4 main( float2 uv : TEXCOORD0 ) : SV_Target
{
	float4 dst = dstTex.Sample( mySampler, uv );
	float4 src = srcTex.Sample( mySampler, uv );
	return dst * src.a;
}
