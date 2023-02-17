#version 430 core

// Special thanks to Nadir Rom�n Guerrero https://github.com/NadirRoGue for inspiring me doing this work!
// Thanks to WFP from gamedev.net and the rest of the community who partecipated at this topic https://www.gamedev.net/forums/topic/680832-horizonzero-dawn-cloud-system/?page=6
// WFP web site http://roar11.com/
// Thanks to reinder and his beautiful Himalayas https://www.shadertoy.com/view/MdGfzh 
// Reinder https://www.shadertoy.com/user/reinder 
// Thanks to Rikard Olajos https://github.com/rikardolajos/clouds
// Thanks to Clay John https://github.com/clayjohn/realtime_clouds

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D fragColor;
layout(rgba32f, binding = 1) uniform image2D bloom;
layout(rgba32f, binding = 2) uniform image2D alphaness;
layout(rgba32f, binding = 3) uniform image2D cloudDistance;

uniform sampler2D sky;

uniform float FOV;
uniform vec2 iResolution;
uniform float iTime;
uniform mat4 inv_view;
uniform mat4 inv_proj;

uniform mat4 invViewProj;

uniform vec3 lightColor = vec3(1.0);
uniform sampler3D cloud;
uniform sampler3D worley32;
uniform sampler2D weatherTex;
uniform sampler2D depthMap;
uniform vec3 lightDirection;

uniform float coverage_multiplier = 0.4;
uniform float cloudSpeed;
uniform float crispiness = 0.4;

uniform vec3 cameraPosition;

uniform vec3 cloudColorTop = (vec3(169., 149., 149.)*(1.5/255.));
uniform vec3 cloudColorBottom =  (vec3(65., 70., 80.)*(1.5/255.));

#define CLOUDS_AMBIENT_COLOR_TOP cloudColorTop
#define CLOUDS_AMBIENT_COLOR_BOTTOM cloudColorBottom

// Cone sampling random offsets
uniform vec3 noiseKernel[6u] = vec3[] 
(
	vec3( 0.38051305,  0.92453449, -0.02111345),
	vec3(-0.50625799, -0.03590792, -0.86163418),
	vec3(-0.32509218, -0.94557439,  0.01428793),
	vec3( 0.09026238, -0.27376545,  0.95755165),
	vec3( 0.28128598,  0.42443639, -0.86065785),
	vec3(-0.16852403,  0.14748697,  0.97460106)
);


// Cloud types height density gradients
#define STRATUS_GRADIENT vec4(0.0, 0.1, 0.2, 0.3)
#define STRATOCUMULUS_GRADIENT vec4(0.02, 0.2, 0.48, 0.625)
#define CUMULUS_GRADIENT vec4(0.00, 0.1625, 0.88, 0.98)


uniform float earthRadius = 600000.0;
uniform float sphereInnerRadius = 5000.0;
uniform float sphereOuterRadius = 17000.0;

#define EARTH_RADIUS earthRadius
#define SPHERE_INNER_RADIUS (EARTH_RADIUS + sphereInnerRadius)
#define SPHERE_OUTER_RADIUS (SPHERE_INNER_RADIUS + sphereOuterRadius)
#define SPHERE_DELTA float(SPHERE_OUTER_RADIUS - SPHERE_INNER_RADIUS)


#define CLOUDS_MIN_TRANSMITTANCE 1e-1
#define CLOUDS_TRANSMITTANCE_THRESHOLD 1.0 - CLOUDS_MIN_TRANSMITTANCE

#define SUN_DIR lightDirection
#define SUN_COLOR lightColor*vec3(1.1,1.1,0.95)
vec3 sphereCenter = vec3(0.0, -EARTH_RADIUS, 0.0);


float HG( float sundotrd, float g) {
	float gg = g * g;
	return (1. - gg) / pow( 1. + gg - 2. * g * sundotrd, 1.5);
}


