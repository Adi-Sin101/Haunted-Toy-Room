// Exponential distance fog shared by both rendering paths.
uniform vec3 uFogColor;
uniform float uFogDensity;
vec3 applyFog(vec3 color,float distance) {
    float visibility=exp(-uFogDensity*max(distance,0.0));
    return mix(uFogColor,color,visibility);
}
