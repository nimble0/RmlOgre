#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D tex;
vulkan( layout( ogre_s0 ) uniform sampler texSampler );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec4 colour;
vulkan( }; )

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( location = 0 )
out vec4 fragColour;

void main()
{
	float a = texture( vkSampler2D( tex, texSampler ), inPs.uv0 ).a;
	fragColour = colour * a;
}
