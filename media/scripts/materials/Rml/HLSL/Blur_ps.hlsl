#define NUM_WEIGHTS 8
// NUM_WEIGHTS / 4
#define NUM_WEIGHTS_ARRAY_SIZE 2

Texture2D    srcTex : register(t0);
SamplerState mySampler : register(s0);

uniform float4 viewportSize;
uniform float2 direction;
uniform float4 weights[NUM_WEIGHTS_ARRAY_SIZE];

struct PS_INPUT
{
	float2 uv0 : TEXCOORD0;
};

float get_weight(int i)
{
	int y = i >> 2;
	int x = i & 3;
	return weights[y][x];
}

float4 main
(
	PS_INPUT inPs
) : SV_Target
{
	float2 d = direction * viewportSize.zw;

	float4 colour = float4(0.0, 0.0, 0.0, 0.0);
	float2 p = inPs.uv0 + d * (-NUM_WEIGHTS + 1);
	for(int i = -NUM_WEIGHTS + 1; i < NUM_WEIGHTS; ++i)
	{
		float4 c = srcTex.Sample( mySampler, p );
		colour += c * get_weight(abs(i));
		p += d;
	}

	return colour;
}