bool intersectCubeMap(vec3 o, vec3 d, out vec3 minT, out vec3 maxT)
{		
	vec3 cubeMin = vec3(-0.5, -0.5, -0.5);
	vec3 cubeMax = vec3(0.5, 1 + cubeMin.y, 0.5);

	// compute intersection of ray with all six bbox planes
	vec3 invR = 1.0 / d;
	vec3 tbot = invR * (cubeMin - o);
	vec3 ttop = invR * (cubeMax - o);
	// re-order intersections to find smallest and largest on each axis
	vec3 tmin = min (ttop, tbot);
	vec3 tmax = max (ttop, tbot);
	// find the largest tmin and the smallest tmax
	vec2 t0 = max (tmin.xx, tmin.yz);
	float tnear = max (t0.x, t0.y);
	t0 = min (tmax.xx, tmax.yz);
	float tfar = min (t0.x, t0.y);
	
	// check for hit
	bool hit;
	if ((tnear > tfar) || tfar < 0.0)
		hit = false;
	else
		hit = true;

	minT = tnear < 0.0? o : o + d * tnear; // if we are inside the bb, start point is cam pos
	maxT = o + d * tfar;

	return hit;
}

void swap(in float a, in float b){
	float c = a;
	a = b;
	b = c;
}

bool raySphereintersectionSkyMap(vec3 rd, float radius, out vec3 startPos){
	
	float t;

	vec3 sphereCenter_ = vec3(0.0);

	float radius2 = radius*radius;

	vec3 L = - sphereCenter_;
	float a = dot(rd, rd);
	float b = 2.0 * dot(rd, L);
	float c = dot(L,L) - radius2;

	float discr = b*b - 4.0 * a * c;
	t = max(0.0, (-b + sqrt(discr))/2);

	startPos = rd*t;

	return true;
}


bool raySphereintersection(vec3 ro, vec3 rd, float radius, out vec3 startPos){
	
	float t;

	sphereCenter.xz = cameraPosition.xz;

	float radius2 = radius*radius;

	vec3 L = ro - sphereCenter;
	float a = dot(rd, rd);
	float b = 2.0 * dot(rd, L);
	float c = dot(L,L) - radius2;

	float discr = b*b - 4.0 * a * c;
	if(discr < 0.0) return false;
	t = max(0.0, (-b + sqrt(discr))/2);
	if(t == 0.0){
		return false;
	}
	startPos = ro + rd*t;

	return true;
}

bool raySphereintersection2(vec3 ro, vec3 rd, float radius, out vec3 startPos){
	
	float t;

	sphereCenter.xz = cameraPosition.xz;

	float radius2 = radius*radius;

	vec3 L = ro - sphereCenter;
	float a = dot(rd, rd);
	float b = 2.0 * dot(rd, L);
	float c = dot(L,L) - radius2;

	float discr = b*b - 4.0 * a * c;
	if(discr < 0.0) return false;
	t = max(0.0, (-b + sqrt(discr))/2);
	if(t == 0.0){
		return false;
	}
	startPos = ro + rd*t;

	return true;
}

uniform vec3 skyColorBottom;
uniform vec3 skyColorTop;

vec3 getSun(const vec3 d, float powExp){
	float sun = clamp( dot(SUN_DIR,d), 0.0, 1.0 );
	vec3 col = 0.8*vec3(1.0,.6,0.1)*pow( sun, powExp );
	return col;
}

