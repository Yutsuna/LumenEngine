#version 460 core

layout(binding = 0) uniform sampler2D uAccumTex;
layout(std140, binding = 0) uniform UniformBlock0
{
    vec4 uResolutionFrame;
    vec4 uCamPosition;
    vec4 uCamForward;
    vec4 uCamRight;
    vec4 uCamUp;
    vec4 uCameraParams;
};

out vec4 fragColor;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const float INV_PI = 0.31830988618;
const float RAY_EPS = 0.001;
const float FAR_DIST = 1e6;
const int MAX_BOUNCES = 8;
const int RR_START_BOUNCE = 3;
const int MAT_DIFFUSE = 0;
const int MAT_METAL = 1;
const int MAT_EMISSIVE = 2;
const vec3 LIGHT_POS = vec3(2.25, 2.75, 0.85);
const float LIGHT_RADIUS = 0.52;
const vec3 LIGHT_EMISSION = vec3(38.0, 30.0, 21.0);

struct HitRecord
{
    float t;
    vec3 pos;
    vec3 normal;
    vec3 albedo;
    vec3 emission;
    float roughness;
    int matType;
};

uint pcgHash(inout uint state)
{
    uint prev = state * 747796405u + 2891336453u;
    uint word = ((prev >> ((prev >> 28u) + 4u)) ^ prev) * 277803737u;
    state = prev;
    return (word >> 22u) ^ word;
}

uint initRng(uvec2 pixel, uint frame)
{
    uint state = pixel.x + pixel.y * 8192u;
    pcgHash(state);
    state ^= frame * 747796405u + 2891336453u;
    return pcgHash(state);
}

float rand01(inout uint state)
{
    return float(pcgHash(state)) / 4294967296.0;
}

void buildOnb(vec3 n, out vec3 tangent, out vec3 bitangent)
{
    if (n.z < -0.999999)
    {
        tangent = vec3(0.0, -1.0, 0.0);
        bitangent = vec3(-1.0, 0.0, 0.0);
    }
    else
    {
        float a = 1.0 / (1.0 + n.z);
        float b = -n.x * n.y * a;
        tangent = vec3(1.0 - n.x * n.x * a, b, -n.x);
        bitangent = vec3(b, 1.0 - n.y * n.y * a, -n.y);
    }
}

vec3 sampleCosineHemisphere(vec3 normal, inout uint rng)
{
    float r1 = rand01(rng);
    float r2 = rand01(rng);
    float phi = TWO_PI * r1;
    float cosTheta = sqrt(r2);
    float sinTheta = sqrt(max(0.0, 1.0 - r2));
    vec3 t, b;
    buildOnb(normal, t, b);
    return normalize(t * (cos(phi) * sinTheta) + b * (sin(phi) * sinTheta) + normal * cosTheta);
}

vec3 sampleUnitSphere(inout uint rng)
{
    float z = 1.0 - 2.0 * rand01(rng);
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = TWO_PI * rand01(rng);
    return vec3(r * cos(phi), r * sin(phi), z);
}

float intersectSphere(vec3 ro, vec3 rd, vec3 center, float radius)
{
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0) return -1.0;
    float sq = sqrt(disc);
    float t0 = -b - sq;
    if (t0 > RAY_EPS) return t0;
    float t1 = -b + sq;
    if (t1 > RAY_EPS) return t1;
    return -1.0;
}

float intersectGroundPlane(vec3 ro, vec3 rd)
{
    if (abs(rd.y) < 1e-5) return -1.0;
    float t = -ro.y / rd.y;
    return (t > RAY_EPS && t < 2000.0) ? t : -1.0;
}

