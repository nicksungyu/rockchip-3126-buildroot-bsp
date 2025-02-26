uniform mat4 u_mvpMatrix;
attribute vec4 a_position;
attribute vec4 a_color;
attribute vec2 a_texuv;
varying vec4 v_color;
varying vec2 v_texuv;
uniform sampler2D s_texture;
uniform sampler2D s_scanlines;

void main(void)
{
    v_color = a_color;
    v_texuv = a_texuv;
    gl_Position = u_mvpMatrix * a_position;
}
