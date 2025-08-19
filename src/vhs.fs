#version 330
// https://www.shadertoy.com/view/MdffD7
// Fork of FMS_Cat's VCR distortion shader


// TODO: Add uniforms for tape crease discoloration and image jiggle

uniform sampler2D screen_texture;

uniform vec2 vhs_resolution = vec2(320.0, 240.0);

uniform vec3 rgb_shift = vec3(0.0, 0.003, 0.0);

// Basic shader inputs
uniform float time;
in vec2 fragTexCoord;
#define PI 3.1415926535897932384626433832795
out vec4 COLOR;

uniform int samples = 2;

// Tape crease uniforms
uniform float tape_crease_smear = 1.0;
uniform float tape_crease_jitter = 0.10;
uniform float tape_crease_speed = 0.5;
uniform float tape_crease_discoloration = 1.0;

// Noise uniforms
//uniform vec2 blur_direction = vec2(1.0, 0.0); // Flou horizontal par défaut
//uniform float blur_intensity = 1.0; // Intensité du flou

vec3 apply_directional_blur(vec2 uv, sampler2D tex, vec2 direction, float intensity) {
	vec3 color = vec3(0.0);
	float total_weight = 0.0;
	for (int i = -samples; i <= samples; i++) {
		float weight = 1.0 - abs(float(i)) / float(samples); // Poids dégressif
		vec2 offset = direction * float(i) * intensity / vhs_resolution;
		color += texture(tex, uv + offset).rgb * weight;
		total_weight += weight;
	}
	return color / total_weight;
}


float v2random(vec2 uv) {
	//return 0.5;// texture(noise_texture, mod(uv, vec2(1.0))).x;
	return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
	vec2 uvn = fragTexCoord;
	vec3 col = vec3(0.0, 0.0, 0.0);

	// Tape wave.
	uvn.x += (v2random(vec2(uvn.y / 10.0, time / 10.0) / 1.0) - 0.5) / vhs_resolution.x * 1.0;
	uvn.x += (v2random(vec2(uvn.y, time * 10.0)) - 0.5) / vhs_resolution.x * 1.0;
	// tape crease
	float tc_phase = smoothstep(0.9, 0.96, sin(uvn.y * 8.0 - (time * tape_crease_speed + tape_crease_jitter * v2random(time * vec2(0.67, 0.59))) * PI * 1.2));
	float tc_noise = smoothstep(0.3, 1.0, v2random(vec2(uvn.y * 4.77, time)));
	float tc = tc_phase * tc_noise;
	uvn.x = uvn.x - tc / vhs_resolution.x * 8.0 * tape_crease_smear;
	// Décalage RGB
	vec2 red_uv = uvn + vec2(rgb_shift.r, 0.0);
	vec2 green_uv = uvn + vec2(rgb_shift.g, 0.0);
	vec2 blue_uv = uvn + vec2(rgb_shift.b, 0.0);
	vec2 blur_direction = vec2(1.0, 0.0);
	float blur_intensity = 1.0;
	int blur_samples = samples;
	vec3 blurred = vec3(0.0);
	float total_weight = 0.0;
	for (int i = -blur_samples; i <= blur_samples; i++) {
	    float weight = 1.0 - abs(float(i)) / float(blur_samples);
	    vec2 offset = blur_direction * float(i) * blur_intensity / vhs_resolution;
	    vec2 red_uv_blur = uvn + offset + vec2(rgb_shift.r, 0.0);
	    vec2 green_uv_blur = uvn + offset + vec2(rgb_shift.g, 0.0);
	    vec2 blue_uv_blur = uvn + offset + vec2(rgb_shift.b, 0.0);
	    blurred.r += texture(screen_texture, red_uv_blur).r * weight;
	    blurred.g += texture(screen_texture, green_uv_blur).g * weight;
	    blurred.b += texture(screen_texture, blue_uv_blur).b * weight;
	    total_weight += weight;
	}
	blurred /= total_weight;
	COLOR = vec4(blurred, 1.0);
	//COLOR = texture(screen_texture, fragTexCoord);
}

//#version 330
//
//// Input vertex attributes (from vertex shader)
//in vec2 fragTexCoord;
//in vec4 fragColor;
//
//// Input uniform values
//uniform sampler2D texture0;
//uniform vec4 colDiffuse;
//
//// Output fragment color
//out vec4 finalColor;
//
//// NOTE: Add here your custom variables
//
//// NOTE: Render size values must be passed from code
//const float renderWidth = 800;
//const float renderHeight = 450;
//
//uniform float pixelWidth = 5.0;
//uniform float pixelHeight = 5.0;
//
//void main()
//{
//    float dx = pixelWidth*(1.0/renderWidth);
//    float dy = pixelHeight*(1.0/renderHeight);
//
//    vec2 coord = vec2(dx*floor(fragTexCoord.x/dx), dy*floor(fragTexCoord.y/dy));
//
//    vec3 tc = texture(texture0, coord).rgb;
//
//    finalColor = vec4(tc, 1.0);
//}