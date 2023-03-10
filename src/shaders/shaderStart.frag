#version 410 core
#define POINTLIGHT_NO 4

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;
in vec4 fPos;

out vec4 fColor;

struct pointLight {    
    vec3 position;
    float constant;
    float linear;
    float quadratic;  
	vec3 color;
};  

struct spotLight{
	vec3 position;
	vec3 direction;
	float cutOff;
	float outerCutOff;
};

uniform pointLight pointLights[POINTLIGHT_NO];
uniform spotLight mySpotLight;

//lighting
uniform	vec3 lightDir;
uniform	vec3 lightColor;
uniform vec3 lightPosEye;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;
uniform samplerCube skybox;
uniform float fogDensity;
uniform float reflectiveFlag;
uniform float transparentFlag;
uniform float pointFlag;
uniform float spotFlag;

vec3 ambient;
vec3 diffuse;
vec3 specular;

//directional light
float ambientStrength = 0.2f;
float specularStrength = 0.5f;
float shininess = 32.0f;


//point light
float ambientPointStrength = 0.5f;
float specularPointStrength = 0.005f;

//spotlight
vec3 spotLightAmbient = vec3(0.0f,0.0f,0.0f);
vec3 spotLightDiffuse = vec3(0.9f, 0.35f, 0.0f);
vec3 spotLightSpecular = vec3(1.0f,1.0f,1.0f);


//directional light
void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(fNormal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDir);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
		
	//compute ambient light
	ambient = ambientStrength * lightColor;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
}

//point light
vec3 computePointLight(pointLight light){
	vec3 cameraPosEye = vec3(0.0f);
	vec3 normalEye = normalize(fNormal);
	vec3 lightDirN = normalize(light.position - fPos.xyz);
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
	
	vec3 ambientAux = ambientPointStrength * light.color;
	vec3 diffuseAux = max(dot(normalEye, lightDirN), 0.0f) * light.color;
	
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	
	vec3 specularAux = specularPointStrength * specCoeff * light.color;
	
	float dist = length(light.position - fPos.xyz);
	float att = 1.0f / (light.constant + light.linear * dist + light.quadratic * dist * dist);
	ambientAux *= att * texture(diffuseTexture, fTexCoords).rgb;
	diffuseAux *= att * texture(diffuseTexture, fTexCoords).rgb;
	specular *= att * texture(specularTexture, fTexCoords).rgb;
	return (ambientAux + diffuseAux + specularAux);
}

//spot light
vec3 computeSpotLight(){
	vec3 lightDir = normalize(mySpotLight.position - fPos.xyz);
	
	float theta = dot(lightDir, normalize (-mySpotLight.direction));
	float epsilon = mySpotLight.cutOff - mySpotLight.outerCutOff;
	float intensity = clamp((theta - mySpotLight.outerCutOff) / epsilon, 0.0f, 1.0f);
	
	vec3 ambientAux =  spotLightAmbient * texture(diffuseTexture, fTexCoords).rgb;
	vec3 diffuseAux =  spotLightDiffuse * intensity * texture(diffuseTexture, fTexCoords).rgb;
	vec3 specularAux = spotLightSpecular * intensity * texture(specularTexture, fTexCoords).rgb;
	
	return (ambientAux + diffuseAux + specularAux);
	
}

float computeShadow()
{
	//perform perspective divide
	vec3 normalizedCoords= fragPosLightSpace.xyz / fragPosLightSpace.w;

	//tranform to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;

	//Get closest depth value from light's perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;

	//Get depth of current fragment from lights perspective
	float currentDepth = normalizedCoords.z;

	float bias = 0.0005f;
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	if (normalizedCoords.z > 1.0f)
		return 0.0f;
	return shadow;
}

float computeFog()
{
	float fragmentDistance = length(fPosEye);
	float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
	return clamp(fogFactor, 0.0f, 1.0f);
}


void main() 
{
	computeLightComponents();
	
	vec3 baseColor = vec3(0.9f, 0.35f, 0.0f);//orange
	
	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;

	float shadow = computeShadow();
	vec3 color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);

	if (pointFlag == 1.0f){	
		for(int i = 0; i<POINTLIGHT_NO;i++)
			color += computePointLight(pointLights[i]);
	}
	
	if (spotFlag == 1.0f){
		color+=computeSpotLight();
	}

	vec4 colorFromTexture = texture(diffuseTexture,fTexCoords);
    	if (colorFromTexture.a < 0.5)
		discard;
	float fogFactor = computeFog();
	vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
	
	if(reflectiveFlag == 1.0f){
		vec3 viewDirectionN = normalize(fPosEye.xyz); 
		vec3 normalN = normalize(fNormal); 
		vec3 reflection = reflect(viewDirectionN, normalN); 
		color = vec3(texture(skybox, reflection));
	}

	float alpha = 1.0f;
	if(transparentFlag == 1.0f)
		alpha = 0.6;
	fColor = fogColor * (1-fogFactor) + vec4(color, alpha)* fogFactor;
}
