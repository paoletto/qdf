varying highp vec2 qt_TexCoord;

uniform sampler2D qt_Texture;
uniform lowp float opacity;
uniform bool invert;

void main()
{
    vec4 fragColor = texture2D(qt_Texture, qt_TexCoord) ; //* opacity;
//    fragColor.g = 0.5;

    if (invert) {
//        gl_FragColor = mix(vec4(vec3(1,1,1) - fragColor.rgb, fragColor.a),
//                           vec4(0,0,0,1), fragColor.a);
        vec4 invertedColor = fragColor;
        invertedColor.rgb = vec3(1.0,1.0,1.0) - invertedColor.rgb;

//        if (fragColor.a == 1.0) {
//            gl_FragColor = invertedColor;
//        } else if (fragColor.a > 0.0) {
//            gl_FragColor =
//                    mix(vec4(0,0,0,1), invertedColor,invertedColor.a);
//        } else {
//            gl_FragColor = vec4(0,0,0,1);
//        }

        gl_FragColor =
                mix(vec4(0,0,0,1), invertedColor,invertedColor.a);
    } else {
        gl_FragColor = fragColor;
    }
}
