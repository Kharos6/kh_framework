#pragma once

// The feature half (rendering_integration_features.hpp): CHAIN, the chain solver: quaternions, joints, contacts and
// the chain step. Never include it on its own.
namespace RenderIntegration {

inline ChainParams chain_defaults() {
    ChainParams p;
    // drag: 0.6 kg/s on the default 1 kg chain is a coupling of 0.6 /s to the
    // air (CLOTH_DRAG), so a 10 m/s wind swings a hanging chain about 30
    // degrees; a heavier chain swings less, by its mass; waterDrag 61.3 is
    // 61.3 /s. substeps 4 / iterations 2: rigid-body XPBD converges by
    // substepping (the 2020 paper's "small steps"). Two passes hold swing_limit
    // at 15 degrees and wider under brisk motion (an abrupt start can carry a
    // joint past it for a moment). At 10 degrees a thin chain (links a
    // centimetre across) swayed hard (0.3 m at a walk) can diverge at two
    // passes and stays diverged until CHAIN_RUNAWAY puts it back, again and
    // again while the motion lasts; four passes hold it. At 5 degrees even four
    // passes break open in bursts under that motion. A tight limit wants
    // iterations 4 and gentle motion.
    p.stretch = 1.0f; p.bend = 0.05f; p.twist = 0.30f; p.damping = 0.02f; p.drag = 0.6f;
    p.lin_damping = 0.0f; p.ang_damping = 0.0f;   // Off: 'drag' alone.
    p.mass = 1.0f; p.gravity = 1.0f; p.thickness = 0.005f; p.friction = 0.5f;
    p.swing_limit = 180.0f; p.twist_limit = 180.0f; p.max_speed = 40.0f; p.teleport = 2.0f;
    p.range = 1.0f; p.sleep = 0.005f; p.wind = 1.0f; p.buoyancy = 0.3f; p.water_drag = 61.3f;
    p.iterations = 2; p.substeps = 4;
    return p;
}

// Quaternions (x, y, z, w), unit, taking a body vector to the frame's.
inline void q_mul(const float* a, const float* b, float* o) {
    const float x = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
    const float y = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
    const float z = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
    const float w = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
    o[0] = x; o[1] = y; o[2] = z; o[3] = w;
}
inline void q_rot(const float* q, const float* v, float* o) {   // q v q*
    const float tx = 2.0f * (q[1] * v[2] - q[2] * v[1]);
    const float ty = 2.0f * (q[2] * v[0] - q[0] * v[2]);
    const float tz = 2.0f * (q[0] * v[1] - q[1] * v[0]);
    const float ox = v[0] + q[3] * tx + (q[1] * tz - q[2] * ty);
    const float oy = v[1] + q[3] * ty + (q[2] * tx - q[0] * tz);
    const float oz = v[2] + q[3] * tz + (q[0] * ty - q[1] * tx);
    o[0] = ox; o[1] = oy; o[2] = oz;
}
inline void q_rot_inv(const float* q, const float* v, float* o) {   // q* v q
    const float c[4] = { -q[0], -q[1], -q[2], q[3] };
    q_rot(c, v, o);
}
inline void q_norm(float* q) {
    const float l = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (!(l > 1.0e-20f)) { q[0] = 0.0f; q[1] = 0.0f; q[2] = 0.0f; q[3] = 1.0f; return; }
    for (int k = 0; k < 4; ++k) q[k] /= l;
}
// From a proper rotation given as the images of the three axes: c[k * 3 + j]
// is component j of where axis k goes - the rows of a rot_m, or the columns of
// the matrix that multiplies a column vector.
inline void q_from_axes(const float* c, float* q) {
    // m[r][s] = c[s * 3 + r]: the matrix whose columns are the images.
    const float m00 = c[0], m11 = c[4], m22 = c[8];
    const float tr = m00 + m11 + m22;
    if (tr > 0.0f) {
        const float s = sqrtf(tr + 1.0f) * 2.0f;
        q[3] = 0.25f * s;
        q[0] = (c[1 * 3 + 2] - c[2 * 3 + 1]) / s;
        q[1] = (c[2 * 3 + 0] - c[0 * 3 + 2]) / s;
        q[2] = (c[0 * 3 + 1] - c[1 * 3 + 0]) / s;
    } else if (m00 > m11 && m00 > m22) {
        const float s = sqrtf(1.0f + m00 - m11 - m22) * 2.0f;
        q[3] = (c[1 * 3 + 2] - c[2 * 3 + 1]) / s;
        q[0] = 0.25f * s;
        q[1] = (c[1 * 3 + 0] + c[0 * 3 + 1]) / s;
        q[2] = (c[2 * 3 + 0] + c[0 * 3 + 2]) / s;
    } else if (m11 > m22) {
        const float s = sqrtf(1.0f + m11 - m00 - m22) * 2.0f;
        q[3] = (c[2 * 3 + 0] - c[0 * 3 + 2]) / s;
        q[0] = (c[1 * 3 + 0] + c[0 * 3 + 1]) / s;
        q[1] = 0.25f * s;
        q[2] = (c[2 * 3 + 1] + c[1 * 3 + 2]) / s;
    } else {
        const float s = sqrtf(1.0f + m22 - m00 - m11) * 2.0f;
        q[3] = (c[0 * 3 + 1] - c[1 * 3 + 0]) / s;
        q[0] = (c[2 * 3 + 0] + c[0 * 3 + 2]) / s;
        q[1] = (c[2 * 3 + 1] + c[1 * 3 + 2]) / s;
        q[2] = 0.25f * s;
    }
    q_norm(q);
}
// The inverse: c[k * 3 + j] = component j of the image of axis k.
inline void q_to_axes(const float* q, float* c) {
    static const float e[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    for (int k = 0; k < 3; ++k) q_rot(q, e + k * 3, c + k * 3);
}
inline void q_nlerp(const float* a, const float* b, float t, float* o) {
    const float d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    const float s = d < 0.0f ? -1.0f : 1.0f;   // The short way round.
    for (int k = 0; k < 4; ++k) o[k] = a[k] + (s * b[k] - a[k]) * t;
    q_norm(o);
}
// q turned by the small rotation vector w (radians, the frame's axes).
inline void q_spin(float* q, const float* w) {
    const float dq[4] = { w[0], w[1], w[2], 0.0f };
    float r[4];
    q_mul(dq, q, r);
    for (int k = 0; k < 4; ++k) q[k] += 0.5f * r[k];
    q_norm(q);
}

// A 0..1 stiffness dial to an XPBD compliance over h squared, relative to the
// constraint's own generalised inverse mass w: the dial sets the natural
// frequency the constraint springs at, 0.05 Hz to 50 Hz exponentially, so it
// means the same on a thin cord and a heavy belt. 1 (>= 0.999) is exactly
// rigid (0); 0 or less is off (negative).
inline float chain_alpha(float dial, float w, float h) {
    if (dial >= 0.999f) return 0.0f;
    if (!(dial > 0.0f)) return -1.0f;
    const float om = 6.2831853f * 0.05f * powf(1000.0f, dial) * h;
    return w / (om * om);
}

// The world inverse inertia applied to a world vector: q Ib^-1 q* v.
inline void chain_iw(const ChainBody& b, const float* v, float* o) {
    float l[3];
    q_rot_inv(b.q, v, l);
    const float* m = b.ib_inv;
    const float r[3] = { m[0] * l[0] + m[1] * l[1] + m[2] * l[2],
                         m[3] * l[0] + m[4] * l[1] + m[5] * l[2],
                         m[6] * l[0] + m[7] * l[1] + m[8] * l[2] };
    q_rot(b.q, r, o);
}
// Generalised inverse mass of a positional correction along n at world offset r.
inline float chain_wpos(const ChainBody& b, const float* r, const float* n) {
    if (!(b.inv_m > 0.0f)) return 0.0f;
    const float rn[3] = { r[1] * n[2] - r[2] * n[1], r[2] * n[0] - r[0] * n[2], r[0] * n[1] - r[1] * n[0] };
    float irn[3];
    chain_iw(b, rn, irn);
    return b.inv_m + rn[0] * irn[0] + rn[1] * irn[1] + rn[2] * irn[2];
}
// ...and of a rotation about n.
inline float chain_wrot(const ChainBody& b, const float* n) {
    if (!(b.inv_m > 0.0f)) return 0.0f;
    float in[3];
    chain_iw(b, n, in);
    return n[0] * in[0] + n[1] * in[1] + n[2] * in[2];
}
// Impulse p (a position correction times mass) at world offset r.
inline void chain_push(ChainBody& b, const float* r, const float* p) {
    if (!(b.inv_m > 0.0f)) return;
    for (int k = 0; k < 3; ++k) b.x[k] += p[k] * b.inv_m;
    const float rp[3] = { r[1] * p[2] - r[2] * p[1], r[2] * p[0] - r[0] * p[2], r[0] * p[1] - r[1] * p[0] };
    float dw[3];
    chain_iw(b, rp, dw);
    q_spin(b.q, dw);
}

// A point of a body to the instance's frame, now or at the substep's start.
inline void chain_point(const ChainBody& b, const float* r, bool start, float* o) {
    float d[3];
    q_rot(start ? b.qp : b.q, r, d);
    const float* x = start ? b.xp : b.x;
    for (int k = 0; k < 3; ++k) o[k] = x[k] + d[k];
}

// Where the carrier (and the skeleton, when a binding hands one) puts a bone
// that is not simulated, as a carrier-local affine map: l' = A l + t, A given
// as the images of the three axes (q_from_axes' layout). g0 / g1 are the
// skeleton's pose at the step's start and end (skin_affine's layout: rows
// Q, then T, v' = v * Q + T in the authored frame), blended at s; a null one
// takes the other's pose, and with both null (or the bone out of range) the
// map is the identity (the rest).
inline void chain_bone_map(const ChainState& c, const float* g0, const float* g1, size_t nbone, int32_t bone,
                           float s, float* q_out, float* t_out) {
    const float* ma = g0 && bone >= 0 && static_cast<size_t>(bone) < nbone ? g0 + static_cast<size_t>(bone) * 12u : nullptr;
    const float* mb = g1 && bone >= 0 && static_cast<size_t>(bone) < nbone ? g1 + static_cast<size_t>(bone) * 12u : nullptr;
    if (!ma) ma = mb;
    if (!mb) mb = ma;
    if (!ma) {
        q_out[0] = q_out[1] = q_out[2] = 0.0f; q_out[3] = 1.0f;
        t_out[0] = t_out[1] = t_out[2] = 0.0f;
        return;
    }
    // Q's rows are where each authored axis goes: exactly the images.
    float qa[4], qb[4], T[3];
    q_from_axes(ma, qa);
    q_from_axes(mb, qb);
    q_nlerp(qa, qb, s, q_out);
    for (int k = 0; k < 3; ++k) T[k] = ma[9 + k] + (mb[9 + k] - ma[9 + k]) * s;
    // Authored p = l + auth_ctr maps to p' = A p + T; carrier-local l' = p' - auth_ctr.
    float ac[3];
    q_rot(q_out, c.auth_ctr, ac);
    for (int k = 0; k < 3; ++k) t_out[k] = ac[k] + T[k] - c.auth_ctr[k];
}

// A body's pose where the carrier frame f and the bone map (q, t) put it: its
// centre of mass and orientation in the instance's frame. qf is f's rotation.
inline void chain_place(const ChainBody& b, const ClothFrame& f, const float* qf, const float* bq,
                        const float* bt, float* x_out, float* q_out, const float* off = nullptr) {
    float l[3];
    q_rot(bq, b.x0, l);
    for (int k = 0; k < 3; ++k) l[k] += bt[k];
    if (off) for (int k = 0; k < 3; ++k) l[k] += off[k];   // CHAIN_POINT.
    float w[3];
    q_rot(qf, l, w);
    for (int k = 0; k < 3; ++k) x_out[k] = f.centre[k] + w[k];
    q_mul(qf, bq, q_out);
    q_norm(q_out);
}
// CHAIN_POINT - a body's rest offset, or null where the chain has none
// (startPoint "tail", the default).
inline const float* chain_start_off(const ChainState& c, size_t i) {
    return c.start_off.size() == c.body.size() * 3u ? &c.start_off[i * 3u] : nullptr;
}
inline void chain_frame_q(const ClothFrame& f, float* q) {
    if (!f.rotated) { q[0] = q[1] = q[2] = 0.0f; q[3] = 1.0f; return; }
    q_from_axes(f.rot, q);
}

// Every body at its place on the carrier - the rest pose, or the skeleton's -
// still. The cold start and the rescue.
inline void chain_reset(const ClothState& s, ChainState& c, const float* g, size_t nbone) {
    ClothFrame f;
    for (int k = 0; k < 3; ++k) { f.centre[k] = s.centre[k]; f.size[k] = s.size[k]; }
    memcpy(f.rot, s.rot, sizeof(f.rot));
    f.rotated = s.rotated;
    float qf[4];
    chain_frame_q(f, qf);
    for (size_t i = 0; i < c.body.size(); ++i) {
        ChainBody& b = c.body[i];
        float bq[4], bt[3];
        chain_bone_map(c, g, g, nbone, b.bone, 1.0f, bq, bt);
        chain_place(b, f, qf, bq, bt, b.x, b.q, chain_start_off(c, i));
        memcpy(b.xp, b.x, sizeof(b.x));
        memcpy(b.qp, b.q, sizeof(b.q));
        for (int k = 0; k < 3; ++k) { b.v[k] = 0.0f; b.w[k] = 0.0f; }
    }
}

// One contact point against one surface: cloth_ground_contact or
// physics_collide_one moved a copy of it by d; that motion is applied to the
// body at the point through its generalised inverse mass (a contact is rigid),
// then Coulomb friction against the surface's own motion sv over the substep
// (null = a still surface), capped by the cone mu * |d|, as the cloth's.
inline void chain_contact(ChainBody& b, const float* r_body, const float* d, const float* sv, float mu) {
    const float dl = cloth_v3_len(d);
    if (!(dl > 1.0e-9f)) return;
    const float n[3] = { d[0] / dl, d[1] / dl, d[2] / dl };
    float p[3], r[3];
    chain_point(b, r_body, false, p);
    cloth_v3_sub(p, b.x, r);
    const float w = chain_wpos(b, r, n);
    if (!(w > 0.0f)) return;
    const float imp[3] = { n[0] * dl / w, n[1] * dl / w, n[2] * dl / w };
    chain_push(b, r, imp);
    if (!(mu > 0.0f)) return;
    float p1[3], p0[3], m[3];
    chain_point(b, r_body, false, p1);
    chain_point(b, r_body, true, p0);
    for (int k = 0; k < 3; ++k) m[k] = (p1[k] - p0[k]) - (sv ? sv[k] : 0.0f);
    const float mn = cloth_v3_dot(m, n);
    float t[3];
    for (int k = 0; k < 3; ++k) t[k] = m[k] - n[k] * mn;
    const float tl = cloth_v3_len(t);
    if (!(tl > 1.0e-9f)) return;
    const float take = tl < mu * dl ? tl : mu * dl;
    const float tn[3] = { -t[0] / tl, -t[1] / tl, -t[2] / tl };
    cloth_v3_sub(p1, b.x, r);
    const float wt = chain_wpos(b, r, tn);
    if (!(wt > 0.0f)) return;
    const float ti[3] = { tn[0] * take / wt, tn[1] * take / wt, tn[2] * take / wt };
    chain_push(b, r, ti);
}

// The substep's inputs beyond the cloth's: the frames and fractions it runs
// between, the skeleton at both ends of the step, and the end's target.
struct ChainSub {
    const ClothFrame* old_f;   // The step's carrier at its start (fraction 0) and end (1).
    const ClothFrame* new_f;
    const float* g0;             // Skeleton at fraction 0 and 1 (skin_affine's layout), or null.
    const float* g1;
    size_t nbone;
    const ChainTarget* tg;
    float t0, t1;                // This substep's fractions.
};

// A body's kinematic pose at fraction s of the step.
inline void chain_kin(const ChainState& c, size_t i, const ChainSub& u, float s, float* x, float* q) {
    const ChainBody& b = c.body[i];
    ClothFrame f;
    cloth_frame_lerp(*u.old_f, *u.new_f, s, f);
    float qf[4], bq[4], bt[3];
    chain_frame_q(f, qf);
    chain_bone_map(c, u.g0, u.g1, u.nbone, b.bone, s, bq, bt);
    chain_place(b, f, qf, bq, bt, x, q, chain_start_off(c, i));
}

// CHAIN_REACH_ROT - the reach of the end's held point about the pin pin (the end's path hangs off it:
// joint end_root - 1's ra on body 0), given the end's held orientation q (carrier-local; null = not held).
// Not held: the straight chain's reach (ChainState::end_reach: the path to the end body's head plus the point's
// offset from the head, CHAIN_POINT), the pin unchanged. Held: the pin moved by the held orientation's image of
// that offset and the reach short of it by the offset's length, never below 0 (CHAIN_REACH_ZERO) - the head's
// own sphere, which is where the point lies with the body at that orientation. Both callers (chain_substep's
// clamp and chain_end_taut's, its draw twin) clamp a target onto the sphere so returned.
inline float chain_end_reach_rot(const ChainState& c, const float* q, float pin[3]) {
    if (!q || c.end_body <= 0 || static_cast<size_t>(c.end_body) > c.joint.size()) return c.end_reach;
    float o[3], w[3];
    cloth_v3_sub(c.end_pt, c.joint[static_cast<size_t>(c.end_body) - 1u].rb, o);
    const float t = cloth_v3_len(o);
    q_rot(q, o, w);
    for (int k = 0; k < 3; ++k) pin[k] += w[k];
    return c.end_reach > t ? c.end_reach - t : 0.0f;
}
// The end's position and orientation targets at fraction s: kind 1, the world point of whichever end of the bone
// endPoint names (CHAIN_POINT, c.end_pt) for pos, the body's world orientation for rot; kind 2, the given target,
// swept from the last step's (prev_p / prev_q) when that was kind 2 too. False = none.
inline bool chain_end_pos(const ChainState& c, const ChainSub& u, float s, float* p) {
    if (c.end_body < 0 || !u.tg) return false;
    const ChainBody& e = c.body[static_cast<size_t>(c.end_body)];
    const ChainJoint* j = e.parent >= 0 ? &c.joint[static_cast<size_t>(c.end_body) - 1u] : nullptr;
    if (u.tg->pos == 1u) {
        float x[3], q[4], d[3];
        chain_kin(c, static_cast<size_t>(c.end_body), u, s, x, q);
        if (!j) { memcpy(p, x, sizeof(x)); return true; }
        q_rot(q, c.end_pt, d);   // CHAIN_POINT: that bone's head or its tail.
        for (int k = 0; k < 3; ++k) p[k] = x[k] + d[k];
        return true;
    }
    if (u.tg->pos == 2u) {
        const bool sweep = c.prev_pos == 2u;
        for (int k = 0; k < 3; ++k) p[k] = sweep ? c.prev_p[k] + (u.tg->p[k] - c.prev_p[k]) * s : u.tg->p[k];
        return true;
    }
    return false;
}
inline bool chain_end_rot(const ChainState& c, const ChainSub& u, float s, float* q) {
    if (c.end_body < 0 || !u.tg) return false;
    if (u.tg->rot == 1u) {
        float x[3];
        chain_kin(c, static_cast<size_t>(c.end_body), u, s, x, q);
        return true;
    }
    if (u.tg->rot == 2u) {
        float qt[4];
        q_from_axes(u.tg->r, qt);
        if (c.prev_rot == 2u) q_nlerp(c.prev_q, qt, s, q);
        else memcpy(q, qt, sizeof(qt));
        return true;
    }
    return false;
}

// CHAIN - THE JOINTS ARE SOLVED DIRECTLY, not one after another: a long
// rigid chain projected joint by joint (Gauss-Seidel) converges only as fast as
// a correction can travel down it, so every substep leaves the chain stretched
// by what gravity added and the next velocity carries that correction as
// motion. Every joint's six rows (its position, and its twist and swing
// springs) and the end's hold are one linear system per pass; a system on a
// tree factors exactly in linear time with no fill-in (Baraff 1996,
// "Linear-Time Dynamics using Lagrange Multipliers"; applied to position-based
// rods by Deul, Kugelstadt, Weiler and Bender 2018). The nodes are the free
// bodies (their 6x6 mass) and the joints (their 6x6 compliance), each joined to
// its neighbours by its Jacobian; children are eliminated into parents, and
// the solve runs down and back up. The kinematic start is no node: the joints
// on it are roots. A rigid row keeps a compliance of CHAIN_RIGID so that a
// joint with one body (the end's hold) is an invertible pivot; a row that is
// off keeps CHAIN_OFF, which asks for no force.
static constexpr double CHAIN_RIGID = 1.0e-9;
static constexpr double CHAIN_OFF = 1.0e12;
static constexpr float CHAIN_SEEK_MS = 2.0f;           // The end's approach: metres per second...
static constexpr float CHAIN_SEEK_RAD = 3.14159265f;   // ...and radians per second.

// 6x6 blocks, row-major, double.
inline bool m6_inv(const double* a, double* o) {
    double m[36];
    memcpy(m, a, sizeof(m));
    for (int i = 0; i < 36; ++i) o[i] = (i % 7 == 0) ? 1.0 : 0.0;
    for (int c = 0; c < 6; ++c) {
        int p = c;
        double best = fabs(m[c * 6 + c]);
        for (int r = c + 1; r < 6; ++r) if (fabs(m[r * 6 + c]) > best) { best = fabs(m[r * 6 + c]); p = r; }
        if (!(best > 1.0e-300)) return false;
        if (p != c) for (int k = 0; k < 6; ++k) { std::swap(m[c * 6 + k], m[p * 6 + k]); std::swap(o[c * 6 + k], o[p * 6 + k]); }
        const double iv = 1.0 / m[c * 6 + c];
        for (int k = 0; k < 6; ++k) { m[c * 6 + k] *= iv; o[c * 6 + k] *= iv; }
        for (int r = 0; r < 6; ++r) {
            if (r == c) continue;
            const double f = m[r * 6 + c];
            if (f == 0.0) continue;
            for (int k = 0; k < 6; ++k) { m[r * 6 + k] -= f * m[c * 6 + k]; o[r * 6 + k] -= f * o[c * 6 + k]; }
        }
    }
    return true;
}
inline void m6_mul(const double* a, const double* b, double* o) {   // o = a b
    for (int r = 0; r < 6; ++r) for (int c = 0; c < 6; ++c) {
        double s = 0.0;
        for (int k = 0; k < 6; ++k) s += a[r * 6 + k] * b[k * 6 + c];
        o[r * 6 + c] = s;
    }
}
inline void m6_tmul_sub(const double* a, const double* b, double* o) {   // o -= a^T b
    for (int r = 0; r < 6; ++r) for (int c = 0; c < 6; ++c) {
        double s = 0.0;
        for (int k = 0; k < 6; ++k) s += a[k * 6 + r] * b[k * 6 + c];
        o[r * 6 + c] -= s;
    }
}

// A joint's Jacobian block against one body, 6x6: rows 0-2 its position (the
// body's anchor, world offset r, with sign sg: +1 the child, -1 the parent),
// rows 3-5 its rotation along the three world axes n (sign -sg: the error is
// what turns the child onto the parent). Columns: the body's dx, then its
// rotation.
inline void chain_jac(const float* r, const float* n, double sg, double* o) {
    memset(o, 0, 36 * sizeof(double));
    for (int k = 0; k < 3; ++k) o[k * 6 + k] = sg;
    // -sg [r]x: the anchor moves by dtheta x r = -r x dtheta.
    const double rx = r[0], ry = r[1], rz = r[2];
    o[0 * 6 + 4] = -sg * -rz; o[0 * 6 + 5] = -sg * ry;
    o[1 * 6 + 3] = -sg * rz;  o[1 * 6 + 5] = -sg * -rx;
    o[2 * 6 + 3] = -sg * -ry; o[2 * 6 + 4] = -sg * rx;
    for (int k = 0; k < 3; ++k) for (int j = 0; j < 3; ++j) o[(3 + k) * 6 + 3 + j] = -sg * n[k * 3 + j];
}

// The rotation vector turning orientation qb onto qa (world axes).
inline void chain_rotvec(const float* qa, const float* qb, float* o) {
    const float cb[4] = { -qb[0], -qb[1], -qb[2], qb[3] };
    float d[4];
    q_mul(qa, cb, d);
    if (d[3] < 0.0f) for (int k = 0; k < 4; ++k) d[k] = -d[k];
    const float s = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    if (!(s > 1.0e-9f)) { o[0] = 2.0f * d[0]; o[1] = 2.0f * d[1]; o[2] = 2.0f * d[2]; return; }
    const float a = 2.0f * atan2f(s, d[3]) / s;
    for (int k = 0; k < 3; ++k) o[k] = d[k] * a;
}

// A joint's three rotation rows and their errors, the same for both of its
// nodes in the solve. Row 0: the twist - about the child's axis u, the angle
// that turns the child's cross-vector e onto the parent's once both are laid
// across u (signed). Row 1: the swing - the true angle between the two bones'
// axes, about their common normal. Row 2: across both, error zero. Exact
// measures, not a split of the relative rotation vector: that split is only
// first order, and with the twist free it puts the swing limit on the wrong
// quantity.
inline void chain_joint_rows(const ChainBody& A, const ChainBody& B, const ChainJoint& j, float* n, float* cv) {
    float ua[3], ub[3], ea[3], eb[3];
    q_rot(A.q, j.u, ua);
    q_rot(B.q, j.u, ub);
    q_rot(A.q, j.e, ea);
    q_rot(B.q, j.e, eb);
    memcpy(n, ub, sizeof(ub));
    // Twist, about ub.
    const float da = cloth_v3_dot(ea, ub), db = cloth_v3_dot(eb, ub);
    float pa[3], pb[3];
    for (int k = 0; k < 3; ++k) { pa[k] = ea[k] - ub[k] * da; pb[k] = eb[k] - ub[k] * db; }
    const float tc[3] = { pb[1] * pa[2] - pb[2] * pa[1], pb[2] * pa[0] - pb[0] * pa[2], pb[0] * pa[1] - pb[1] * pa[0] };
    cv[0] = atan2f(cloth_v3_dot(tc, ub), cloth_v3_dot(pb, pa));
    // Swing, about ub x ua.
    const float sc[3] = { ub[1] * ua[2] - ub[2] * ua[1], ub[2] * ua[0] - ub[0] * ua[2], ub[0] * ua[1] - ub[1] * ua[0] };
    const float sl = cloth_v3_len(sc);
    const float cs = cloth_v3_dot(ua, ub);
    if (sl > 1.0e-7f) {
        for (int k = 0; k < 3; ++k) n[3 + k] = sc[k] / sl;
        cv[1] = atan2f(sl, cs);
    } else {
        memcpy(n + 3, eb, sizeof(eb));   // Aligned (no swing) or folded straight back (a half turn).
        cv[1] = cs >= 0.0f ? 0.0f : 3.14159265f;
    }
    n[6] = n[1] * n[5] - n[2] * n[4];
    n[7] = n[2] * n[3] - n[0] * n[5];
    n[8] = n[0] * n[4] - n[1] * n[3];
    cv[2] = 0.0f;
}

// One pass of the direct solve over every joint and the end's hold, applied
// to the bodies. lam: 6 multipliers per joint then 6 for the end (XPBD's
// accumulated lambda, reset per substep by the caller). A joint's rotation
// rows are its twist (about the child's axis u) and its swing split along the
// direction it is swung (s) and across it: the springs pull each toward the
// rest angle with the bend / twist stiffness, and a limit (radians, negative
// = none) that is exceeded turns its row rigid at the limit for this pass - an
// inequality, re-decided on every pass, whose multiplier is not carried.
inline void chain_joint_solve(ChainState& c, const ChainParams& pr, float h, bool end_p, const float* endp,
                              bool end_q, const float* endq, float swing_lim, float twist_lim,
                              std::vector<double>& wk, float* lam) {
    const size_t nb = c.body.size();
    if (nb < 2u) return;
    const bool end_on = c.end_body > 0 && (end_p || end_q);
    // Node order, children before parents: for each body from the last,
    // [its end hold], its body node, its joint node.
    const size_t nn = 2u * (nb - 1u) + (end_on ? 1u : 0u);
    // Per node: kind (0 body, 1 joint, 2 end), body, parent node. 36 D, 36
    // L, 36 H_ip, 36 H_ii / Dinv, 6 x - in wk.
    std::vector<int32_t>& meta = c.solve_meta;
    meta.assign(nn * 3u, -1);
    std::vector<int32_t>& at_b = c.solve_at;
    at_b.assign(nb * 2u, -1);   // [body node, joint node] per body.
    size_t k = 0;
    for (size_t bi = nb - 1u; bi >= 1u; --bi) {
        if (end_on && static_cast<int32_t>(bi) == c.end_body) { meta[k * 3u] = 2; meta[k * 3u + 1u] = static_cast<int32_t>(bi); ++k; }
        meta[k * 3u] = 0; meta[k * 3u + 1u] = static_cast<int32_t>(bi); at_b[bi * 2u] = static_cast<int32_t>(k); ++k;
        meta[k * 3u] = 1; meta[k * 3u + 1u] = static_cast<int32_t>(bi); at_b[bi * 2u + 1u] = static_cast<int32_t>(k); ++k;
    }
    for (size_t i = 0; i < nn; ++i) {
        const int32_t kind = meta[i * 3u], bi = meta[i * 3u + 1u];
        if (kind == 2) meta[i * 3u + 2u] = at_b[static_cast<size_t>(bi) * 2u];
        else if (kind == 0) meta[i * 3u + 2u] = at_b[static_cast<size_t>(bi) * 2u + 1u];
        else {
            const int32_t pa = c.body[static_cast<size_t>(bi)].parent;
            meta[i * 3u + 2u] = pa >= 1 ? at_b[static_cast<size_t>(pa) * 2u] : -1;
        }
    }
    wk.assign(nn * 150u, 0.0);
    auto D = [&](size_t i) { return &wk[i * 150u]; };
    auto L = [&](size_t i) { return &wk[i * 150u + 36u]; };
    auto Hp = [&](size_t i) { return &wk[i * 150u + 72u]; };
    auto Di = [&](size_t i) { return &wk[i * 150u + 108u]; };
    auto X = [&](size_t i) { return &wk[i * 150u + 144u]; };
    static const float kIdent[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    // Assemble: each node's own block, the block to its parent, its right side.
    for (size_t i = 0; i < nn; ++i) {
        const int32_t kind = meta[i * 3u];
        const size_t bi = static_cast<size_t>(meta[i * 3u + 1u]);
        ChainBody& B = c.body[bi];
        double* d = D(i);
        double* x = X(i);
        if (kind == 0) {
            // The body's mass, world frame: m, then R Ib R^T.
            for (int e = 0; e < 3; ++e) d[e * 6 + e] = B.inv_m > 0.0f ? 1.0 / static_cast<double>(B.inv_m) : 1.0e30;
            float ax[9];
            q_to_axes(B.q, ax);   // ax[k * 3 + j]: component j of body axis k.
            for (int r = 0; r < 3; ++r) for (int s2 = 0; s2 < 3; ++s2) {
                double v = 0.0;
                for (int p = 0; p < 3; ++p) for (int q = 0; q < 3; ++q) v += ax[p * 3 + r] * static_cast<double>(B.ib[p * 3 + q]) * ax[q * 3 + s2];
                d[(3 + r) * 6 + 3 + s2] = v;
            }
            // To its joint (the parent node): that joint's Jacobian on it, transposed.
            const ChainJoint& j = c.joint[bi - 1u];
            float rb[3], n[9], cv[3];
            q_rot(B.q, j.rb, rb);
            chain_joint_rows(c.body[j.a], B, j, n, cv);   // The joint node's own rows (below), exactly.
            double jb[36];
            chain_jac(rb, n, 1.0, jb);
            double* hp = Hp(i);
            for (int r = 0; r < 6; ++r) for (int s2 = 0; s2 < 6; ++s2) hp[r * 6 + s2] = jb[s2 * 6 + r];
        } else if (kind == 1) {
            const ChainJoint& j = c.joint[bi - 1u];
            ChainBody& A = c.body[j.a];
            float* lj = lam + (bi - 1u) * 6u;
            float pa[3], pb[3], ra[3], rb[3], n[9], cp[3], jc[3];
            chain_point(A, j.ra, false, pa);
            chain_point(B, j.rb, false, pb);
            cloth_v3_sub(pa, A.x, ra);
            cloth_v3_sub(pb, B.x, rb);
            cloth_v3_sub(pb, pa, cp);
            chain_joint_rows(A, B, j, n, jc);
            const float tw = jc[0], swl = jc[1];
            // Compliance per row.
            double al[6];
            const float wt = A.inv_m + B.inv_m;
            const float as = chain_alpha(pr.stretch, wt, h);
            for (int r = 0; r < 3; ++r) al[r] = as < 0.0f ? CHAIN_OFF : (as > 0.0f ? static_cast<double>(as) : CHAIN_RIGID);
            for (int r = 0; r < 3; ++r) {
                const float wr = chain_wrot(A, n + r * 3) + chain_wrot(B, n + r * 3);
                const float aa = chain_alpha(r == 0 ? pr.twist : pr.bend, wr, h);
                al[3 + r] = aa < 0.0f ? CHAIN_OFF : (aa > 0.0f ? static_cast<double>(aa) : CHAIN_RIGID);
            }
            // Right side: -(C + alpha lambda), a limit's row rigid at the limit.
            for (int r = 0; r < 3; ++r) x[r] = -(static_cast<double>(cp[r]) + al[r] * lj[r]);
            double cv[3] = { tw, swl, 0.0 };
            bool lim[3] = { false, false, false };
            if (twist_lim >= 0.0f && fabsf(tw) > twist_lim) {
                cv[0] = tw > 0.0f ? tw - twist_lim : tw + twist_lim;
                lim[0] = true;
            }
            if (swing_lim >= 0.0f && swl > swing_lim) { cv[1] = swl - swing_lim; lim[1] = true; }
            uint8_t* was = c.lam_lim.size() == (nb - 1u) * 3u ? &c.lam_lim[(bi - 1u) * 3u] : nullptr;
            for (int r = 0; r < 3; ++r) {
                // THE ACTIVE SET CHANGED, SO THE WARM START IS VOID. A row's
                // accumulated multiplier belongs to the constraint it was
                // accumulated against (the spring's to the rest angle, the
                // limit's to the limit) and the right side carries it as
                // alpha * lambda; a row leaving its limit meets a compliance
                // orders of magnitude softer, so a rigid pass's multiplier
                // would ask for a correction of metres.
                const uint8_t now = lim[r] ? 1u : 0u;
                if (was && was[r] != now) { lj[3 + r] = 0.0f; was[r] = now; }
                if (lim[r]) { al[3 + r] = CHAIN_RIGID; lj[3 + r] = 0.0f; }
                x[3 + r] = -(cv[r] + al[3 + r] * lj[3 + r]);
            }
            for (int r = 0; r < 6; ++r) d[r * 6 + r] = -al[r];
            if (meta[i * 3u + 2u] >= 0) chain_jac(ra, n, -1.0, Hp(i));   // To the parent body.
        } else {
            // The end's hold: position (its held point - head or tail,
            // CHAIN_POINT - on its target) and rotation (the body onto its
            // target), each rigid where held and off where not.
            float* lj = lam + (nb - 1u) * 6u;
            float ph[3], rb[3], cp[3] = { 0.0f, 0.0f, 0.0f }, cr[3] = { 0.0f, 0.0f, 0.0f };
            chain_point(B, c.end_pt, false, ph);   // CHAIN_POINT.
            cloth_v3_sub(ph, B.x, rb);
            if (end_p) cloth_v3_sub(ph, endp, cp);
            if (end_q) chain_rotvec(endq, B.q, cr);
            double al[6];
            for (int r = 0; r < 3; ++r) { al[r] = end_p ? CHAIN_RIGID : CHAIN_OFF; al[3 + r] = end_q ? CHAIN_RIGID : CHAIN_OFF; }
            for (int r = 0; r < 6; ++r) d[r * 6 + r] = -al[r];
            for (int r = 0; r < 3; ++r) {
                x[r] = -(static_cast<double>(cp[r]) + al[r] * lj[r]);
                x[3 + r] = -(static_cast<double>(cr[r]) + al[3 + r] * lj[3 + r]);
            }
            chain_jac(rb, kIdent, 1.0, Hp(i));   // Its Jacobian on the end body, which is its parent node.
        }
    }
    // Factor: children into parents.
    for (size_t i = 0; i < nn; ++i) {
        if (!m6_inv(D(i), Di(i))) memset(Di(i), 0, 36 * sizeof(double));
        const int32_t p = meta[i * 3u + 2u];
        if (p < 0) continue;
        m6_mul(Di(i), Hp(i), L(i));
        m6_tmul_sub(Hp(i), L(i), D(static_cast<size_t>(p)));
    }
    // Solve: up (children into parents), across, and back down.
    for (size_t i = 0; i < nn; ++i) {
        const int32_t p = meta[i * 3u + 2u];
        if (p < 0) continue;
        const double* l = L(i);
        const double* xi = X(i);
        double* xp = X(static_cast<size_t>(p));
        for (int r = 0; r < 6; ++r) {
            double s = 0.0;
            for (int q = 0; q < 6; ++q) s += l[q * 6 + r] * xi[q];
            xp[r] -= s;
        }
    }
    for (size_t i = 0; i < nn; ++i) {
        double t[6];
        const double* di = Di(i);
        const double* xi = X(i);
        for (int r = 0; r < 6; ++r) { double s = 0.0; for (int q = 0; q < 6; ++q) s += di[r * 6 + q] * xi[q]; t[r] = s; }
        memcpy(X(i), t, sizeof(t));
    }
    for (size_t ii = nn; ii-- > 0;) {
        const int32_t p = meta[ii * 3u + 2u];
        if (p < 0) continue;
        const double* l = L(ii);
        const double* xp = X(static_cast<size_t>(p));
        double* xi = X(ii);
        for (int r = 0; r < 6; ++r) { double s = 0.0; for (int q = 0; q < 6; ++q) s += l[r * 6 + q] * xp[q]; xi[r] -= s; }
    }
    // Apply: the bodies move by their rows; the multipliers take -y.
    for (size_t i = 0; i < nn; ++i) {
        const int32_t kind = meta[i * 3u];
        const size_t bi = static_cast<size_t>(meta[i * 3u + 1u]);
        const double* xi = X(i);
        if (kind == 0) {
            ChainBody& B = c.body[bi];
            float dth[3];
            for (int r = 0; r < 3; ++r) { B.x[r] += static_cast<float>(xi[r]); dth[r] = static_cast<float>(xi[3 + r]); }
            if (cloth_finite3(dth)) q_spin(B.q, dth);
        } else {
            float* lj = lam + (kind == 1 ? (bi - 1u) * 6u : (nb - 1u) * 6u);
            for (int r = 0; r < 6; ++r) lj[r] -= static_cast<float>(xi[r]);
        }
    }
}

// One substep. s carries the carrier at this substep's end (the cloth's
// convention: chain_step sets it before the call, as cloth_step does
// for cloth_substep); f0 is its start.
inline void chain_substep(ClothState& s, ChainState& c, const ChainParams& pr, const PhysicsColliderView* cv,
                          size_t ncol, float h, const float* wind, const ClothFrame& f0, const ChainSub& u,
                          const ClothAffView* aff, size_t naff, float tback, const ClothGround* ground) {
    const size_t nb = c.body.size();
    if (nb == 0 || !(h > 0.0f)) return;
    const float rate = h / CLOTH_H;
    auto keep = [rate](float dial) {
        const float kk = 1.0f - (dial < 0.0f ? 0.0f : (dial > 1.0f ? 1.0f : dial));
        return rate == 1.0f ? kk : powf(kk, rate);
    };
    const float g = -CLOTH_G * pr.gravity;
    const float kdamp = keep(pr.damping);
    // CLOTH_DRAG: the media's drag per unit mass (1/s) - the dials are the whole chain's, as its mass is, and
    // a body takes both in the same share, so every body has the chain's rate.
    const float mchain = pr.mass > 1.0e-4f ? pr.mass : 1.0e-4f;
    const float cair = (pr.drag > 0.0f ? pr.drag : 0.0f) / mchain;
    const float cwat = (pr.water_drag > 0.0f ? pr.water_drag : 0.0f) / mchain;
    const float klin = keep(pr.lin_damping), kang = keep(pr.ang_damping);
    const float buoy = pr.buoyancy < 0.0f ? 0.0f : pr.buoyancy;
    const float vmax = pr.max_speed > 0.0f ? pr.max_speed : 1.0e6f;
    ClothFrame f1;
    for (int k = 0; k < 3; ++k) { f1.centre[k] = s.centre[k]; f1.size[k] = s.size[k]; }
    memcpy(f1.rot, s.rot, sizeof(f1.rot));
    f1.rotated = s.rotated;
    const bool cmove = !cloth_frame_same(f0, f1);
    const float one[3] = { 1.0f, 1.0f, 1.0f };
    // The carrier's own turn over this substep, as an angular velocity: what
    // ang_damping damps a body's spin toward.
    float cw[3] = { 0.0f, 0.0f, 0.0f };
    if (cmove && kang < 1.0f) {
        float q0[4], q1[4], rv[3];
        chain_frame_q(f0, q0);
        chain_frame_q(f1, q1);
        chain_rotvec(q1, q0, rv);
        for (int k = 0; k < 3; ++k) cw[k] = rv[k] / h;
    }

    // Integrate: the free bodies by their velocity and the forces on them, the
    // kinematic start (and nothing else) to where the carrier puts it.
    for (size_t i = 0; i < nb; ++i) {
        ChainBody& b = c.body[i];
        if (!(b.inv_m > 0.0f)) {
            chain_kin(c, i, u, u.t0, b.xp, b.qp);
            chain_kin(c, i, u, u.t1, b.x, b.q);
            continue;
        }
        float sub = (s.sea - b.x[CLOTH_UP]) * (1.0f / CLOTH_WATER_BAND);
        if (sub < 0.0f) sub = 0.0f; else if (sub > 1.0f) sub = 1.0f;
        float aair[3] = { 0.0f, 0.0f, 0.0f }, aacc[3] = { 0.0f, 0.0f, 0.0f }, adrag = 0.0f;
        if (naff != 0) cloth_aff_eval(aff, naff, b.x, tback, aair, aacc, adrag);
        // CLOTH_DRAG: the cloth's drag along every axis (a link has no facing): the air above the water,
        // which moves with the wind and the air affectors, and the still water below, each in its share
        // across the band, pull the body toward ue as one linear force at F / mass, exact over the substep -
        // a contraction whatever the dials. The spin is drawn down at the same rate.
        const float ra = (1.0f - sub) * (1.0f + adrag) * cair;
        const float rn = ra + sub * cwat;
        float ue[3] = { 0.0f, 0.0f, 0.0f };
        if (ra > 0.0f) {
            const float sh = ra / rn;
            for (int k = 0; k < 3; ++k) ue[k] = ((wind ? wind[k] * pr.wind : 0.0f) + aair[k]) * sh;
        }
        const float med = expf(-rn * h);
        for (int k = 0; k < 3; ++k) {
            b.v[k] = ue[k] + (b.v[k] - ue[k]) * med;
            b.w[k] *= med;
            b.v[k] += aacc[k] * h;
        }
        // The carrier's motion at this body: the reference both the damping
        // below and the speed clamp further down are taken against.
        float cd[3] = { 0.0f, 0.0f, 0.0f };
        if (cmove) {
            cloth_frame_delta(b.x, f0, one, f1, cd);
            for (int k = 0; k < 3; ++k) cd[k] /= h;
        }
        if (klin < 1.0f) for (int k = 0; k < 3; ++k) b.v[k] = cd[k] + (b.v[k] - cd[k]) * klin;
        if (kang < 1.0f) for (int k = 0; k < 3; ++k) b.w[k] = cw[k] + (b.w[k] - cw[k]) * kang;
        b.v[CLOTH_UP] += (g + sub * buoy * CLOTH_G) * h;
        {   // The gyroscopic term - Euler's equations with no external torque,
            // w += h Ib^-1 (-(w x Ib w)) in the body's frame (Mueller et al.
            // 2020, Algorithm 2) - so a spin about an axis that is not
            // principal carries its angular momentum, not its angular
            // velocity, through the step.
            float wb[3], iwb[3], cx[3], dwb[3];
            q_rot_inv(b.q, b.w, wb);
            for (int r = 0; r < 3; ++r) iwb[r] = b.ib[r * 3] * wb[0] + b.ib[r * 3 + 1] * wb[1] + b.ib[r * 3 + 2] * wb[2];
            cx[0] = wb[1] * iwb[2] - wb[2] * iwb[1];
            cx[1] = wb[2] * iwb[0] - wb[0] * iwb[2];
            cx[2] = wb[0] * iwb[1] - wb[1] * iwb[0];
            for (int r = 0; r < 3; ++r) dwb[r] = b.ib_inv[r * 3] * cx[0] + b.ib_inv[r * 3 + 1] * cx[1] + b.ib_inv[r * 3 + 2] * cx[2];
            for (int r = 0; r < 3; ++r) wb[r] -= h * dwb[r];
            q_rot(b.q, wb, b.w);
        }
        // The cloth's guard, against the carrier's own motion at the body (cd,
        // taken above). EA_CHAIN_GUARD: as the cloth's, only a body fast in
        // the world is tested, so one the carrier leaves standing is never
        // pulled along by the clamp (a still carrier: identical).
        if (cloth_v3_len(b.v) > vmax) {
            float rel[3];
            for (int k = 0; k < 3; ++k) rel[k] = b.v[k] - cd[k];
            const float rl = cloth_v3_len(rel);
            if (rl > vmax) for (int k = 0; k < 3; ++k) b.v[k] = cd[k] + rel[k] * (vmax / rl);
        }
        const float wl = cloth_v3_len(b.w);
        const float wmax = vmax / (b.radius > 0.05f ? b.radius : 0.05f);
        if (wl > wmax) for (int k = 0; k < 3; ++k) b.w[k] *= wmax / wl;
        memcpy(b.xp, b.x, sizeof(b.x));
        memcpy(b.qp, b.q, sizeof(b.q));
        for (int k = 0; k < 3; ++k) b.x[k] += b.v[k] * h;
        const float dw[3] = { b.w[0] * h, b.w[1] * h, b.w[2] * h };
        q_spin(b.q, dw);
    }

    const size_t nj = c.joint.size();
    if (c.lam.size() != (nj + 1u) * 6u) c.lam.assign((nj + 1u) * 6u, 0.0f);
    else std::fill(c.lam.begin(), c.lam.end(), 0.0f);
    if (c.lam_lim.size() != nj * 3u) c.lam_lim.assign(nj * 3u, 0u);
    else std::fill(c.lam_lim.begin(), c.lam_lim.end(), 0u);
    const float swing_lim = pr.swing_limit < 180.0f ? (pr.swing_limit > 0.0f ? pr.swing_limit : 0.0f) * 0.01745329f : -1.0f;
    const float twist_lim = pr.twist_limit < 180.0f ? (pr.twist_limit > 0.0f ? pr.twist_limit : 0.0f) * 0.01745329f : -1.0f;
    float endp[3], endq[4];
    const bool end_p = chain_end_pos(c, u, u.t1, endp);
    const bool end_q = chain_end_rot(c, u, u.t1, endq);
    // Taut, not torn: a target beyond what the chain can reach is pulled in to
    // the farthest point it CAN reach in that direction (ChainState::end_reach),
    // so the hold and the joints are satisfiable together. Ahead of the
    // approach, which then engages on the point actually held.
    // CHAIN_REACH_ROT: with the end's rotation held too, the held point stands off the end body's head by the
    // held orientation's image of its offset (end_pt less the head, chain_end_reach_rot), so what the point can
    // reach is the HEAD's sphere (the path's length to the head, about the pin moved by that image), inside the
    // straight chain's. A target between the two spheres is unreachable with both holds on (the end body cannot lie
    // along the chain and at its held orientation at once): the solve, every row rigid, would leave the joints open
    // (a CHAIN_RUNAWAY reset every CHAIN_RUNAWAY_S). TWIN: chain_end_taut (the draw's correction).
    if (end_p && c.end_root > 0) {   // CHAIN_REACH_ZERO: a reach of 0 clamps the target to the pin.
        float pin[3], cs_d[3];
        chain_point(c.body[0], c.joint[static_cast<size_t>(c.end_root) - 1u].ra, false, pin);
        const float cs_r = chain_end_reach_rot(c, end_q ? endq : nullptr, pin);
        cloth_v3_sub(endp, pin, cs_d);
        const float cs_l = cloth_v3_len(cs_d);
        if (cs_l > cs_r && cs_l > 1.0e-9f) {
            const float cs_sc = cs_r / cs_l;
            for (int k = 0; k < 3; ++k) endp[k] = pin[k] + cs_d[k] * cs_sc;
        }
    }
    // The end's approach (ChainState::seek_p): engaged from where the end
    // stands, closed at a bounded rate.
    if (end_p && c.end_body > 0) {
        const ChainBody& E = c.body[static_cast<size_t>(c.end_body)];
        if (!c.seek_p_on) {
            float ph[3];
            chain_point(E, c.end_pt, false, ph);   // CHAIN_POINT.
            cloth_v3_sub(ph, endp, c.seek_p);
            c.seek_p_on = true;
        } else {
            const float sl = cloth_v3_len(c.seek_p), st = CHAIN_SEEK_MS * h;
            const float sc = sl > st ? (sl - st) / sl : 0.0f;
            for (int k = 0; k < 3; ++k) c.seek_p[k] *= sc;
        }
        for (int k = 0; k < 3; ++k) endp[k] += c.seek_p[k];
    } else {
        c.seek_p_on = false;
    }
    if (end_q && c.end_body > 0) {
        const ChainBody& E = c.body[static_cast<size_t>(c.end_body)];
        if (!c.seek_q_on) {
            const float tc[4] = { -endq[0], -endq[1], -endq[2], endq[3] };
            q_mul(E.q, tc, c.seek_q);
            q_norm(c.seek_q);
            c.seek_q_on = true;
        } else {
            if (c.seek_q[3] < 0.0f) for (int k = 0; k < 4; ++k) c.seek_q[k] = -c.seek_q[k];
            const float sn = sqrtf(c.seek_q[0] * c.seek_q[0] + c.seek_q[1] * c.seek_q[1] + c.seek_q[2] * c.seek_q[2]);
            const float ang = 2.0f * atan2f(sn, c.seek_q[3]), st = CHAIN_SEEK_RAD * h;
            if (ang <= st || !(sn > 1.0e-9f)) {
                c.seek_q[0] = c.seek_q[1] = c.seek_q[2] = 0.0f; c.seek_q[3] = 1.0f;
            } else {
                const float na = 0.5f * (ang - st), f = sinf(na) / sn;
                for (int k = 0; k < 3; ++k) c.seek_q[k] *= f;
                c.seek_q[3] = cosf(na);
            }
        }
        float qq[4];
        q_mul(c.seek_q, endq, qq);
        q_norm(qq);
        memcpy(endq, qq, sizeof(qq));
    } else {
        c.seek_q_on = false;
    }
    const uint32_t iters = pr.iterations < 1 ? 1u : static_cast<uint32_t>(pr.iterations);
    for (uint32_t it = 0; it < iters; ++it) {
        // Every joint, its limits and the end's hold at once (chain_joint_solve).
        chain_joint_solve(c, pr, h, end_p, endp, end_q, endq, swing_lim, twist_lim, c.solve_wk, c.lam.data());
        // The geometry against the world: every collider, then the terrain -
        // the cloth's order (cloth_substep), so a point pressed between a
        // collider and the ground ends the pass on the ground's side, never
        // under the terrain the player sees.
        for (size_t i = 0; i < nb; ++i) {
            ChainBody& b = c.body[i];
            if (!(b.inv_m > 0.0f) || b.sn == 0u) continue;
            for (uint32_t si = 0; si < b.sn; ++si) {
                const float* r = &c.samp[static_cast<size_t>(b.s0 + si) * 3u];
                for (size_t ci = 0; ci < ncol; ++ci) {
                    const PhysicsColliderView& cc = cv[ci];
                    float p[3], p0[3], m[3], cn[3], sv[3] = { 0.0f, 0.0f, 0.0f };
                    chain_point(b, r, false, p);
                    if (!cc.inside) {   // Broadphase: this body against the collider's sphere.
                        float dd[3];
                        cloth_v3_sub(b.x, cc.bs_c, dd);
                        // + 1 m: slack - a broadphase only rejects, so a larger sphere costs tests, never contacts.
                        const float rr = cc.bs_r + b.radius + pr.thickness + cc.margin + 1.0f;
                        if (cloth_v3_dot(dd, dd) > rr * rr) continue;
                    }
                    chain_point(b, r, true, p0);
                    memcpy(m, p, sizeof(p));
                    if (physics_collide_one(cc, pr.thickness, p0, m, cn, sv)) {
                        float d[3];
                        cloth_v3_sub(m, p, d);
                        chain_contact(b, r, d, sv, pr.friction * cc.friction);
                    }
                }
                if (ground && ground->on) {
                    float p[3], m[3], gn[3];
                    chain_point(b, r, false, p);
                    memcpy(m, p, sizeof(p));
                    if (cloth_ground_contact(*ground, pr.thickness, m, gn)) {
                        float d[3];
                        cloth_v3_sub(m, p, d);
                        chain_contact(b, r, d, nullptr, pr.friction);
                    }
                }
            }
        }
    }
    // Velocities from the motion, then the joints' relative spin damped.
    const float ih = 1.0f / h;
    for (size_t i = 0; i < nb; ++i) {
        ChainBody& b = c.body[i];
        for (int k = 0; k < 3; ++k) b.v[k] = (b.x[k] - b.xp[k]) * ih;
        const float cq[4] = { -b.qp[0], -b.qp[1], -b.qp[2], b.qp[3] };
        float dq[4];
        q_mul(b.q, cq, dq);
        const float sg = dq[3] < 0.0f ? -2.0f : 2.0f;
        for (int k = 0; k < 3; ++k) b.w[k] = dq[k] * sg * ih;
    }
    if (kdamp < 1.0f) {
        for (size_t jj = 0; jj < nj; ++jj) {
            const ChainJoint& j = c.joint[jj];
            ChainBody& A = c.body[j.a];
            ChainBody& B = c.body[j.b];
            float dw[3];
            cloth_v3_sub(B.w, A.w, dw);
            const float l = cloth_v3_len(dw);
            if (!(l > 1.0e-9f)) continue;
            const float n[3] = { dw[0] / l, dw[1] / l, dw[2] / l };
            const float w = chain_wrot(A, n) + chain_wrot(B, n);
            if (!(w > 0.0f)) continue;
            const float p = l * (1.0f - kdamp) / w;
            const float pn[3] = { n[0] * p, n[1] * p, n[2] * p };
            float ia[3], ib[3];
            chain_iw(A, pn, ia);
            chain_iw(B, pn, ib);
            for (int k = 0; k < 3; ++k) {
                if (A.inv_m > 0.0f) A.w[k] += ia[k];
                if (B.inv_m > 0.0f) B.w[k] -= ib[k];
            }
        }
    }
}

// CHAIN_RUNAWAY - a FINITE divergence, which chain_step's non-finite rescue never sees. A joint solve that
// diverges (too few passes for the chain's length, a tight swing limit under brisk motion, a cold start on a fast
// carrier) leaves rigid joints metres apart and links at hundreds of m/s for good; maxSpeed cannot hold it (it clamps
// the velocity going INTO a substep, not what the solve makes of it). A rigid joint (stiffness 1, chain_alpha's
// exact rows) that converged stands closed to rounding; one whose anchors stand more than half its span apart (the
// parent's centre of mass to the head to the child's), and never under CHAIN_RUNAWAY_GAP, is open. A jolt opens
// joints for a fraction of a second, so only persistence counts: ChainState::runaway_t gains each step a joint
// stands open and loses as much each step none does; past CHAIN_RUNAWAY_S the chain is diverged. A soft chain
// (stiffness < 1) stretches legitimately under its own weight and is never judged.
// WARNING: a chain whose holds CANNOT be met together trips this for good (a reset every CHAIN_RUNAWAY_S, seen as
// a pop): a held rotation that a swing / twist limit forbids, WITH a held position (the reach clamp,
// CHAIN_REACH_ROT, assumes the chain straight; the position rows cannot close). The reachable set under limits has
// no closed form and a compliant hold would change every held chain's response, so the author is warned instead, in
// the script documentation (chainSimulation's endRotation / swingLimit).
static constexpr float CHAIN_RUNAWAY_GAP = 0.02f;   // m.
static constexpr float CHAIN_RUNAWAY_S = 6.0f;      // s.
inline bool chain_open(const ChainState& c, const ChainParams& pr) {
    if (!(pr.stretch >= 0.999f)) return false;
    for (const ChainJoint& j : c.joint) {
        float pa[3], pb[3], g[3];
        chain_point(c.body[j.a], j.ra, false, pa);
        chain_point(c.body[j.b], j.rb, false, pb);
        cloth_v3_sub(pb, pa, g);
        const float sp = 0.5f * (cloth_v3_len(j.ra) + cloth_v3_len(j.rb));
        if (cloth_v3_len(g) > (sp > CHAIN_RUNAWAY_GAP ? sp : CHAIN_RUNAWAY_GAP)) return true;
    }
    return false;
}

// One frame of one chain: the cloth's step discipline (cloth_step) around chain_substep. g is this frame's
// skeleton (skin_affine's layout, nbone bones) or null; tg the end's targets.
inline void chain_step(ClothState& s, ChainState& c, const ChainParams& pr, const PhysicsColliderView* col,
                       size_t ncol, const float* centre, const float* rot, bool rotated, const float* size,
                       float dt, const float* wind, const float* g, size_t nbone, const ChainTarget& tg,
                       const ClothAffView* aff, size_t naff, const ClothGround* ground) {
    KH_PROF_SCOPE(PROF_W_CHAIN_SIM);   // PROF.
    if (c.body.empty()) return;
    if (!(dt > 0.0f)) dt = 0.0f;
    const float dt_raw = dt;   // DT_RAW: the cloth's rule (cloth_step) - the velocity over the frame it took.
    if (dt > CLOTH_DT_MAX) dt = CLOTH_DT_MAX;
    // SUB_CLOCK: the cloth's rule (cloth_step) - fo is where the simulation stands (the state's frame, the
    // substeps' start and the rebase's source), fe the last frame's end, which the frame's motion is measured from.
    ClothFrame fo, fn, fe = s.fend;
    for (int k = 0; k < 3; ++k) { fo.centre[k] = s.centre[k]; fo.size[k] = s.size[k]; fn.centre[k] = centre[k]; fn.size[k] = size[k]; }
    memcpy(fo.rot, s.rot, sizeof(fo.rot));
    fo.rotated = s.rotated;
    memcpy(fn.rot, rot, sizeof(fn.rot));
    fn.rotated = rotated;
    const bool guided = g != nullptr && nbone != 0u;
    const float* g0 = guided && c.guide_prev.size() == nbone * 12u ? c.guide_prev.data() : g;
    bool reset = !s.primed;
    bool rebase = false;
    if (!reset) {
        ClothFrame pred = fe;
        if (s.carrier_vok) for (int k = 0; k < 3; ++k) pred.centre[k] += s.carrier_v[k] * dt;
        rebase = cloth_frame_jumped(pred, fn, pr.teleport);
        // TELEPORT_ONCE: the cloth's rule (cloth_step).
        if (rebase && s.carrier_vok && s.carrier_vreb && !cloth_frame_jumped(fe, fn, pr.teleport)) rebase = false;
        if (dt > 0.0f) {
            for (int k = 0; k < 3; ++k) s.carrier_v[k] = (fn.centre[k] - fe.centre[k]) / dt_raw;   // DT_RAW.
            s.carrier_vok = true;
            s.carrier_vreb = rebase;   // TELEPORT_ONCE.
        }
    }
    if (rebase) {
        // A teleport is not motion: every body arrives with the carrier, its pose against the carrier kept. The
        // end's held place stays in the world, where it was held.
        float qo[4], qn[4], qd[4];
        chain_frame_q(fo, qo);
        chain_frame_q(fn, qn);
        const float qoc[4] = { -qo[0], -qo[1], -qo[2], qo[3] };
        q_mul(qn, qoc, qd);
        for (ChainBody& b : c.body) {
            for (int pass = 0; pass < 2; ++pass) {
                float* x = pass == 0 ? b.x : b.xp;
                float* q = pass == 0 ? b.q : b.qp;
                float l[3], d[3], w[3];
                cloth_v3_sub(x, fo.centre, d);
                q_rot_inv(qo, d, l);
                q_rot(qn, l, w);
                for (int k = 0; k < 3; ++k) x[k] = fn.centre[k] + w[k];
                float r[4];
                q_mul(qd, q, r);
                q_norm(r);
                memcpy(q, r, sizeof(r));
            }
            q_rot(qd, b.v, b.v);
            q_rot(qd, b.w, b.w);
        }
        fo = fn;
        fe = fn;
        c.prev_pos = 0u;
        c.prev_rot = 0u;
        c.seek_p_on = false;
        c.seek_q_on = false;
        g0 = g;
    }
    // The end's 'false' targets take the pose the chain first stood in.
    ChainTarget t = tg;
    auto capture = [&]() {
        if (c.end_body < 0 || c.hold_on) return;
        const ChainBody& e = c.body[static_cast<size_t>(c.end_body)];
        float d[3] = { 0.0f, 0.0f, 0.0f };
        if (e.parent >= 0) q_rot(e.q, c.end_pt, d);   // CHAIN_POINT.
        for (int k = 0; k < 3; ++k) c.hold_p[k] = e.x[k] + d[k];
        memcpy(c.hold_q, e.q, sizeof(c.hold_q));
        c.hold_on = true;
    };
    if (reset) {
        // Placed, and nothing more this frame: the frame it arrives from is not one it moved across (the cloth's
        // rule, cloth_step). SUB_CLOCK: at the frame's end, with nothing left over.
        for (int k = 0; k < 3; ++k) { s.centre[k] = centre[k]; s.size[k] = size[k]; }
        memcpy(s.rot, rot, sizeof(s.rot));
        s.rotated = rotated;
        s.fend = fn;
        s.sub_t = 1.0f;
        chain_reset(s, c, g, nbone);
        s.accum = 0.0f;
        s.primed = true;
        s.carrier_vok = false;
        s.carrier_vreb = false;   // TELEPORT_ONCE.
        if (t.pos_hold || t.rot_hold) capture();
        c.prev_pos = 0u;
        c.prev_rot = 0u;
        c.seek_p_on = false;
        c.seek_q_on = false;
        s.speed_mean = 0.0f;
        s.reach = 0.0f;
        if (guided) c.guide_prev.assign(g, g + nbone * 12u);
        else c.guide_prev.clear();
        c.guide_end = c.guide_prev;   // GUIDE_END.
        return;
    }
    if (t.pos_hold || t.rot_hold) capture();
    if (t.pos_hold) { if (c.hold_on) { t.pos = 2u; memcpy(t.p, c.hold_p, sizeof(t.p)); } else t.pos = 0u; }
    if (t.rot_hold) { if (c.hold_on) { t.rot = 2u; q_to_axes(c.hold_q, t.r); } else t.rot = 0u; }
    float tq[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (t.rot == 2u) q_from_axes(t.r, tq);
    // Whether the carrier or the skeleton's pose moved over this step, taken before the loop walks the frames to
    // their end: a chain may park only when neither did (the sleep test below).
    // GUIDE_END: against the last step's frame-end pose as given (guide_prev is the substeps' blended start).
    const bool held_moved = !cloth_frame_same(fe, fn) ||   // SUB_CLOCK: frame to frame.
                            (guided && c.guide_end.size() == nbone * 12u &&
                             memcmp(c.guide_end.data(), g, nbone * 12u * sizeof(float)) != 0);

    const int want = pr.substeps < 1 ? 1 : (pr.substeps > CLOTH_MAX_SUBSTEP ? static_cast<int>(CLOTH_MAX_SUBSTEP)
                                                                             : static_cast<int>(pr.substeps));
    const float h = 1.0f / (60.0f * static_cast<float>(want));
    s.accum += dt;
    if (s.accum > CLOTH_DT_MAX) s.accum = CLOTH_DT_MAX;
    const uint32_t nmax = static_cast<uint32_t>(CLOTH_DT_MAX / h) + 1u;
    uint32_t n = 0;
    float left = s.accum;
    for (; left >= h && n < nmax; left -= h) ++n;
    // SUB_CLOCK: the share of the step's span (fo to fn) the substeps cover - the cloth's rule, cloth_step.
    const float cov = n == 0u ? 0.0f : 1.0f - left / s.accum;
    if (s.col_sub.size() < ncol) s.col_sub.resize(ncol);
    PhysicsColliderView* const cv = s.col_sub.data();
    ChainSub u;
    u.old_f = &fo;
    u.new_f = &fn;
    u.g0 = guided ? g0 : nullptr;
    u.g1 = guided ? g : nullptr;
    u.nbone = nbone;
    u.tg = &t;
    ClothFrame f0 = fo;
    uint32_t ran = 0;
    while (ran < n) {
        const float t0 = static_cast<float>(ran) / static_cast<float>(n) * cov;
        const float t1 = static_cast<float>(ran + 1u) / static_cast<float>(n) * cov;
        ClothFrame fs;
        cloth_frame_lerp(fo, fn, t1, fs);
        for (int k = 0; k < 3; ++k) { s.centre[k] = fs.centre[k]; s.size[k] = fs.size[k]; }
        memcpy(s.rot, fs.rot, sizeof(s.rot));
        s.rotated = fs.rotated;
        // The colliders across the substeps, exactly as cloth_step moves them.
        for (size_t ci = 0; ci < ncol; ++ci) {
            const PhysicsColliderView& src = col[ci];
            ClothFrame cur;
            for (int k = 0; k < 3; ++k) { cur.centre[k] = src.centre[k]; cur.size[k] = src.size[k]; }
            memcpy(cur.rot, src.rot, sizeof(cur.rot));
            cur.rotated = src.rotated;
            const bool pv = src.prev.size[0] != 0.0f || src.prev.size[1] != 0.0f || src.prev.size[2] != 0.0f;
            const ClothFrame& p0 = pv ? src.prev : cur;
            ClothFrame fa, fb;
            cloth_frame_lerp(p0, cur, t0, fa);
            cloth_frame_lerp(p0, cur, t1, fb);
            PhysicsColliderView& v = cv[ci];
            v = src;
            float bs = 0.0f;
            for (int k = 0; k < 3; ++k) {
                v.centre[k] = fb.centre[k];
                v.bs_c[k] = fb.centre[k];
                const float e = fabsf(fb.size[k]) < 1.0e-6f ? 1.0e-6f : fb.size[k];
                v.size[k] = e;
                v.inv_size[k] = 1.0f / e;
                bs += 0.25f * e * e;
            }
            memcpy(v.rot, fb.rot, sizeof(v.rot));
            v.rotated = fb.rotated;
            v.bs_r = sqrtf(bs);
            v.prev = fa;
            v.def0 = src.deform ? t0 - 1.0f : 0.0f;
            v.def1 = src.deform ? t1 - 1.0f : 0.0f;
            if (src.deform && src.bs_r > v.bs_r) v.bs_r = src.bs_r;
        }
        u.t0 = t0;
        u.t1 = t1;
        // EA_TURB_CLOCK: this substep's end before the frame's end, leftover included (SUB_CLOCK; the cloth's).
        chain_substep(s, c, pr, cv, ncol, h, wind, f0, u, aff, naff, static_cast<float>(n - 1u - ran) * h + left,
                      ground);
        s.accum -= h;
        f0 = fs;
        ++ran;
    }
    // SUB_CLOCK: the state's frame is where the substeps ended (the cloth's rule, cloth_step), the writeback
    // is relative to it and the frame's end stands beside it. A frame shorter than a substep advances nothing and
    // moves nothing: the start stands against that frame, and the draw carries the chain with the object.
    for (int k = 0; k < 3; ++k) { s.centre[k] = f0.centre[k]; s.size[k] = f0.size[k]; }
    memcpy(s.rot, f0.rot, sizeof(s.rot));
    s.rotated = f0.rotated;
    s.fend = fn;
    s.sub_t = cov;
    // The rescue: a non-finite body puts the whole chain back where the carrier holds it.
    bool bad = false;
    for (const ChainBody& b : c.body) {
        if (!cloth_finite3(b.x) || !cloth_finite3(b.v) || !cloth_finite3(b.w) || !(b.q[3] == b.q[3])) { bad = true; break; }
    }
    // CHAIN_RUNAWAY: so does a finite divergence (chain_open, persisting past CHAIN_RUNAWAY_S).
    bool runaway = false;
    if (!bad && ran != 0u) {
        if (chain_open(c, pr)) c.runaway_t += dt;
        else c.runaway_t = c.runaway_t > dt ? c.runaway_t - dt : 0.0f;
        runaway = c.runaway_t > CHAIN_RUNAWAY_S;
    }
    if (bad || runaway) {
        chain_reset(s, c, g, nbone);
        // The end approach starts again from where the end now stands (as the cold reset): a remainder the approach
        // had nearly closed would put the whole gap to the held target into one substep. (prev_pos / prev_rot are this
        // step's targets' kinds, set just below on every path.)
        c.seek_p_on = false;
        c.seek_q_on = false;
        c.runaway_t = 0.0f;
    }
    // Remembered for the next step's interpolation. SUB_CLOCK: where the substeps left each lane - cov of the way
    // from its start to its end (as chain_bone_map and chain_end_pos / _rot blend them), the end itself when
    // nothing was left over, the start when none ran - so the next step's substeps run on from there.
    if (guided) c.guide_end.assign(g, g + nbone * 12u);   // GUIDE_END: this frame's end, as given.
    else c.guide_end.clear();
    if (!guided) c.guide_prev.clear();
    else if (!(cov < 1.0f) || g0 == g) c.guide_prev.assign(g, g + nbone * 12u);
    else if (cov > 0.0f) {
        for (size_t bi = 0; bi < nbone; ++bi) {
            float* m = &c.guide_prev[bi * 12u];
            const float* mg = g + bi * 12u;
            float qa[4], qb[4], q[4];
            q_from_axes(m, qa);
            q_from_axes(mg, qb);
            q_nlerp(qa, qb, cov, q);
            q_to_axes(q, m);
            for (int k = 0; k < 3; ++k) m[9 + k] = m[9 + k] + (mg[9 + k] - m[9 + k]) * cov;
        }
    }
    if (t.pos == 2u) {
        if (c.prev_pos == 2u && cov < 1.0f) {
            for (int k = 0; k < 3; ++k) c.prev_p[k] = c.prev_p[k] + (t.p[k] - c.prev_p[k]) * cov;
        } else {
            memcpy(c.prev_p, t.p, sizeof(c.prev_p));
        }
    }
    if (t.rot == 2u) {
        if (c.prev_rot == 2u && cov < 1.0f) {
            float q[4];
            q_nlerp(c.prev_q, tq, cov, q);
            memcpy(c.prev_q, q, sizeof(c.prev_q));
        } else {
            memcpy(c.prev_q, tq, sizeof(c.prev_q));
        }
    }
    c.prev_pos = t.pos == 2u ? 2u : 0u;
    c.prev_rot = t.rot == 2u ? 2u : 0u;
    // The sleep test over the free bodies; the reach over every body, the kinematic start included.
    double sum = 0.0;
    uint32_t nfree = 0;
    float reach = 0.0f;
    for (const ChainBody& b : c.body) {
        float r[3];
        cloth_v3_sub(b.x, s.centre, r);
        const float rr = cloth_v3_len(r) + b.radius;
        if (rr > reach) reach = rr;
        if (!(b.inv_m > 0.0f)) continue;
        sum += cloth_v3_len(b.v);
        ++nfree;
    }
    s.speed_mean = nfree ? static_cast<float>(sum / static_cast<double>(nfree)) : 0.0f;
    s.reach = reach;
    if (!held_moved && ran != 0u && pr.sleep > 0.0f && s.speed_mean < pr.sleep) {
        for (ChainBody& b : c.body) {
            for (int k = 0; k < 3; ++k) { b.v[k] = 0.0f; b.w[k] = 0.0f; }
        }
        s.speed_mean = 0.0f;
    }
}
}