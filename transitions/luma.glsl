// Author: gre
// License: MIT

vec4 transition(vec2 uv)
{
    if (progress == 1.0)
    {
        return getToColor(uv);
    }
    else
    {
        return mix(getToColor(uv), getFromColor(uv), step(progress, getFromColor(uv).r));
    }
}
