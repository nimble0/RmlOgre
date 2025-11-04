#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D srcTex;

vulkan( layout( ogre_s0 ) uniform sampler texSampler );

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( location = 0 )
out vec4 fragColour;

const float gamma = 2.2;

void main()
{
	vec4 src = texture( vkSampler2D( srcTex, texSampler ), inPs.uv0 );
	fragColour = vec4(pow(src.rgb, vec3(1.0 / gamma)), src.a);
}
