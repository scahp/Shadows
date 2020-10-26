#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
//layout(location = 1) in vec4 Color;

uniform mat4 MVP;

uniform mat4 World2NDC;
uniform vec4 WaterTint;
uniform vec4 Frequency;
uniform vec4 Phase;
uniform vec4 Amplitude;
uniform vec4 DirX;
uniform vec4 DirY;
uniform vec4 SpecAtten;
uniform vec4 CameraPos;
uniform vec4 EnvAdjust;
uniform vec4 EnvTint;
uniform mat4 Local2World;
uniform vec4 Lengths;
uniform vec4 DepthOffset;
uniform vec4 DepthScale;
uniform vec4 FogParams;
uniform vec4 DirXK;
uniform vec4 DirYK;
uniform vec4 DirXW;
uniform vec4 DirYW;
uniform vec4 KW;
uniform vec4 DirXSqKW;
uniform vec4 DirYSqKW;
uniform vec4 DirXdirYKW;

out vec4 ModColor_;
out vec4 AddColor_;
out float Fog;
out vec4 TexCoord0;

out vec4 BTN_X_; // Binormal.x, Tangent.x, Normal.x
out vec4 BTN_Y_; // Bin.y, Tan.y, Norm.y
out vec4 BTN_Z_; // Bin.z, Tan.z, Norm.z


// Depth filter channels control:
// dFilter.x => overall opacity
// dFilter.y => reflection strength
// dFilter.z => wave height
vec3 CalcDepthFilter(vec4 depthOffset, vec4 depthScale, vec4 wPos)
{
	// ���� ���̿� ���ý��� ��ġ ���̸� ����.
	vec3 dFilter = vec3(depthOffset.xyz) - wPos.zzz;

	// �� ���̿��� �������� Scale ���� ������. (�⺻���� [0.5, 0.5, 0.5]��)
	dFilter = dFilter * vec3(depthScale.xyz);

	// Clamp(dFilter, 0.0, 1.0)
	dFilter = max(dFilter, 0.f);
	dFilter = min(dFilter, 1.f);

	return dFilter;
}

void CalcSinCos(vec4 wPos,
	vec4 dirX,
	vec4 dirY,
	vec4 amplitude,
	vec4 frequency,
	vec4 phase,
	vec4 lengths,
	float ooEdgeLength,
	float scale,
	out vec4 sines, out vec4 cosines)
{
	// sin(dot(Di, xy) * frequency + time * phasecontant)

	// Dot x and y with direction vectors
	// 1. dot(Di, xy)
	vec4 dists = dirX * wPos.xxxx;
	dists = dirY * wPos.yyyy + dists;

	// Scale in our frequency and add in our phase
	// 2. dot(Di, xy) * frequency * phase
	dists = dists * frequency;
	dists = dists + phase;

	const float kPi = 3.14159265f;
	const float kTwoPi = 2.f * kPi;
	const float kOOTwoPi = 1.f / kTwoPi;
	// Mod into range [-Pi..Pi]
	// ������ -pi ~ pi �� ���� �Ͽ� ��.
	dists = dists + kPi;
	dists = dists * kOOTwoPi;
	dists = fract(dists);
	dists = dists * kTwoPi;
	dists = dists - kPi;

	// 3. cos, sin ���ϴ� ���̸�, -pi~pi ���̿� ������ ������ sin cos �� ������
	// [
	vec4 dists2 = dists * dists;
	vec4 dists3 = dists2 * dists;
	vec4 dists4 = dists2 * dists2;
	vec4 dists5 = dists3 * dists2;
	vec4 dists6 = dists3 * dists3;
	vec4 dists7 = dists4 * dists3;

	const vec4 kSinConsts = vec4(1.f, -1.f / 6.f, 1.f / 120.f, -1.f / 5040.f);
	const vec4 kCosConsts = vec4(1.f, -1.f / 2.f, 1.f / 24.f, -1.f / 720.f);
	sines = dists + dists3 * kSinConsts.yyyy + dists5 * kSinConsts.zzzz + dists7 * kSinConsts.wwww;
	cosines = kCosConsts.xxxx + dists2 * kCosConsts.yyyy + dists4 * kCosConsts.zzzz + dists6 * kCosConsts.wwww;
	// ]

	// 4. FilteredAmp ����
	// sin * clamp(lengths * ooEdgeLength) * scale * amp
	// cos * clamp(lengths * ooEdgeLength) * scale * amp
	vec4 filteredAmp = lengths * ooEdgeLength;
	filteredAmp = max(filteredAmp, 0.f);
	filteredAmp = min(filteredAmp, 1.f);
	filteredAmp = filteredAmp * scale;
	filteredAmp = filteredAmp * amplitude;

	sines = sines * filteredAmp;
	cosines = cosines * filteredAmp * scale;
}

