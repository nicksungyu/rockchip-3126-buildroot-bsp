precision mediump float;

varying vec4 v_color;
varying vec2 v_texuv;
uniform sampler2D s_texture;
uniform sampler2D s_scanlines;
uniform vec4 v_key;

void main(void)
{
	vec4 col = v_color;
	col.a *= dot(texture2D(s_texture, v_texuv), v_key);
	gl_FragColor = col;
}
