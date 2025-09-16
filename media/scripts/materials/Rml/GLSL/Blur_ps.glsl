#version ogre_glsl_ver_330

#define NUM_WEIGHTS 8
// NUM_WEIGHTS / 4
#define NUM_WEIGHTS_ARRAY_SIZE 2

vulkan_layout( ogre_t0 ) uniform texture2D srcTex;

vulkan( layout( ogre_s0 ) uniform sampler texSampler );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec4 viewportSize;
	uniform vec2 direction;
	uniform vec4 weights[NUM_WEIGHTS_ARRAY_SIZE];
vulkan( }; )

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( location = 0 )
out vec4 fragColour;

float get_weight(int i)
{
	int y = i >> 2;
	int x = i & 3;
	return weights[y][x];
}

void main()
{
	vec2 d = direction * viewportSize.zw;

	vec4 colour = vec4(0.0, 0.0, 0.0, 0.0);
	vec2 p = inPs.uv0 + d * (-NUM_WEIGHTS + 1);
	for(int i = -NUM_WEIGHTS + 1; i < NUM_WEIGHTS; ++i)
	{
		vec4 c = texture( vkSampler2D( srcTex, texSampler ), p );
		colour += c * get_weight(abs(i));
		p += d;
	}

	fragColour = colour;
}
