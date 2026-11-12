#pragma once

#include <cmath>
#include <cstring>

// 4x4 matrices in the game's layout: element [row * 4 + column], points are row
// vectors transformed as p' = p * M, and each vertex shader register holds one
// row. The game's projections are right-handed (row 2, column 3 is -1): the
// camera looks down -Z with +Y up and +X right, the same axes as an OpenXR view.
struct XrMatrix
{
    float m[16];
};

inline XrMatrix XrMatrixIdentity()
{
    XrMatrix result{};
    result.m[0] = result.m[5] = result.m[10] = result.m[15] = 1.0f;
    return result;
}

inline XrMatrix XrMatrixMultiply(const XrMatrix& a, const XrMatrix& b)
{
    XrMatrix result;

    for (int row = 0; row < 4; row++)
    {
        for (int column = 0; column < 4; column++)
        {
            result.m[row * 4 + column] =
                a.m[row * 4 + 0] * b.m[0 * 4 + column] +
                a.m[row * 4 + 1] * b.m[1 * 4 + column] +
                a.m[row * 4 + 2] * b.m[2 * 4 + column] +
                a.m[row * 4 + 3] * b.m[3 * 4 + column];
        }
    }

    return result;
}

// General inverse. False when the matrix is singular.
inline bool XrMatrixInvert(const XrMatrix& matrix, XrMatrix& result)
{
    const float* m = matrix.m;
    float inv[16];

    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    float determinant = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (std::abs(determinant) < 1.0e-20f)
        return false;

    float inverseDeterminant = 1.0f / determinant;
    for (int i = 0; i < 16; i++)
        result.m[i] = inv[i] * inverseDeterminant;

    return true;
}

// The eye's transform from the game camera's view space: the game camera stands
// where the player's head was when the view was last recentred, so an eye at
// `position` (metres from there) with `orientation` (in the application space,
// whose axes the game camera's view space shares) sees view-space points p as
// p * result. unitsPerMeter scales head motion and the distance between the eyes.
inline XrMatrix XrMatrixEyeFromView(const float orientation[4], const float position[3], float unitsPerMeter)
{
    float x = orientation[0], y = orientation[1], z = orientation[2], w = orientation[3];
    float length = std::sqrt(x * x + y * y + z * z + w * w);
    if (length > 0.0f)
    {
        x /= length;
        y /= length;
        z /= length;
        w /= length;
    }

    // The eye's axes in view space (column vectors: viewPoint = R * eyePoint).
    float r[3][3] =
    {
        { 1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - z * w), 2.0f * (x * z + y * w) },
        { 2.0f * (x * y + z * w), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - x * w) },
        { 2.0f * (x * z - y * w), 2.0f * (y * z + x * w), 1.0f - 2.0f * (x * x + y * y) },
    };

    float t[3] = { position[0] * unitsPerMeter, position[1] * unitsPerMeter, position[2] * unitsPerMeter };

    // eyePoint = transpose(R) * (viewPoint - t), as a row vector: (p - t) * R.
    XrMatrix result{};
    for (int row = 0; row < 3; row++)
    {
        for (int column = 0; column < 3; column++)
            result.m[row * 4 + column] = r[row][column];
    }

    for (int column = 0; column < 3; column++)
        result.m[12 + column] = -(t[0] * r[0][column] + t[1] * r[1][column] + t[2] * r[2][column]);

    result.m[15] = 1.0f;
    return result;
}

// The four frustum terms of a right-handed off-centre projection for an OpenXR
// field of view, in the game's layout: elements [0], [5], [8] and [9]. The
// depth terms are left to the game, so its near/far planes and reverse Z hold.
inline void XrProjectionFrustumTerms(float angleLeft, float angleRight, float angleUp, float angleDown, float terms[4])
{
    float tanLeft = std::tan(angleLeft);
    float tanRight = std::tan(angleRight);
    float tanUp = std::tan(angleUp);
    float tanDown = std::tan(angleDown);

    terms[0] = 2.0f / (tanRight - tanLeft);
    terms[1] = 2.0f / (tanUp - tanDown);
    terms[2] = (tanRight + tanLeft) / (tanRight - tanLeft);
    terms[3] = (tanUp + tanDown) / (tanUp - tanDown);
}
