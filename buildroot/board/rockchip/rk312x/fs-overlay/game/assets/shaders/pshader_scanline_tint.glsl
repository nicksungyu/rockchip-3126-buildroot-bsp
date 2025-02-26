precision mediump float;

varying vec4 v_color;
varying vec2 v_texuv;
uniform sampler2D s_texture;
uniform sampler2D s_scanlines;

void main(void)
{
   gl_FragColor = texture2D(s_texture, v_texuv) * texture2D(s_scanlines, v_texuv) * v_color;
}