bool traceScene(vec3 ro, vec3 rd, out HitRecord rec)
{
    rec.t = FAR_DIST;
    bool hit = false;

    float tGround = intersectGroundPlane(ro, rd);
    if (tGround > 0.0 && tGround < rec.t)
    {
        hit = true;
        rec.t = tGround;
        rec.pos = ro + rd * tGround;
        rec.normal = vec3(0.0, 1.0, 0.0);
        float checker = mod(floor(rec.pos.x) + floor(rec.pos.z), 2.0);
        float fade = exp(-0.015 * length(rec.pos.xz));
        vec3 colA = vec3(0.46, 0.48, 0.52);
        vec3 colB = vec3(0.62, 0.63, 0.66);
        rec.albedo = mix(vec3(0.52, 0.53, 0.56), mix(colA, colB, checker), fade);
        rec.emission = vec3(0.0);
        rec.roughness = 1.0;
        rec.matType = MAT_DIFFUSE;
    }

    float tS0 = intersectSphere(ro, rd, vec3(0.0, 1.0, 0.0), 1.0);
    if (tS0 > 0.0 && tS0 < rec.t)
    {
        hit = true;
        rec.t = tS0;
        rec.pos = ro + rd * tS0;
        rec.normal = normalize(rec.pos - vec3(0.0, 1.0, 0.0));
        rec.albedo = vec3(0.92, 0.94, 0.97);
        rec.emission = vec3(0.0);
        rec.roughness = 0.015;
        rec.matType = MAT_METAL;
    }

    float tS1 = intersectSphere(ro, rd, vec3(-2.1, 0.76, 0.42), 0.76);
    if (tS1 > 0.0 && tS1 < rec.t)
    {
        hit = true;
        rec.t = tS1;
        rec.pos = ro + rd * tS1;
        rec.normal = normalize(rec.pos - vec3(-2.1, 0.76, 0.42));
        rec.albedo = vec3(0.82, 0.27, 0.20);
        rec.emission = vec3(0.0);
        rec.roughness = 1.0;
        rec.matType = MAT_DIFFUSE;
    }

    float tS2 = intersectSphere(ro, rd, vec3(2.0, 0.66, -0.35), 0.66);
    if (tS2 > 0.0 && tS2 < rec.t)
    {
        hit = true;
        rec.t = tS2;
        rec.pos = ro + rd * tS2;
        rec.normal = normalize(rec.pos - vec3(2.0, 0.66, -0.35));
        rec.albedo = vec3(0.94, 0.72, 0.34);
        rec.emission = vec3(0.0);
        rec.roughness = 0.14;
        rec.matType = MAT_METAL;
    }

    float tS3 = intersectSphere(ro, rd, vec3(-0.95, 0.46, -1.6), 0.46);
    if (tS3 > 0.0 && tS3 < rec.t)
    {
        hit = true;
        rec.t = tS3;
        rec.pos = ro + rd * tS3;
        rec.normal = normalize(rec.pos - vec3(-0.95, 0.46, -1.6));
        rec.albedo = vec3(0.22, 0.44, 0.76);
        rec.emission = vec3(0.0);
        rec.roughness = 1.0;
        rec.matType = MAT_DIFFUSE;
    }

    float tLight = intersectSphere(ro, rd, LIGHT_POS, LIGHT_RADIUS);
    if (tLight > 0.0 && tLight < rec.t)
    {
        hit = true;
        rec.t = tLight;
        rec.pos = ro + rd * tLight;
        rec.normal = normalize(rec.pos - LIGHT_POS);
        rec.albedo = vec3(0.0);
        rec.emission = LIGHT_EMISSION;
        rec.roughness = 0.0;
        rec.matType = MAT_EMISSIVE;
    }
    return hit;
}

bool isOccludedBefore(vec3 ro, vec3 rd, float maxDist)
{
    float t = intersectGroundPlane(ro, rd);
    if (t > 0.0 && t < maxDist) return true;
    t = intersectSphere(ro, rd, vec3(0.0, 1.0, 0.0), 1.0);
    if (t > 0.0 && t < maxDist) return true;
    t = intersectSphere(ro, rd, vec3(-2.1, 0.76, 0.42), 0.76);
    if (t > 0.0 && t < maxDist) return true;
    t = intersectSphere(ro, rd, vec3(2.0, 0.66, -0.35), 0.66);
    if (t > 0.0 && t < maxDist) return true;
    t = intersectSphere(ro, rd, vec3(-0.95, 0.46, -1.6), 0.46);
    if (t > 0.0 && t < maxDist) return true;
    return false;
}

vec3 evalSky(vec3 rd)
{
    vec3 horizon = vec3(0.54, 0.67, 0.85);
    vec3 zenith = vec3(0.11, 0.24, 0.54);
    vec3 nadir = vec3(0.08, 0.09, 0.12);
    if (rd.y >= 0.0)
    {
        float h = pow(clamp(rd.y, 0.0, 1.0), 0.55);
        return mix(horizon, zenith, h) * 0.38;
    }
    return mix(horizon * 0.34, nadir, clamp(-rd.y * 4.0, 0.0, 1.0));
}

vec3 evalLightAirglow(vec3 ro, vec3 rd, float maxT)
{
    vec3 toCenter = LIGHT_POS - ro;
    float proj = dot(toCenter, rd);
    if (proj <= 0.0 || proj >= maxT) return vec3(0.0);
    vec3 closest = ro + rd * proj;
    float distToCenter = length(closest - LIGHT_POS);
    float edgeDist = max(0.0, distToCenter - LIGHT_RADIUS);
    float tightGlow = exp(-edgeDist * 5.2) * 0.65;
    float wideGlow = exp(-edgeDist * 1.25) * 0.15;
    return vec3(1.0, 0.80, 0.52) * (tightGlow + wideGlow);
}

