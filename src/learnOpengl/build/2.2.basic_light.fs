#version 330 core

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform vec3 objectColor;

void main()
{
	// ambient 
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength *lightColor;


	// diffuse
	vec3 normal = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(normal,lightDir), 0);
	vec3 diffuse = diff *lightColor;

	// specular 
	float specularStrength =0.5;
	vec3 viewDir = normalize(cameraPos - FragPos);
	vec3 refletDir = reflect(-lightDir, normal );
	
	float spec= pow(max(dot(refletDir, viewDir),0), 32);
	vec3 specular =specularStrength* spec* lightColor;

	vec3 result =(ambient + diffuse + specular) *objectColor;
	
	FragColor = vec4(result, 1.0);
}