vec3 FinitizeEyeRay(vec3 cam2Vtx, vec4 envAdjust)
{
	// Compute our finitized eyeray.

	// Our "finitized" eyeray is:
	//	camPos + D * t - envCenter = D * t - (envCenter - camPos)
	// with
	//	D = (pos - camPos) / |pos - camPos| // normalized usual eyeray
	// and
	//	t = D dot F + sqrt( (D dot F)^2 - G )
	// with
	//	F = (envCenter - camPos)	=> envAdjust.xyz
	//	G = F^2 - R^2				=> nevAdjust.w
	// where R is the sphere radius.
	//
	// This all derives from the positive root of equation
	//	(camPos + (pos - camPos) * t - envCenter)^2 = R^2,
	// In other words, where on a sphere of radius R centered about envCenter
	// does the ray from the real camera position through this point hit.
	//
	// Note that F and G are both constants (one 3-point, one scalar).
	float dDotF = dot(cam2Vtx, vec3(envAdjust.xyz));
	float t = dDotF + sqrt(dDotF * dDotF - envAdjust.w);
	return cam2Vtx * t - vec3(envAdjust.xyz);				// A - C

}

void CalcScreenPosAndFog(mat4 world2NDC, vec4 fogParams, vec4 wPos, out vec4 scrPos, out float fog)
{
	// Calc screen position and fog from screen W
	// Fog is basic linear from start distance to end distance.
	vec4 sPos = world2NDC * wPos;
	fog = (sPos.w + fogParams.x) * fogParams.y;
	scrPos = sPos;
}

void CalcFinalColors(vec3 norm,
	vec3 cam2Vtx,
	float opacMin,
	float opacScale,
	float colorFilter,
	float opacFilter,
	vec4 tint,
	out vec4 modColor,
	out vec4 addColor)
{
	// Calculate colors
	// Final color will be
	// rgb = Color1.rgb + Color0.rgb * envMap.rgb
	// alpha = Color0.a

	// Color 0

	// Vertex based Fresnel-esque effect.
	// Input vertex color.b limits how much we attenuate based on angle.
	// So we map 
	// ���ý� ��� ������ ȿ��
	// �Է� ���ý� color.b �� ������ ������� ���ⷮ�� �����մϴ�.
	// (dot(norm,cam2Vtx)==0) => 1 for grazing angle
	// and (dot(norm,cam2Vtx)==1 => 1-In.Color.b for perpendicular view.
	float atten = 1.0 + dot(norm, cam2Vtx) * opacMin;

	// Filter the color based on depth
	// ���� ������� �÷��� ���͸� �϶�.
	modColor.rgb = vec3(colorFilter * atten);

	// Boost the alpha so the reflections fade out faster than the tint
	// and apply the input attenuation factors.
	// ���ĸ� �ν����϶� �׷��� ���÷����� ƾƮ ���� �� ������ ���̵� �ƿ��ϰ� �Է� ���� ��Ҹ� �����Ѵ�.
	modColor.a = (atten + 1.0) * 0.5 * opacFilter * opacScale * tint.a;

	// Color 1 is just a constant.
	addColor = tint;
}

