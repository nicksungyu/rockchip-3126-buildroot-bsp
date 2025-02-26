precision mediump float;

varying vec4 v_color;
varying vec2 v_texuv;
uniform sampler2D s_texture;
uniform vec4 v_key;

void main(void)
{
  vec4 pixel = texture2D(s_texture, v_texuv);
  pixel.a *= v_color.a;
  if (pixel.rgb == v_key.rgb)
  {
    pixel.rgb = v_color.rgb;
  }
  gl_FragColor = pixel;
}