float Random2D(in vec3 st)
{
	return fract(sin(iTime*dot(st.xyz, vec3(12.9898, 78.233, 57.152))) * 43758.5453123);
}

 float getHeightFraction(vec3 inPos){
	return (length(inPos - sphereCenter) - SPHERE_INNER_RADIUS)/(SPHERE_OUTER_RADIUS - SPHERE_INNER_RADIUS);
 }


 float remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax)
{
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

float getDensityForCloud(float heightFraction, float cloudType)
{
	float stratusFactor = 1.0 - clamp(cloudType * 2.0, 0.0, 1.0);
	float stratoCumulusFactor = 1.0 - abs(cloudType - 0.5) * 2.0;
	float cumulusFactor = clamp(cloudType - 0.5, 0.0, 1.0) * 2.0;

	vec4 baseGradient = stratusFactor * STRATUS_GRADIENT + stratoCumulusFactor * STRATOCUMULUS_GRADIENT + cumulusFactor * CUMULUS_GRADIENT;

	// gradicent computation (see Siggraph 2017 Nubis-Decima talk)
	//return remap(heightFraction, baseGradient.x, baseGradient.y, 0.0, 1.0) * remap(heightFraction, baseGradient.z, baseGradient.w, 1.0, 0.0);
	return smoothstep(baseGradient.x, baseGradient.y, heightFraction) - smoothstep(baseGradient.z, baseGradient.w, heightFraction);

}

float threshold(const float v, const float t)
{
	return v > t ? v : 0.0;
}

const vec3 windDirection = normalize(vec3(0.5, 0.0, 0.1));

vec2 getUVProjection(vec3 p){
	return p.xz/SPHERE_INNER_RADIUS + 0.5;
}

#define CLOUD_TOP_OFFSET 750.0
#define SATURATE(x) clamp(x, 0.0, 1.0)
#define CLOUD_SCALE crispiness
#define CLOUD_SPEED cloudSpeed

uniform float curliness;

float sampleCloudDensity(vec3 p, bool expensive, float lod){

	float heightFraction = getHeightFraction(p);
	vec3 animation = heightFraction * windDirection * CLOUD_TOP_OFFSET + windDirection * iTime * CLOUD_SPEED;
	vec2 uv = getUVProjection(p);
	vec2 moving_uv = getUVProjection(p + animation);


	if(heightFraction < 0.0 || heightFraction > 1.0){
		return 0.0;
	}

	vec4 low_frequency_noise = textureLod(cloud, vec3(uv*CLOUD_SCALE, heightFraction), lod);
	float lowFreqFBM = dot(low_frequency_noise.gba, vec3(0.625, 0.25, 0.125));
	float base_cloud = remap(low_frequency_noise.r, -(1.0 - lowFreqFBM), 1., 0.0 , 1.0);
	
	float density = getDensityForCloud(heightFraction, 1.0);
	base_cloud *= (density/heightFraction);

	vec3 weather_data = texture(weatherTex, moving_uv).rgb;
	float cloud_coverage = weather_data.r*coverage_multiplier;
	float base_cloud_with_coverage = remap(base_cloud , cloud_coverage , 1.0 , 0.0 , 1.0);
	base_cloud_with_coverage *= cloud_coverage;

	//bool expensive = true;
	
	if(expensive)
	{
		vec3 erodeCloudNoise = textureLod(worley32, vec3(moving_uv*CLOUD_SCALE, heightFraction)*curliness, lod).rgb;
		float highFreqFBM = dot(erodeCloudNoise.rgb, vec3(0.625, 0.25, 0.125));//(erodeCloudNoise.r * 0.625) + (erodeCloudNoise.g * 0.25) + (erodeCloudNoise.b * 0.125);
		float highFreqNoiseModifier = mix(highFreqFBM, 1.0 - highFreqFBM, clamp(heightFraction * 10.0, 0.0, 1.0));

		base_cloud_with_coverage = base_cloud_with_coverage - highFreqNoiseModifier * (1.0 - base_cloud_with_coverage);

		base_cloud_with_coverage = remap(base_cloud_with_coverage*2.0, highFreqNoiseModifier * 0.2, 1.0, 0.0, 1.0);
	}

	return clamp(base_cloud_with_coverage, 0.0, 1.0);
}


float beer(float d){
	return exp(-d);
}

float powder(float d){
	return (1. - exp(-2.*d));

}


 float phase(vec3 inLightVec, vec3 inViewVec, float g) {
	float costheta = dot(inLightVec, inViewVec) / length(inLightVec) / length(inViewVec);
	return HG(costheta, g);
}

 vec3 eye = cameraPosition;

uniform float absorption = 0.0035;

float raymarchToLight(vec3 o, float stepSize, vec3 lightDir, float originalDensity, float lightDotEye)
{

	vec3 startPos = o;
	float ds = stepSize * 6.0;
	vec3 rayStep = lightDir * ds;
	const float CONE_STEP = 1.0/6.0;
	float coneRadius = 1.0; 
	float density = 0.0;
	float coneDensity = 0.0;
	float invDepth = 1.0/ds;
	float sigma_ds = -ds*absorption;
	vec3 pos;

	float T = 1.0;

	for(int i = 0; i < 6; i++)
	{
		pos = startPos + coneRadius*noiseKernel[i]*float(i);

		float heightFraction = getHeightFraction(pos);
		if(heightFraction >= 0)
		{
			
			float cloudDensity = sampleCloudDensity(pos, density > 0.3, i/16);
			if(cloudDensity > 0.0)
			{
				float Ti = exp(cloudDensity*sigma_ds);
				T *= Ti;
				density += cloudDensity;
			}
		}
		startPos += rayStep;
		coneRadius += CONE_STEP;
	}

	//return 2.0*T*powder((originalDensity));//*powder(originalDensity, 0.0);
	return T;
}

vec3 ambientlight = vec3(255, 255, 235)/255;

float ambientFactor = 0.5;
vec3 lc = ambientlight * ambientFactor;// * cloud_bright;

vec3 ambient_light(float heightFrac)
{
	return mix( vec3(0.5, 0.67, 0.82), vec3(1.0), heightFrac);
}
      

#define BAYER_FACTOR 1.0/16.0
uniform float bayerFilter[16u] = float[]
(
	0.0*BAYER_FACTOR, 8.0*BAYER_FACTOR, 2.0*BAYER_FACTOR, 10.0*BAYER_FACTOR,
	12.0*BAYER_FACTOR, 4.0*BAYER_FACTOR, 14.0*BAYER_FACTOR, 6.0*BAYER_FACTOR,
	3.0*BAYER_FACTOR, 11.0*BAYER_FACTOR, 1.0*BAYER_FACTOR, 9.0*BAYER_FACTOR,
	15.0*BAYER_FACTOR, 7.0*BAYER_FACTOR, 13.0*BAYER_FACTOR, 5.0*BAYER_FACTOR
);

uniform bool enablePowder = false;

uniform float densityFactor = 0.02;
vec4 raymarchToCloud(vec3 startPos, vec3 endPos, vec3 bg, out vec4 cloudPos){
	vec3 path = endPos - startPos;
	float len = length(path);

	//float maxLen = length(planeDim);

	//float volumeHeight = planeMax.y - planeMin.y;

	const int nSteps = 64;//int(mix(48.0, 96.0, clamp( len/SPHERE_DELTA - 1.0,0.0,1.0) ));
	
	float ds = len/nSteps;
	vec3 dir = path/len;
	dir *= ds;
	vec4 col = vec4(0.0);
	uvec2 fragCoord = gl_GlobalInvocationID.xy;
	int a = int(fragCoord.x) % 4;
	int b = int(fragCoord.y) % 4;
	startPos += dir * bayerFilter[a * 4 + b];
	//startPos += dir*abs(Random2D(vec3(a,b,a+b)))*.5;
	vec3 pos = startPos;

	float density = 0.0;

	float lightDotEye = dot(normalize(SUN_DIR), normalize(dir));

	float T = 1.0;
	float sigma_ds = -ds*densityFactor;
	bool expensive = true;
	bool entered = false;

	int zero_density_sample = 0;

	for(int i = 0; i < nSteps; ++i)
	{	
		//if( pos.y >= cameraPosition.y - SPHERE_DELTA*1.5 ){

		float density_sample = sampleCloudDensity(pos, true, i/16);
		if(density_sample > 0.)
		{
			if(!entered){
				cloudPos = vec4(pos,1.0);
				entered = true;	
			}
			float height = getHeightFraction(pos);
			vec3 ambientLight = CLOUDS_AMBIENT_COLOR_BOTTOM; //mix( CLOUDS_AMBIENT_COLOR_BOTTOM, CLOUDS_AMBIENT_COLOR_TOP, height );
			float light_density = raymarchToLight(pos, ds*0.1, SUN_DIR, density_sample, lightDotEye);
			float scattering = mix(HG(lightDotEye, -0.08), HG(lightDotEye, 0.08), clamp(lightDotEye*0.5 + 0.5, 0.0, 1.0));
			//scattering = 0.6;
			scattering = max(scattering, 1.0);
			float powderTerm =  powder(density_sample);
			if(!enablePowder)
				powderTerm = 1.0;
			
			vec3 S = 0.6*( mix( mix(ambientLight*1.8, bg, 0.2), scattering*SUN_COLOR, powderTerm*light_density)) * density_sample;
			float dTrans = exp(density_sample*sigma_ds);
			vec3 Sint = (S - S * dTrans) * (1. / density_sample);
			col.rgb += T * Sint;
			T *= dTrans;

		}

		if( T <= CLOUDS_MIN_TRANSMITTANCE ) break;

		pos += dir;
		//}
	}
	//col.rgb += ambientlight*0.02;
	col.a = 1.0 - T;
	
	//col = vec4( vec3(getHeightFraction(startPos)), 1.0);

	return col;
}

vec3 computeClipSpaceCoord(uvec2 fragCoord){
	vec2 ray_nds = 2.0*vec2(fragCoord.xy)/iResolution.xy - 1.0;
	return vec3(ray_nds, 1.0);
}

vec2 computeScreenPos(vec2 ndc){
	return (ndc*0.5 + 0.5);
}

float computeFogAmount(in vec3 startPos, in float factor){
	float dist = length(startPos - cameraPosition);
	float radius = (cameraPosition.y - sphereCenter.y) * 0.3;
	float alpha = (dist / radius);
	//v.rgb = mix(v.rgb, ambientColor, alpha*alpha);

	return (1.-exp( -dist*alpha*factor));
}

#define HDR(col, exps) 1.0 - exp(-col * exps)

void main()
{
	vec4 fragColor_v, bloom_v, alphaness_v, cloudDistance_v;
	ivec2 fragCoord = ivec2(gl_GlobalInvocationID.xy);

	//if(texture(depthMap, TexCoords).r < 1.0)
	//compute ray direction
	vec4 ray_clip = vec4(computeClipSpaceCoord(fragCoord), 1.0);
	vec4 ray_view = inv_proj * ray_clip;
	ray_view = vec4(ray_view.xy, -1.0, 0.0);
	vec3 worldDir = (inv_view * ray_view).xyz;
	worldDir = normalize(worldDir);

	vec3 startPos, endPos;
	vec4 v = vec4(0.0);

	//compute background color
	vec3 stub, cubeMapEndPos;
	//intersectCubeMap(vec3(0.0, 0.0, 0.0), worldDir, stub, cubeMapEndPos);
	bool hit = raySphereintersectionSkyMap(worldDir, 0.5, cubeMapEndPos);

	vec4 bg = texture(sky, fragCoord/iResolution);
	vec3 red = vec3(1.0);

	bg = mix( mix(red.rgbr, vec4(1.0), SUN_DIR.y), bg, pow( max(cubeMapEndPos.y+0.1, .0), 0.2));
	//vec4 bg = vec4( TonemapACES(preetham(worldDir)), 1.0);
	int case_ = 0;
	//compute raymarching starting and ending point
	vec3 fogRay;
	if(cameraPosition.y < SPHERE_INNER_RADIUS - EARTH_RADIUS){
		raySphereintersection(cameraPosition, worldDir, SPHERE_INNER_RADIUS, startPos);
		raySphereintersection(cameraPosition, worldDir, SPHERE_OUTER_RADIUS, endPos);
		fogRay = startPos;
	}else if(cameraPosition.y > SPHERE_INNER_RADIUS - EARTH_RADIUS && cameraPosition.y < SPHERE_OUTER_RADIUS - EARTH_RADIUS){
		startPos = cameraPosition;
		raySphereintersection(cameraPosition, worldDir, SPHERE_OUTER_RADIUS, endPos);
		bool hit = raySphereintersection(cameraPosition, worldDir, SPHERE_INNER_RADIUS, fogRay);
		if(!hit)
			fogRay = startPos;
		case_ = 1;
	}else{
		raySphereintersection2(cameraPosition, worldDir, SPHERE_OUTER_RADIUS, startPos);
		raySphereintersection2(cameraPosition, worldDir, SPHERE_INNER_RADIUS, endPos);
		raySphereintersection(cameraPosition, worldDir, SPHERE_OUTER_RADIUS, fogRay);
		case_ = 2;
	}

	//compute fog amount and early exit if over a certain value
	float fogAmount = computeFogAmount(fogRay, 0.00006);

	fragColor_v = bg;
	bloom_v = vec4(getSun(worldDir, 128)*1.3,1.0);

	if(fogAmount > 0.965){
		fragColor_v = bg;
		bloom_v = bg;
		imageStore(fragColor, fragCoord, fragColor_v);
		imageStore(bloom, fragCoord, bloom_v);
		imageStore(alphaness, fragCoord, vec4(0.0));
		imageStore(cloudDistance, fragCoord, vec4(-1.0)); 
		return; //early exit
	}

	v = raymarchToCloud(startPos,endPos, bg.rgb, cloudDistance_v);
	cloudDistance_v = vec4(distance(cameraPosition, cloudDistance_v.xyz), 0.0,0.0,0.0);
	//cloudDistance_v = v;

	float cloudAlphaness = threshold(v.a, 0.2);
	v.rgb = v.rgb*1.8 - 0.1; // contrast-illumination tuning

	// apply atmospheric fog to far away clouds
	vec3 ambientColor = bg.rgb;

	// use current position distance to center as action radius
    v.rgb = mix(v.rgb, bg.rgb*v.a, clamp(fogAmount,0.,1.));

	// add sun glare to clouds
	float sun = clamp( dot(SUN_DIR,normalize(endPos - startPos)), 0.0, 1.0 );
	vec3 s = 0.8*vec3(1.0,0.4,0.2)*pow( sun, 256.0 );
	v.rgb += s*v.a;

	// blend clouds and background

	bg.rgb = bg.rgb*(1.0 - v.a) + v.rgb;
	bg.a = 1.0;


	fragColor_v = bg;
	alphaness_v = vec4(cloudAlphaness, 0.0, 0.0, 1.0); // alphaness buffer

	if(cloudAlphaness > 0.1){ //apply fog to bloom buffer
		float fogAmount = computeFogAmount(startPos, 0.00003);

		vec3 cloud = mix(vec3(0.0), bloom_v.rgb, clamp(fogAmount,0.,1.));
		bloom_v.rgb = bloom_v.rgb*(1.0 - cloudAlphaness) + cloud.rgb;

	}
	fragColor_v.a = alphaness_v.r;
	imageStore(fragColor, fragCoord, fragColor_v);
	imageStore(bloom, fragCoord, bloom_v);
	imageStore(alphaness, fragCoord, alphaness_v);
	imageStore(cloudDistance, fragCoord, cloudDistance_v);
}