void CalcEyeRayAndBumpAttenuation(vec4 wPos,
	vec4 cameraPos,
	vec4 specAtten,
	out vec3 cam2Vtx,
	out float pertAtten)
{
	// ī�޶󿡼� ���ý��� ���� ������� ����ȭ�� ���Ϳ� ������ �Ÿ��� ������.
	// Get normalized vec from camera to vertex, saving original distance.
	cam2Vtx = vec3(wPos.xyz) - vec3(cameraPos.xyz);
	pertAtten = length(cam2Vtx);
	cam2Vtx = cam2Vtx / pertAtten;

	// �츮�� ����� ���� ��ȭ�� ���� ����϶�. �� ����� �ַ� �ٸ���̰� �ο�� ���ؼ�, 
	// ���� ������ ���� �ʿ��� ���� ����� ���� ���� ��ҿ� ����˴ϴ�.
	// �̰��� ��ָʿ��� ���� �÷��� �����Ű�� �ʽ��ϴ�. �̰��� "bumps"�� �����ŵ�ϴ�
	// Calculate our normal perturbation attenuation. This attenuation will be
	// applied to the horizontal components of the normal read from the computed
	// ripple bump map, mostly to fight aliasing. This doesn't attenuate the 
	// color computed from the normal map, it attenuates the "bumps".

	// a = clamp((dist + specAtten.x) * specAtten.y, 0.0, 1.0)
	// a * a * a * specAtten.z
	pertAtten = pertAtten + specAtten.x;
	pertAtten = pertAtten * specAtten.y;
	pertAtten = min(pertAtten, 1.f);
	pertAtten = max(pertAtten, 0.f);
	pertAtten = pertAtten * pertAtten; // Square it to account for perspective.
	pertAtten = pertAtten * specAtten.z;
}

// See Pos in CalcTangentBasis comments.
vec4 CalcFinalPosition(vec4 wPos,
	vec4 sines,
	vec4 cosines,
	float depthOffset,
	vec4 dirXK,
	vec4 dirYK)
{
	// Equation 9 ����.
	// P(x, y, z)�� ���غ���.

	// Height

	// Sum to a scalar
	// 4���� �ĵ��� �������� �� ���� �� �ִ� �ִ� height ���� h�� ��.
	float h = dot(sines, vec4(1.f)) + depthOffset;

	// Clamp to never go beneath input height
	wPos.z = max(wPos.z, h);

	// dirXK �� �����ϱ�?
	// DirX * K �� �� ���� ��Ÿ��. Qi = Q/(wi * Ai x numWaves) (Equation 9�� �Ʒ� ����)
	// dot(cosines, dirXK)�� dirX * Qi * Ai cos �� ��.
	wPos.x = wPos.x + dot(cosines, dirXK);
	wPos.y = wPos.y + dot(cosines, dirYK);

	// Z�� Y�� �ٲ� �׽�Ʈ
	wPos.yz = wPos.zy;
	wPos.y = -wPos.y;

	return wPos;
}

void CalcTangentBasis(vec4 sines,
	vec4 cosines,
	vec4 dirXSqKW,
	vec4 dirXDirYKW,
	vec4 dirYSqKW,
	vec4 dirXW,
	vec4 dirYW,
	vec4 KW,
	float pertAtten,
	vec3 eyeRay,
	out vec4 BTN_X,
	out vec4 BTN_Y,
	out vec4 BTN_Z,
	out vec3 norm)
{
	// Equation 10, 11, 12 �� TBN ���� ���ϴ� �׸�
	// Note that we're swapping Y and Z and negating Z (rotation about X)
	// to match the D3D convention of Y being up in cubemaps.

	BTN_X.x = 1.f + dot(sines, -dirXSqKW);
	BTN_X.y = dot(sines, -dirXDirYKW);
	BTN_X.z = dot(cosines, -dirXW);
	BTN_X.xy = BTN_X.xy * pertAtten;
	norm.x = BTN_X.z;

	BTN_Z.x = dot(sines, -dirXDirYKW);
	BTN_Z.y = 1.f + dot(sines, -dirYSqKW);
	BTN_Z.z = dot(cosines, -dirYW);
	BTN_Z.xy = BTN_Z.xy * pertAtten;
	norm.y = BTN_Z.z;

	BTN_Y.x = -dot(cosines, dirXW);
	BTN_Y.y = -dot(cosines, dirYW);
	BTN_Y.z = -(1.f + dot(sines, -KW));
	BTN_Y.xy = BTN_Y.xy * pertAtten;
	norm.z = -BTN_Y.z;


	BTN_X.w = eyeRay.x;
	BTN_Y.w = -eyeRay.y;
	BTN_Z.w = eyeRay.z;
}


