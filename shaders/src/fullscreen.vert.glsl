#version 450
// PortalPC — Fullscreen quad vertex shader
// Generates a fullscreen triangle/quad without a vertex buffer (clip-space only).
// Two triangles covering NDC [-1,1] generated from vertex index.

layout(location = 0) out vec2 v_uv;

void main() {
    // Generate UV and position from vertex index (0..5 for two tris, or 0..2 for one fullscreen tri)
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    v_uv = uv;
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
