#version 130

uniform sampler2D texture;
uniform int colorBlindMode;  // 0=None, 1=Protanopia, 2=Deuteranopia, 3=Tritanopia, 4=HighContrast

// Protanopia (red-blind) simulation matrix
mat3 protanopia_sim = mat3(
    0.567, 0.433, 0.000,
    0.558, 0.442, 0.000,
    0.000, 0.242, 0.758
);

// Protanopia correction matrix
mat3 protanopia_correct = mat3(
    1.0, 0.0, 0.0,
    0.7, 1.0, 0.0,
    0.7, 0.0, 1.0
);

// Deuteranopia (green-blind) simulation matrix
mat3 deuteranopia_sim = mat3(
    0.625, 0.375, 0.000,
    0.700, 0.300, 0.000,
    0.000, 0.300, 0.700
);

// Deuteranopia correction matrix
mat3 deuteranopia_correct = mat3(
    1.0, 0.0, 0.0,
    0.7, 1.0, 0.0,
    0.7, 0.0, 1.0
);

// Tritanopia (blue-blind) simulation matrix
mat3 tritanopia_sim = mat3(
    0.950, 0.050, 0.000,
    0.000, 0.433, 0.567,
    0.000, 0.475, 0.525
);

// Tritanopia correction matrix
mat3 tritanopia_correct = mat3(
    1.0, 0.0, 0.7,
    0.0, 1.0, 0.7,
    0.0, 0.0, 1.0
);

vec3 applyColorblindCorrection(vec3 color, mat3 sim, mat3 correct)
{
    vec3 simulated = sim * color;
    vec3 error = color - simulated;
    vec3 correction = correct * error;
    return clamp(color + correction, 0.0, 1.0);
}

vec3 applyHighContrast(vec3 color)
{
    float luminance = dot(color, vec3(0.299, 0.587, 0.114));

    // Boost saturation by 40%
    vec3 saturated = mix(vec3(luminance), color, 1.4);

    // Increase contrast by 30%
    vec3 contrasted = (saturated - 0.5) * 1.3 + 0.5;

    return clamp(contrasted, 0.0, 1.0);
}

void main()
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    vec3 color = pixel.rgb;

    if (colorBlindMode == 1) {
        // Protanopia
        color = applyColorblindCorrection(color, protanopia_sim, protanopia_correct);
    } else if (colorBlindMode == 2) {
        // Deuteranopia
        color = applyColorblindCorrection(color, deuteranopia_sim, deuteranopia_correct);
    } else if (colorBlindMode == 3) {
        // Tritanopia
        color = applyColorblindCorrection(color, tritanopia_sim, tritanopia_correct);
    } else if (colorBlindMode == 4) {
        // High Contrast
        color = applyHighContrast(color);
    }
    // colorBlindMode == 0 (None) - no transformation

    gl_FragColor = vec4(color, pixel.a);
}
