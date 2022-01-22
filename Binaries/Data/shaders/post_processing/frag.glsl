#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D _ColorTexture;
uniform sampler2D _DepthTexture;

void main()
{    
    FragColor = texture(_ColorTexture, TexCoords);
}