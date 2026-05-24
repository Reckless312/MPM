#version 330 core

flat in int particleID;
in vec3 eyeSpacePos;

uniform mat4 projection;
uniform float sphereRadius;
uniform vec3 lightDirEye;

out vec4 FragmentColor;

const float pi = 3.14159265;
const float ambientStrength = 0.35;
const float specularShininess = 80.0;
const float specularStrength = 0.9;
const float fresnelStrength = 0.4;

const vec3 shadowColor = vec3(0.45, 0.60, 0.80);
const vec3 litColor    = vec3(1.0,  1.0,  1.0);

// Hex prism SDF: hex cross-section in XZ, extends along Y.
// r = hex radius, h = half-height.
float sdHexPrism(vec3 p, float r, float h)
{
    const vec3 k = vec3(-0.8660254, 0.5, 0.5773503);
    vec2 q = abs(p.xz);
    q -= 2.0 * min(dot(k.xy, q), 0.0) * k.xy;
    vec2 d = vec2(
        length(q - vec2(clamp(q.x, -k.z * r, k.z * r), r)) * sign(q.y - r),
        abs(p.y) - h
    );
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

vec3 hexNormal(vec3 p, float r, float h)
{
    float e = 0.001 * r;
    return normalize(vec3(
        sdHexPrism(p + vec3(e,0,0), r, h) - sdHexPrism(p - vec3(e,0,0), r, h),
        sdHexPrism(p + vec3(0,e,0), r, h) - sdHexPrism(p - vec3(0,e,0), r, h),
        sdHexPrism(p + vec3(0,0,e), r, h) - sdHexPrism(p - vec3(0,0,e), r, h)
    ));
}

void main()
{
    vec2 offset = gl_PointCoord * 2.0 - 1.0;
    if (dot(offset, offset) > 1.0) discard;

    // Per-particle random orientation: Y-rotation + tilt via two golden-ratio hashes.
    float angle = fract(float(particleID) * 0.618034) * 2.0 * pi;
    float tilt  = fract(float(particleID) * 0.381966) * pi;
    float cosA = cos(angle); float sinA = sin(angle);
    float cosT = cos(tilt);  float sinT = sin(tilt);

    float hexR = sphereRadius * 0.75;
    float hexH = sphereRadius * 0.22;

    // Ray in particle-local eye space (particle center at origin).
    vec3 ro = -eyeSpacePos;
    vec3 rd = normalize(eyeSpacePos + vec3(offset * sphereRadius, 0.0));

    // Intersect ray with bounding sphere to get march start.
    float b    = dot(ro, rd);
    float c    = dot(ro, ro) - sphereRadius * sphereRadius;
    float disc = b * b - c;
    if (disc < 0.0) discard;
    float tEntry = -b - sqrt(disc);
    if (tEntry < 0.0) discard;

    vec3 p = ro + tEntry * rd;

    bool hit = false;
    const float EPS = 0.001;
    float exitR2 = sphereRadius * sphereRadius * 1.05;

    for (int i = 0; i < 32; i++)
    {
        // Apply Y-rotation then X-tilt to bring p into crystal frame.
        vec3 p1 = vec3(p.x * cosA - p.z * sinA, p.y, p.x * sinA + p.z * cosA);
        vec3 pr = vec3(p1.x, p1.y * cosT - p1.z * sinT, p1.y * sinT + p1.z * cosT);
        float d = sdHexPrism(pr, hexR, hexH);
        if (d < EPS * sphereRadius) { hit = true; break; }
        if (dot(p, p) > exitR2) break;
        p += rd * max(d, EPS * sphereRadius);
    }

    if (!hit) discard;

    // Normal: compute in crystal frame, apply inverse transform (undo tilt, then undo Y-rotation).
    vec3 p1_hit = vec3(p.x * cosA - p.z * sinA, p.y, p.x * sinA + p.z * cosA);
    vec3 pr_hit = vec3(p1_hit.x, p1_hit.y * cosT - p1_hit.z * sinT, p1_hit.y * sinT + p1_hit.z * cosT);
    vec3 localN = hexNormal(pr_hit, hexR, hexH);
    vec3 n1     = vec3(localN.x, localN.y * cosT + localN.z * sinT, -localN.y * sinT + localN.z * cosT);
    vec3 eyeNorm = normalize(vec3(n1.x * cosA + n1.z * sinA, n1.y, -n1.x * sinA + n1.z * cosA));

    vec3 viewDir = normalize(ro - p);
    if (dot(eyeNorm, viewDir) < 0.0) eyeNorm = -eyeNorm;

    float diff    = ambientStrength + (1.0 - ambientStrength) * max(0.0, dot(eyeNorm, lightDirEye));
    vec3 reflDir  = reflect(-lightDirEye, eyeNorm);
    float spec    = pow(max(0.0, dot(viewDir, reflDir)), specularShininess) * specularStrength;
    float fresnel = pow(1.0 - max(0.0, dot(eyeNorm, viewDir)), 4.0) * fresnelStrength;

    vec3 result = mix(shadowColor, litColor, diff) + vec3(spec + fresnel);

    vec3 hitEye     = eyeSpacePos + p;
    vec4 clipPos    = projection * vec4(hitEye, 1.0);
    gl_FragDepth    = clipPos.z / clipPos.w * 0.5 + 0.5;

    FragmentColor = vec4(result, 1.0);
}
