static const int MAX_STOP_COLOURS = 16;
// MAX_STOP_POSITIONS = MAX_STOP_COLOURS / 4
static const int MAX_STOP_POSITIONS = 4;

static const int LINEAR_GRADIENT = 0;
static const int RADIAL_GRADIENT = 1;
static const int CONIC_GRADIENT = 2;
static const float PI = 3.14159265;

// glsl style mod
#define mod(x, y) (x - y * floor(x / y))

Texture2D    srcTex : register(t0);
SamplerState mySampler : register(s0);

uniform int gradientType;
uniform int repeating;
uniform float2 origin;
uniform float2 scale;
uniform int numStops;
uniform float4 stopPositions[MAX_STOP_POSITIONS];
uniform float4 stopColours[MAX_STOP_COLOURS];

struct PS_INPUT
{
	float2 uv0         : TEXCOORD0;
	float4 diffuse     : DIFFUSE;
	//float4 gl_Position : SV_POSITION;
};

float get_stop_position(int i)
{
	int y = i >> 2;
	int x = i & 3;
	return stopPositions[y][x];
}

float4 mix_stop_colours(float t)
{
	float4 colour = stopColours[0];
	for(int i = 1; i < numStops; ++i)
	{
		colour = lerp(
			colour,
			stopColours[i],
			smoothstep(get_stop_position(i - 1), get_stop_position(i), t));
	}

	return colour;
}

float linear_gradient(float2 origin, float2 scale, float2 p)
{
	return dot(scale, p - origin) / dot(scale, scale);
}

float radial_gradient(float2 origin, float2 scale, float2 p)
{
	return length(scale * (p - origin));
}

float conic_gradient(float2 origin, float2 scale, float2 p)
{
	float2x2 scaleMat = float2x2(scale.x, -scale.y, scale.y, scale.x);
	float2 v = mul(scaleMat, (p - origin));
	return 0.5 + atan2(-v.x, v.y) / (2.0 * PI);
}

float4 main
(
	PS_INPUT inPs
) : SV_Target
{
	float t = 0.0;
	if(gradientType == LINEAR_GRADIENT)
		t = linear_gradient(origin, scale, inPs.uv0);
	else if(gradientType == RADIAL_GRADIENT)
		t = radial_gradient(origin, scale, inPs.uv0);
	else if(gradientType == CONIC_GRADIENT)
		t = conic_gradient(origin, scale, inPs.uv0);

	if(repeating != 0)
	{
		float t0 = get_stop_position(0);
		float t1 = get_stop_position(numStops - 1);
		t = t0 + mod(t - t0, t1 - t0);
	}

	return inPs.diffuse * mix_stop_colours(t);
}
