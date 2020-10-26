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
	// 물의 깊이와 버택스의 위치 차이를 구함.
	vec3 dFilter = vec3(depthOffset.xyz) - wPos.zzz;

	// 물 깊이와의 차이점에 Scale 값을 적용함. (기본값은 [0.5, 0.5, 0.5]임)
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
	// 각도를 -pi ~ pi 로 제한 하여 줌.
	dists = dists + kPi;
	dists = dists * kOOTwoPi;
	dists = fract(dists);
	dists = dists * kTwoPi;
	dists = dists - kPi;

	// 3. cos, sin 구하는 식이며, -pi~pi 사이에 공식이 들어오면 sin cos 를 구해줌
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

	// 4. FilteredAmp 적용
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
	// 버택스 기반 프레넬 효과
	// 입력 버택스 color.b 는 각도를 기반으로 감쇄량을 제한합니다.
	// (dot(norm,cam2Vtx)==0) => 1 for grazing angle
	// and (dot(norm,cam2Vtx)==1 => 1-In.Color.b for perpendicular view.
	float atten = 1.0 + dot(norm, cam2Vtx) * opacMin;

	// Filter the color based on depth
	// 깊이 기반으로 컬러를 필터링 하라.
	modColor.rgb = vec3(colorFilter * atten);

	// Boost the alpha so the reflections fade out faster than the tint
	// and apply the input attenuation factors.
	// 알파를 부스터하라 그래서 리플렉션이 틴트 보다 더 빠르게 페이드 아웃하고 입력 감쇄 요소를 적용한다.
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
	// 카메라에서 버택스로 부터 만들어진 정규화된 벡터와 원래의 거리를 저장함.
	// Get normalized vec from camera to vertex, saving original distance.
	cam2Vtx = vec3(wPos.xyz) - vec3(cameraPos.xyz);
	pertAtten = length(cam2Vtx);
	cam2Vtx = cam2Vtx / pertAtten;

	// 우리의 노멀의 작은 변화의 감쇄 계산하라. 이 감쇄는 주로 앨리어싱과 싸우기 위해서, 
	// 계산된 전물결 범프 맵에서 읽은 노멀의 수평 구성 요소에 적용됩니다.
	// 이것은 노멀맵에서 계산된 컬러를 감쇄시키지 않습니다. 이것은 "bumps"를 감쇄시킵니다
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
	// Equation 9 같음.
	// P(x, y, z)를 구해보자.

	// Height

	// Sum to a scalar
	// 4개의 파도가 더해졌을 때 나올 수 있는 최대 height 값을 h로 둠.
	float h = dot(sines, vec4(1.f)) + depthOffset;

	// Clamp to never go beneath input height
	wPos.z = max(wPos.z, h);

	// dirXK 가 무엇일까?
	// DirX * K 는 이 식을 나타냄. Qi = Q/(wi * Ai x numWaves) (Equation 9에 아래 나옴)
	// dot(cosines, dirXK)는 dirX * Qi * Ai cos 가 됨.
	wPos.x = wPos.x + dot(cosines, dirXK);
	wPos.y = wPos.y + dot(cosines, dirYK);

	// Z랑 Y랑 바꿈 테스트
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
	// Equation 10, 11, 12 의 TBN 벡터 구하는 항목
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
	// 1. 월드 공간으로 변형시키고 이후 계산은 모두 월드공간에서 계산 됨.
	vec4 wPos = Local2World * vec4(Pos, 1.0);

	// Calculate ripple UV from position
	// ??? SpecAtten.w 는 RippleScale 임.
	TexCoord0.xy = wPos.xy * SpecAtten.ww;
	TexCoord0.z = 0.f;
	TexCoord0.w = 1.f;

	// Get our depth based filters. 
	// cDepthOffset,
	// cDepthScale,
	// vec4 depthOffset, vec4 depthScale, vec4 wPos, float waterLevel
	// 깊이 기반으로 필터를 구해낸다. 0~1 사이 값으로 얻어냄
	// [overall opacity, reflection strength, wave height]
	vec3 dFilter = CalcDepthFilter(DepthOffset, DepthScale, wPos);

	// Build our 4 waves

	vec4 sines;
	vec4 cosines;

	vec4 Color = vec4(1.0);

	// Amp * Cos, Amp * Sin 구하는 것.
	CalcSinCos(wPos,
		DirX, DirY,
		Amplitude, Frequency, Phase,
		Lengths, Color.a, 
		dFilter.z,		// dFilter.z => wave height
		sines, cosines);

	// Equation 9 구하기
	wPos = CalcFinalPosition(wPos, sines, cosines, DepthOffset.w, DirXK, DirYK);

	// 최종 위치를 가진다. 우리는 카메라에서 버택스로의 정규화된 벡터가 여러번 필요합니다. 그래서 여기 만듭니다.
	// We have our final position. We'll be needing normalized vector from camera 
	// to vertex several times, so we go ahead and grab it.
	vec3 cam2Vtx;
	float pertAtten;
	CalcEyeRayAndBumpAttenuation(wPos, CameraPos, SpecAtten, cam2Vtx, pertAtten);

	// Compute our finitized eyeray.
	// 환경맵을 현재 위치에 맞게 보정하는 코드. Eye Vector 파트 보면 됨.
	vec3 eyeRay = FinitizeEyeRay(cam2Vtx, EnvAdjust);

	// Equation 10 구하기
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
	// Screen 위치와 Fog (NDC의 Z 기준, W로 나누기 전이기 때문이므로 Z가 들어있을 것임)
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