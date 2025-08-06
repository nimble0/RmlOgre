#version ogre_glsl_ver_330

#define MAX_STOP_COLOURS 16
// MAX_STOP_POSITIONS = MAX_STOP_COLOURS / 4
#define MAX_STOP_POSITIONS 4

const int LINEAR_GRADIENT = 0;
const int RADIAL_GRADIENT = 1;
const int CONIC_GRADIENT = 2;
const float PI = 3.14159265;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform int gradientType;
	uniform int repeating;
	uniform vec2 origin;
	uniform vec2 scale;
	uniform int numStops;
	uniform vec4 stopPositions[MAX_STOP_POSITIONS];
	uniform vec4 stopColours[MAX_STOP_COLOURS];
vulkan( }; )

vulkan_layout( location = 0 )
in block
{
	vec4 colour;
	vec2 uv0;
} inPs;

vulkan_layout( location = 0 )
out vec4 fragColour;

float get_stop_position(int i)
{
	int y = i >> 2;
	int x = i & 3;
	return stopPositions[y][x];
}

vec4 mix_stop_colours(float t)
{
	vec4 colour = stopColours[0];
	for(int i = 1; i < numStops; ++i)
	{
		colour = mix(
			colour,
			stopColours[i],
			smoothstep(get_stop_position(i - 1), get_stop_position(i), t));
	}

	return colour;
}

float linear_gradient(vec2 origin, vec2 scale, vec2 p)
{
	return dot(scale, p - origin) / dot(scale, scale);
}

float radial_gradient(vec2 origin, vec2 scale, vec2 p)
{
	return length(scale * (p - origin));
}

float conic_gradient(vec2 origin, vec2 scale, vec2 p)
{
	mat2 scaleMat = mat2(scale.x, -scale.y, scale.y, scale.x);
	vec2 v = scaleMat * (p - origin);
	return 0.5 + atan(-v.x, v.y) / (2.0 * PI);
}

void main()
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

	fragColour = inPs.colour * mix_stop_colours(t);
}
