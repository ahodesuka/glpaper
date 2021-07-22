// License: MIT
// Author: Xaychru
// ported by gre from https://gist.github.com/Xaychru/ce1d48f0ce00bb379750

const float smoothness = 1.0;

const float PI = 3.141592653589;

vec4 transition(vec2 p)
{
    if (progress == 0.0)
    {
        return getFromColor(p);
    }
    else if (progress == 1.0)
    {
        return getToColor(p);
    }
    else
    {
        vec2 rp = p * 2. - 1.;
        return mix(getToColor(p),
                   getFromColor(p),
                   smoothstep(0., smoothness, atan(rp.y, rp.x) - (progress - .5) * PI * 2.5));
    }
}