void main()
{
    ////////////////////////////

	// Evaluate world space base position. All subsequent calculations in world space.
	// 1. ���� �������� ������Ű�� ���� ����� ��� ����������� ��� ��.
	vec4 wPos = Local2World * vec4(Pos, 1.0);

	// Calculate ripple UV from position
	// ??? SpecAtten.w �� RippleScale ��.
	TexCoord0.xy = wPos.xy * SpecAtten.ww;
	TexCoord0.z = 0.f;
	TexCoord0.w = 1.f;

	// Get our depth based filters. 
	// cDepthOffset,
	// cDepthScale,
	// vec4 depthOffset, vec4 depthScale, vec4 wPos, float waterLevel
	// ���� ������� ���͸� ���س���. 0~1 ���� ������ ��
	// [overall opacity, reflection strength, wave height]
	vec3 dFilter = CalcDepthFilter(DepthOffset, DepthScale, wPos);

	// Build our 4 waves

	vec4 sines;
	vec4 cosines;

	vec4 Color = vec4(1.0);

	// Amp * Cos, Amp * Sin ���ϴ� ��.
	CalcSinCos(wPos,
		DirX, DirY,
		Amplitude, Frequency, Phase,
		Lengths, Color.a, 
		dFilter.z,		// dFilter.z => wave height
		sines, cosines);

	// Equation 9 ���ϱ�
	wPos = CalcFinalPosition(wPos, sines, cosines, DepthOffset.w, DirXK, DirYK);

	// ���� ��ġ�� ������. �츮�� ī�޶󿡼� ���ý����� ����ȭ�� ���Ͱ� ������ �ʿ��մϴ�. �׷��� ���� ����ϴ�.
	// We have our final position. We'll be needing normalized vector from camera 
	// to vertex several times, so we go ahead and grab it.
	vec3 cam2Vtx;
	float pertAtten;
	CalcEyeRayAndBumpAttenuation(wPos, CameraPos, SpecAtten, cam2Vtx, pertAtten);

	// Compute our finitized eyeray.
	// ȯ����� ���� ��ġ�� �°� �����ϴ� �ڵ�. Eye Vector ��Ʈ ���� ��.
	vec3 eyeRay = FinitizeEyeRay(cam2Vtx, EnvAdjust);

	// Equation 10 ���ϱ�
	vec3 norm;
	CalcTangentBasis(sines, cosines,
		DirXSqKW,
		DirXdirYKW,
		DirYSqKW,
		DirXW,
		DirYW,
		KW,
		pertAtten,
		eyeRay,
		BTN_X_,
		BTN_Y_,
		BTN_Z_,
		norm);

	vec4 Position;

	// Calc screen position and fog
	// Screen ��ġ�� Fog (NDC�� Z ����, W�� ������ ���̱� �����̹Ƿ� Z�� ������� ����)
	CalcScreenPosAndFog(World2NDC, FogParams, wPos, Position, Fog);

	CalcFinalColors(norm,
		cam2Vtx,
		Color.b,
		Color.r,
		dFilter.y,		// dFilter.y => reflection strength
		dFilter.x,		// dFilter.x => overall opacity
		WaterTint,
		ModColor_,
		AddColor_);

    gl_Position = MVP * vec4(Pos, 1.0);
	gl_Position = Position;
}