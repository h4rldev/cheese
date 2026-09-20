#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uTexture;

void main() {
    float d = texture(uTexture, fragUV).r;
    float screen_px = (d - 0.5) / max(fwidth(d), 1e-5);
    float alpha = clamp(screen_px + 0.5, 0.0, 1.0);
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
