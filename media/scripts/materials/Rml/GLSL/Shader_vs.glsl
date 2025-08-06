#version ogre_glsl_ver_330

vulkan_layout( OGRE_POSITION ) in vec3 vertex;
vulkan_layout( OGRE_DIFFUSE ) in vec4 colour;
vulkan_layout( OGRE_TEXCOORD0 ) in vec2 uv0;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform mat4 worldViewProj;
vulkan( }; )

out gl_PerVertex
{
	vec4 gl_Position;
};

vulkan_layout( location = 0 )
out block
{
	vec4 colour;
	vec2 uv0;
} outVs;

void main()
{
	gl_Position = worldViewProj * vec4( vertex, 1.0 );
	outVs.colour = colour;
	outVs.uv0 = uv0;
}
