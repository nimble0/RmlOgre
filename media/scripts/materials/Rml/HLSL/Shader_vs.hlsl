struct VS_INPUT
{
	float2 vertex  : POSITION;
	float2 uv0     : TEXCOORD0;
	float4 diffuse : COLOR;
};

struct PS_INPUT
{
	float2 uv0         : TEXCOORD0;
	float4 diffuse     : DIFFUSE;
	float4 gl_Position : SV_POSITION;
};

PS_INPUT main
(
	VS_INPUT input,
	uniform matrix worldViewProj
)
{
	PS_INPUT outVs;

	outVs.gl_Position = mul( worldViewProj, float4(input.vertex, 1, 1) );
	outVs.uv0         = input.uv0;
	outVs.diffuse     = input.diffuse;

	return outVs;
}

