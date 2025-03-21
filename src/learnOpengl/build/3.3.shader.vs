#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D tex;

void main()
{
//    gl_Position = projection* view * model*vec4(aPos, 1.0);
    gl_Position = vec4(aPos, 1);
    //ourColor = aColor;
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}
