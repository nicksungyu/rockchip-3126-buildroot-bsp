precision mediump float;

varying vec4 v_color;
varying vec2 v_texuv;
uniform sampler2D s_texture;

void main(void)
{
   gl_FragColor = texture2D(s_texture, v_texuv);
}