void main()
{
    uvec2 pixelCoord = uvec2(gl_FragCoord.xy);
    uint frame = uint(uResolutionFrame.z);
    uint rng = initRng(pixelCoord, frame);

    vec2 jitter = vec2(rand01(rng), rand01(rng));
    vec2 uv = (floor(gl_FragCoord.xy) + jitter) / uResolutionFrame.xy;
    vec2 ndc = uv * 2.0 - 1.0;
    float aspect = uResolutionFrame.x / uResolutionFrame.y;
    float tanHalfFov = uCameraParams.x;

    vec3 ro = uCamPosition.xyz;
    vec3 rd = normalize(
        uCamForward.xyz +
        uCamRight.xyz * (ndc.x * aspect * tanHalfFov) +
        uCamUp.xyz * (ndc.y * tanHalfFov)
    );

    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);
    bool lastWasSpecular = true;

    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        HitRecord hit;
        bool didHit = traceScene(ro, rd, hit);
        float segmentDist = didHit ? hit.t : FAR_DIST;

        if (lastWasSpecular && bounce <= 1)
        {
            radiance += throughput * evalLightAirglow(ro, rd, segmentDist);
        }
        if (!didHit)
        {
            radiance += throughput * evalSky(rd);
            break;
        }
        if (hit.matType == MAT_EMISSIVE)
        {
            if (lastWasSpecular)
            {
                float ndotv = clamp(dot(-rd, hit.normal), 0.0, 1.0);
                vec3 rimTint = vec3(7.5, 4.2, 1.8);
                vec3 coreEmission = LIGHT_EMISSION * 1.25;
                vec3 visibleEmission = mix(rimTint, coreEmission, pow(ndotv, 0.42));
                vec3 contrib = throughput * visibleEmission;
                if (bounce >= 2)
                {
                    contrib = min(contrib, vec3(7.5));
                }
                radiance += contrib;
            }
            break;
        }

        if (hit.matType == MAT_DIFFUSE)
        {
            vec3 toLight = LIGHT_POS - hit.pos;
            float dist2 = dot(toLight, toLight);
            float dist = sqrt(dist2);
            if (dist > LIGHT_RADIUS + RAY_EPS)
            {
                vec3 lightAxis = toLight / dist;
                float sinThetaMax2 = clamp((LIGHT_RADIUS * LIGHT_RADIUS) / dist2, 0.0, 1.0);
                float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMax2));
                float u1 = rand01(rng);
                float u2 = rand01(rng);
                float cosAlpha = 1.0 - u1 * (1.0 - cosThetaMax);
                float sinAlpha = sqrt(max(0.0, 1.0 - cosAlpha * cosAlpha));
                float phi = TWO_PI * u2;
                vec3 lt, lb;
                buildOnb(lightAxis, lt, lb);
                vec3 lightDir = normalize(
                    lt * (cos(phi) * sinAlpha) +
                    lb * (sin(phi) * sinAlpha) +
                    lightAxis * cosAlpha
                );
                float ndotl = dot(hit.normal, lightDir);
                if (ndotl > 0.0)
                {
                    vec3 shadowOrigin = hit.pos + hit.normal * RAY_EPS;
                    float tLight = intersectSphere(shadowOrigin, lightDir, LIGHT_POS, LIGHT_RADIUS);
                    if (tLight > 0.0 && !isOccludedBefore(shadowOrigin, lightDir, tLight - RAY_EPS))
                    {
                        float solidAngle = TWO_PI * (1.0 - cosThetaMax);
                        vec3 brdf = hit.albedo * INV_PI;
                        radiance += throughput * LIGHT_EMISSION * brdf * ndotl * solidAngle;
                    }
                }
            }
            ro = hit.pos + hit.normal * RAY_EPS;
            rd = sampleCosineHemisphere(hit.normal, rng);
            throughput *= hit.albedo;
            lastWasSpecular = false;
        }
        else if (hit.matType == MAT_METAL)
        {
            vec3 refl = reflect(rd, hit.normal);
            vec3 scatterDir = normalize(refl + hit.roughness * sampleUnitSphere(rng));
            if (dot(scatterDir, hit.normal) <= 0.0)
            {
                scatterDir = refl;
            }
            ro = hit.pos + hit.normal * RAY_EPS;
            rd = scatterDir;
            throughput *= hit.albedo;
            lastWasSpecular = true;
        }

        if (bounce >= RR_START_BOUNCE)
        {
            float p = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05, 0.95);
            if (rand01(rng) > p)
            {
                break;
            }
            throughput /= p;
        }
    }

    if (any(isnan(radiance)) || any(isinf(radiance)))
    {
        radiance = vec3(0.0);
    }
    radiance = min(radiance, vec3(32.0));

    ivec2 pix = ivec2(gl_FragCoord.xy);
    vec3 prevSum = (frame == 0u) ? vec3(0.0) : texelFetch(uAccumTex, pix, 0).rgb;
    fragColor = vec4(prevSum + radiance, 1.0);
}
