#ifndef VECTOR_OPERATIONS_HPP
#define VECTOR_OPERATIONS_HPP

/* This file depends on common_defines.hpp being included first */
#ifndef COMMON_DEFINES_HPP
#error "vector_operations.hpp requires common_defines.hpp to be included first"
#endif

#include <ctype.h>
#include <math.h>

/* Constants */
#define MAX_VECTOR_ARGS 4

/* Data structures */
typedef struct {
    double x, y, z;
} vector3_t;

typedef struct {
    double x, y;
} vector2_t;

typedef struct {
    double w, x, y, z;  /* w is scalar component, x,y,z are vector components */
} quaternion_t;

typedef struct {
    double pitch, yaw, roll;  /* Euler angles in degrees */
} euler_t;

/* Vector function types for dispatch table */
typedef enum {
    VEC_FUNC_V3_TO_SCALAR,    /* Vector3 -> Scalar */
    VEC_FUNC_V3V3_TO_SCALAR,  /* Vector3, Vector3 -> Scalar */
    VEC_FUNC_V3_TO_V3,        /* Vector3 -> Vector3 */
    VEC_FUNC_V3V3_TO_V3,      /* Vector3, Vector3 -> Vector3 */
    VEC_FUNC_V3S_TO_V3,       /* Vector3, Scalar -> Vector3 */
    VEC_FUNC_V3V3S_TO_V3,     /* Vector3, Vector3, Scalar -> Vector3 */
    VEC_FUNC_EULER_TO_QUAT,   /* Euler -> Quaternion */
    VEC_FUNC_QUAT_TO_EULER,   /* Quaternion -> Euler */
    VEC_FUNC_QUAT_TO_QUAT,    /* Quaternion -> Quaternion */
    VEC_FUNC_QUATQUAT_TO_QUAT,/* Quaternion, Quaternion -> Quaternion */
    VEC_FUNC_V3QUAT_TO_V3,    /* Vector3, Quaternion -> Vector3 */
    VEC_FUNC_SPECIAL          /* Special handling required */
} vector_func_type_t;

/* Vector function definition structure */
typedef struct {
    const char* name;
    vector_func_type_t type;
    int arg_count;
    void* func_ptr;
    const char* description;
} vector_function_t;

/* Vector3 utility functions */
static inline double kh_vec3_magnitude(vector3_t v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline vector3_t kh_vec3_normalize(vector3_t v) {
    double mag = kh_vec3_magnitude(v);
    if (mag < VECTOR_EPSILON) {
        vector3_t zero = {0.0, 0.0, 0.0};
        return zero;
    }
    vector3_t result = {v.x / mag, v.y / mag, v.z / mag};
    return result;
}

static inline double kh_vec3_dot(vector3_t a, vector3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vector3_t kh_vec3_cross(vector3_t a, vector3_t b) {
    vector3_t result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

static inline vector3_t kh_vec3_add(vector3_t a, vector3_t b) {
    vector3_t result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}

static inline vector3_t kh_vec3_subtract(vector3_t a, vector3_t b) {
    vector3_t result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

static inline vector3_t kh_vec3_multiply(vector3_t v, double scalar) {
    vector3_t result = {v.x * scalar, v.y * scalar, v.z * scalar};
    return result;
}

static inline vector3_t kh_vec3_divide(vector3_t v, double scalar) {
    if (fabs(scalar) < VECTOR_EPSILON) {
        vector3_t zero = {0.0, 0.0, 0.0};
        return zero;
    }
    vector3_t result = {v.x / scalar, v.y / scalar, v.z / scalar};
    return result;
}

static inline double kh_vec3_distance(vector3_t a, vector3_t b) {
    vector3_t diff = kh_vec3_subtract(a, b);
    return kh_vec3_magnitude(diff);
}

static inline double kh_vec3_angle(vector3_t a, vector3_t b) {
    double dot_product = kh_vec3_dot(a, b);
    double mag_a = kh_vec3_magnitude(a);
    double mag_b = kh_vec3_magnitude(b);
    
    if (mag_a < VECTOR_EPSILON || mag_b < VECTOR_EPSILON) {
        return 0.0;
    }
    
    double cos_angle = dot_product / (mag_a * mag_b);
    /* Clamp to valid domain for acos */
    if (cos_angle > 1.0) cos_angle = 1.0;
    if (cos_angle < -1.0) cos_angle = -1.0;
    
    return acos(cos_angle) * 180.0 / MATH_PI; /* Return in degrees */
}

static inline vector3_t kh_vec3_lerp(vector3_t a, vector3_t b, double t) {
    /* Clamp t to [0, 1] */
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    
    vector3_t result;
    result.x = a.x + t * (b.x - a.x);
    result.y = a.y + t * (b.y - a.y);
    result.z = a.z + t * (b.z - a.z);
    return result;
}

static inline vector3_t kh_vec3_project(vector3_t a, vector3_t b) {
    double dot_ab = kh_vec3_dot(a, b);
    double dot_bb = kh_vec3_dot(b, b);
    
    if (dot_bb < VECTOR_EPSILON) {
        vector3_t zero = {0.0, 0.0, 0.0};
        return zero;
    }
    
    double scalar = dot_ab / dot_bb;
    return kh_vec3_multiply(b, scalar);
}

static inline vector3_t kh_vec3_reject(vector3_t a, vector3_t b) {
    vector3_t projection = kh_vec3_project(a, b);
    return kh_vec3_subtract(a, projection);
}

static inline vector3_t kh_vec3_reflect(vector3_t incident, vector3_t normal) {
    vector3_t norm_normal = kh_vec3_normalize(normal);
    double dot_product = kh_vec3_dot(incident, norm_normal);
    vector3_t scaled_normal = kh_vec3_multiply(norm_normal, 2.0 * dot_product);
    return kh_vec3_subtract(incident, scaled_normal);
}

/* Quaternion utility functions */
static inline double kh_quat_magnitude(quaternion_t q) {
    return sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
}

static inline quaternion_t kh_quat_normalize(quaternion_t q) {
    double mag = kh_quat_magnitude(q);
    if (mag < VECTOR_EPSILON) {
        quaternion_t identity = {1.0, 0.0, 0.0, 0.0};
        return identity;
    }
    quaternion_t result = {q.w / mag, q.x / mag, q.y / mag, q.z / mag};
    return result;
}

static inline quaternion_t kh_quat_conjugate(quaternion_t q) {
    quaternion_t result = {q.w, -q.x, -q.y, -q.z};
    return result;
}

static inline quaternion_t kh_quat_multiply(quaternion_t a, quaternion_t b) {
    quaternion_t result;
    result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return result;
}

static inline quaternion_t kh_quat_inverse(quaternion_t q) {
    double mag_squared = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    if (mag_squared < VECTOR_EPSILON) {
        quaternion_t identity = {1.0, 0.0, 0.0, 0.0};
        return identity;
    }
    
    quaternion_t conjugate = kh_quat_conjugate(q);
    quaternion_t result = {
        conjugate.w / mag_squared,
        conjugate.x / mag_squared,
        conjugate.y / mag_squared,
        conjugate.z / mag_squared
    };
    return result;
}

static inline quaternion_t kh_euler_to_quaternion(euler_t euler) {
    /* Convert degrees to radians */
    double pitch_rad = euler.pitch * MATH_PI / 180.0;
    double yaw_rad = euler.yaw * MATH_PI / 180.0;
    double roll_rad = euler.roll * MATH_PI / 180.0;
    
    /* Half angles */
    double cp = cos(pitch_rad * 0.5);
    double sp = sin(pitch_rad * 0.5);
    double cy = cos(yaw_rad * 0.5);
    double sy = sin(yaw_rad * 0.5);
    double cr = cos(roll_rad * 0.5);
    double sr = sin(roll_rad * 0.5);
    
    /* ZYX Euler angle to quaternion conversion */
    quaternion_t q;
    q.w = cr * cy * cp + sr * sy * sp;
    q.x = cr * cy * sp - sr * sy * cp;
    q.y = cr * sy * cp + sr * cy * sp;
    q.z = sr * cy * cp - cr * sy * sp;
    
    return kh_quat_normalize(q);
}

static inline euler_t kh_quaternion_to_euler(quaternion_t q) {
    q = kh_quat_normalize(q);
    
    euler_t euler;
    
    /* Convert quaternion to ZYX Euler angles */
    
    /* Pitch (X-axis rotation) */
    double sinp = 2.0 * (q.w * q.x + q.y * q.z);
    if (fabs(sinp) >= 1.0) {
        euler.pitch = copysign(90.0, sinp);
    } else {
        euler.pitch = asin(sinp) * 180.0 / MATH_PI;
    }
    
    /* Yaw (Y-axis rotation) */
    double siny_cosp = 2.0 * (q.w * q.y - q.z * q.x);
    double cosy_cosp = 1.0 - 2.0 * (q.x * q.x + q.y * q.y);
    euler.yaw = atan2(siny_cosp, cosy_cosp) * 180.0 / MATH_PI;
    
    /* Roll (Z-axis rotation) */
    double sinr_cosp = 2.0 * (q.w * q.z + q.x * q.y);
    double cosr_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
    euler.roll = atan2(sinr_cosp, cosr_cosp) * 180.0 / MATH_PI;
    
    return euler;
}

static inline vector3_t kh_quat_rotate_vector(vector3_t v, quaternion_t q) {
    q = kh_quat_normalize(q);
    
    /* Convert vector to quaternion */
    quaternion_t v_quat = {0.0, v.x, v.y, v.z};
    
    /* Rotate: result = q * v * q_conjugate */
    quaternion_t q_conj = kh_quat_conjugate(q);
    quaternion_t temp = kh_quat_multiply(q, v_quat);
    quaternion_t result_quat = kh_quat_multiply(temp, q_conj);
    
    vector3_t result = {result_quat.x, result_quat.y, result_quat.z};
    return result;
}

static inline quaternion_t kh_quat_slerp(quaternion_t a, quaternion_t b, double t) {
    /* Clamp t to [0, 1] */
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    
    a = kh_quat_normalize(a);
    b = kh_quat_normalize(b);
    
    double dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    
    /* If dot product is negative, slerp won't take the shorter path */
    if (dot < 0.0) {
        b.w = -b.w;
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        dot = -dot;
    }
    
    /* If quaternions are very close, use linear interpolation */
    if (dot > 0.9995) {
        quaternion_t result;
        result.w = a.w + t * (b.w - a.w);
        result.x = a.x + t * (b.x - a.x);
        result.y = a.y + t * (b.y - a.y);
        result.z = a.z + t * (b.z - a.z);
        return kh_quat_normalize(result);
    }
    
    double theta_0 = acos(fabs(dot));
    double theta = theta_0 * t;
    double sin_theta = sin(theta);
    double sin_theta_0 = sin(theta_0);
    
    double s0 = cos(theta) - dot * sin_theta / sin_theta_0;
    double s1 = sin_theta / sin_theta_0;
    
    quaternion_t result;
    result.w = s0 * a.w + s1 * b.w;
    result.x = s0 * a.x + s1 * b.x;
    result.y = s0 * a.y + s1 * b.y;
    result.z = s0 * a.z + s1 * b.z;
    
    return kh_quat_normalize(result);
}

/* Enhanced input parsing functions with better error handling */
static int kh_parse_vector3(const char* input, vector3_t* result) {
    if (!input || !result) return 0;
    
    const char* ptr = input;
    double values[4]; /* Allocate space for 4 to detect over-parsing */
    int value_count = 0;
    
    /* Skip whitespace and opening bracket */
    while (*ptr && (isspace(*ptr) || *ptr == '[')) ptr++;
    
    /* Parse up to 4 numbers to detect if there are too many */
    while (*ptr && value_count < 4) {
        char* end_ptr;
        values[value_count] = strtod(ptr, &end_ptr);
        
        if (end_ptr == ptr) break; /* No number found */
        
        value_count++;
        ptr = end_ptr;
        
        /* Skip comma and whitespace */
        while (*ptr && (isspace(*ptr) || *ptr == ',')) ptr++;
    }
    
    /* Check for exactly 3 values */
    if (value_count == 3) {
        /* Verify no more numbers exist */
        while (*ptr && (isspace(*ptr) || *ptr == ',' || *ptr == ']')) ptr++;
        if (*ptr != '\0') {
            /* Check if there's another number */
            char* end_ptr;
            strtod(ptr, &end_ptr);
            if (end_ptr != ptr) return 0; /* Found additional number */
        }
        
        result->x = values[0];
        result->y = values[1];
        result->z = values[2];
        return 1;
    }
    
    return 0;
}

static int kh_parse_quaternion(const char* input, quaternion_t* result) {
    if (!input || !result) return 0;
    
    const char* ptr = input;
    double values[4];
    int value_count = 0;
    
    /* Skip whitespace and opening bracket */
    while (*ptr && (isspace(*ptr) || *ptr == '[')) ptr++;
    
    /* Parse up to 4 numbers */
    while (*ptr && value_count < 4) {
        char* end_ptr;
        values[value_count] = strtod(ptr, &end_ptr);
        
        if (end_ptr == ptr) break;
        
        value_count++;
        ptr = end_ptr;
        
        /* Skip comma and whitespace */
        while (*ptr && (isspace(*ptr) || *ptr == ',')) ptr++;
    }
    
    if (value_count == 4) {
        result->w = values[0];
        result->x = values[1];
        result->y = values[2];
        result->z = values[3];
        return 1;
    }
    
    return 0;
}

static int kh_parse_euler(const char* input, euler_t* result) {
    if (!input || !result) return 0;
    
    const char* ptr = input;
    double values[3];
    int value_count = 0;
    
    /* Skip whitespace and opening bracket */
    while (*ptr && (isspace(*ptr) || *ptr == '[')) ptr++;
    
    /* Parse up to 3 numbers */
    while (*ptr && value_count < 3) {
        char* end_ptr;
        values[value_count] = strtod(ptr, &end_ptr);
        
        if (end_ptr == ptr) break;
        
        value_count++;
        ptr = end_ptr;
        
        /* Skip comma and whitespace */
        while (*ptr && (isspace(*ptr) || *ptr == ',')) ptr++;
    }
    
    if (value_count == 3) {
        result->pitch = values[0];
        result->yaw = values[1];
        result->roll = values[2];
        return 1;
    }
    
    return 0;
}

static inline double kh_parse_scalar(const char* input) {
    if (!input) return 0.0;
    char* end_ptr;
    return strtod(input, &end_ptr);
}

/* Output formatting functions */
static inline void kh_format_vector3(vector3_t v, char* output, int output_size) {
    if (!output || output_size <= 0) return;
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "[%.6g,%.6g,%.6g]", v.x, v.y, v.z);
}

static inline void kh_format_quaternion(quaternion_t q, char* output, int output_size) {
    if (!output || output_size <= 0) return;
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "[%.6g,%.6g,%.6g,%.6g]", q.w, q.x, q.y, q.z);
}

static inline void kh_format_euler(euler_t e, char* output, int output_size) {
    if (!output || output_size <= 0) return;
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "[%.6g,%.6g,%.6g]", e.pitch, e.yaw, e.roll);
}

static inline void kh_format_scalar(double s, char* output, int output_size) {
    if (!output || output_size <= 0) return;
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "%.6g", s);
}

/* Function implementations for dispatch table */
static inline double kh_vec_magnitude_impl(vector3_t v) { return kh_vec3_magnitude(v); }
static inline double kh_vec_dot_impl(vector3_t a, vector3_t b) { return kh_vec3_dot(a, b); }
static inline double kh_vec_distance_impl(vector3_t a, vector3_t b) { return kh_vec3_distance(a, b); }
static inline double kh_vec_angle_impl(vector3_t a, vector3_t b) { return kh_vec3_angle(a, b); }

static inline vector3_t kh_vec_normalize_impl(vector3_t v) { return kh_vec3_normalize(v); }
static inline vector3_t kh_vec_add_impl(vector3_t a, vector3_t b) { return kh_vec3_add(a, b); }
static inline vector3_t kh_vec_subtract_impl(vector3_t a, vector3_t b) { return kh_vec3_subtract(a, b); }
static inline vector3_t kh_vec_cross_impl(vector3_t a, vector3_t b) { return kh_vec3_cross(a, b); }
static inline vector3_t kh_vec_project_impl(vector3_t a, vector3_t b) { return kh_vec3_project(a, b); }
static inline vector3_t kh_vec_reject_impl(vector3_t a, vector3_t b) { return kh_vec3_reject(a, b); }
static inline vector3_t kh_vec_reflect_impl(vector3_t incident, vector3_t normal) { return kh_vec3_reflect(incident, normal); }

static inline vector3_t kh_vec_multiply_impl(vector3_t v, double s) { return kh_vec3_multiply(v, s); }
static inline vector3_t kh_vec_divide_impl(vector3_t v, double s) { return kh_vec3_divide(v, s); }

static inline vector3_t kh_vec_lerp_impl(vector3_t a, vector3_t b, double t) { return kh_vec3_lerp(a, b, t); }

static inline quaternion_t kh_euler_to_quat_impl(euler_t e) { return kh_euler_to_quaternion(e); }
static inline euler_t kh_quat_to_euler_impl(quaternion_t q) { return kh_quaternion_to_euler(q); }

static inline quaternion_t kh_quat_normalize_impl(quaternion_t q) { return kh_quat_normalize(q); }
static inline quaternion_t kh_quat_conjugate_impl(quaternion_t q) { return kh_quat_conjugate(q); }
static inline quaternion_t kh_quat_inverse_impl(quaternion_t q) { return kh_quat_inverse(q); }
static inline quaternion_t kh_quat_multiply_impl(quaternion_t a, quaternion_t b) { return kh_quat_multiply(a, b); }
static inline quaternion_t kh_quat_slerp_impl(quaternion_t a, quaternion_t b, double t) { return kh_quat_slerp(a, b, t); }

static inline vector3_t kh_quat_rotate_impl(vector3_t v, quaternion_t q) { return kh_quat_rotate_vector(v, q); }

/* Whitelist of allowed vector functions - SECURITY CRITICAL */
static const vector_function_t KH_ALLOWED_VECTOR_FUNCTIONS[] = {
    /* Vector3 to Scalar operations */
    {"vmag", VEC_FUNC_V3_TO_SCALAR, 1, (void*)kh_vec_magnitude_impl, "Vector magnitude"},
    {"vlength", VEC_FUNC_V3_TO_SCALAR, 1, (void*)kh_vec_magnitude_impl, "Vector length (alias)"},
    
    /* Vector3, Vector3 to Scalar operations */
    {"vdot", VEC_FUNC_V3V3_TO_SCALAR, 2, (void*)kh_vec_dot_impl, "Vector dot product"},
    {"vdist", VEC_FUNC_V3V3_TO_SCALAR, 2, (void*)kh_vec_distance_impl, "Distance between vectors"},
    {"vangle", VEC_FUNC_V3V3_TO_SCALAR, 2, (void*)kh_vec_angle_impl, "Angle between vectors (degrees)"},
    
    /* Vector3 to Vector3 operations */
    {"vnorm", VEC_FUNC_V3_TO_V3, 1, (void*)kh_vec_normalize_impl, "Normalize vector"},
    {"vnormalize", VEC_FUNC_V3_TO_V3, 1, (void*)kh_vec_normalize_impl, "Normalize vector (alias)"},
    
    /* Vector3, Vector3 to Vector3 operations */
    {"vadd", VEC_FUNC_V3V3_TO_V3, 2, (void*)kh_vec_add_impl, "Add vectors"},
    {"vsub", VEC_FUNC_V3V3_TO_V3, 2, (void*)kh_vec_subtract_impl, "Subtract vectors"},
    {"vcross", VEC_FUNC_V3V3_TO_V3, 2, (void*)kh_vec_cross_impl, "Vector cross product"},
    {"vproject", VEC_FUNC_V3V3_TO_V3, 2, (void*)kh_vec_project_impl, "Project vector A onto vector B"},
    {"vreject", VEC_FUNC_V3V3_TO_V3, 2, (void*)kh_vec_reject_impl, "Reject vector A from vector B"},
    {"vreflect", VEC_FUNC_V3V3_TO_V3, 2, (void*)kh_vec_reflect_impl, "Reflect vector off normal"},
    
    /* Vector3, Scalar to Vector3 operations */
    {"vmul", VEC_FUNC_V3S_TO_V3, 2, (void*)kh_vec_multiply_impl, "Multiply vector by scalar"},
    {"vdiv", VEC_FUNC_V3S_TO_V3, 2, (void*)kh_vec_divide_impl, "Divide vector by scalar"},
    {"vscale", VEC_FUNC_V3S_TO_V3, 2, (void*)kh_vec_multiply_impl, "Scale vector (alias)"},
    
    /* Vector3, Vector3, Scalar to Vector3 operations */
    {"vlerp", VEC_FUNC_V3V3S_TO_V3, 3, (void*)kh_vec_lerp_impl, "Linear interpolation between vectors"},
    
    /* Euler to Quaternion conversion */
    {"etoquat", VEC_FUNC_EULER_TO_QUAT, 1, (void*)kh_euler_to_quat_impl, "Convert Euler angles to quaternion"},
    {"euler2quat", VEC_FUNC_EULER_TO_QUAT, 1, (void*)kh_euler_to_quat_impl, "Convert Euler angles to quaternion (alias)"},
    
    /* Quaternion to Euler conversion */
    {"quattoeuler", VEC_FUNC_QUAT_TO_EULER, 1, (void*)kh_quat_to_euler_impl, "Convert quaternion to Euler angles"},
    {"quat2euler", VEC_FUNC_QUAT_TO_EULER, 1, (void*)kh_quat_to_euler_impl, "Convert quaternion to Euler angles (alias)"},
    
    /* Quaternion operations */
    {"qnorm", VEC_FUNC_QUAT_TO_QUAT, 1, (void*)kh_quat_normalize_impl, "Normalize quaternion"},
    {"qnormalize", VEC_FUNC_QUAT_TO_QUAT, 1, (void*)kh_quat_normalize_impl, "Normalize quaternion (alias)"},
    {"qconj", VEC_FUNC_QUAT_TO_QUAT, 1, (void*)kh_quat_conjugate_impl, "Quaternion conjugate"},
    {"qconjugate", VEC_FUNC_QUAT_TO_QUAT, 1, (void*)kh_quat_conjugate_impl, "Quaternion conjugate (alias)"},
    {"qinv", VEC_FUNC_QUAT_TO_QUAT, 1, (void*)kh_quat_inverse_impl, "Quaternion inverse"},
    {"qinverse", VEC_FUNC_QUAT_TO_QUAT, 1, (void*)kh_quat_inverse_impl, "Quaternion inverse (alias)"},
    
    /* Quaternion, Quaternion operations */
    {"qmul", VEC_FUNC_QUATQUAT_TO_QUAT, 2, (void*)kh_quat_multiply_impl, "Multiply quaternions"},
    {"qmultiply", VEC_FUNC_QUATQUAT_TO_QUAT, 2, (void*)kh_quat_multiply_impl, "Multiply quaternions (alias)"},
    {"qslerp", VEC_FUNC_SPECIAL, 3, (void*)kh_quat_slerp_impl, "Spherical linear interpolation"},
    
    /* Vector3, Quaternion operations */
    {"vrotate", VEC_FUNC_V3QUAT_TO_V3, 2, (void*)kh_quat_rotate_impl, "Rotate vector by quaternion"}
};

static const int KH_VECTOR_FUNCTION_COUNT = sizeof(KH_ALLOWED_VECTOR_FUNCTIONS) / sizeof(vector_function_t);

/* Find vector function in whitelist - SECURITY CRITICAL */
static inline const vector_function_t* kh_find_vector_function(const char* name) {
    if (!name) return NULL;
    
    int i;
    for (i = 0; i < KH_VECTOR_FUNCTION_COUNT; i++) {
        if (strcmp(KH_ALLOWED_VECTOR_FUNCTIONS[i].name, name) == 0) {
            return &KH_ALLOWED_VECTOR_FUNCTIONS[i];
        }
    }
    return NULL;
}

/* Main vector operation processing function - enhanced with better memory management */
static int kh_process_vector_operation(char* output, int output_size, const char** argv, int argc) {
    if (!output || output_size <= 0 || !argv || argc < 1) {
        if (output && output_size > 0) {
            kh_set_error(output, output_size, "INVALID PARAMETERS");
        }
        return 1;
    }
    
    char* clean_func = NULL;
    char** clean_args = NULL;
    const vector_function_t* func;
    int i;
    int result = 1;
    
    /* Allocate memory for cleaned function name and arguments */
    clean_func = (char*)malloc(strlen(argv[0]) + 1);
    if (!clean_func) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(argv[0], clean_func, (int)strlen(argv[0]) + 1);
    
    if (strlen(clean_func) == 0) {
        kh_set_error(output, output_size, "EMPTY FUNCTION NAME");
        goto cleanup;
    }
    
    /* Find function in whitelist */
    func = kh_find_vector_function(clean_func);
    if (!func) {
        _snprintf_s(output, (size_t)output_size, _TRUNCATE, KH_ERROR_PREFIX "UNKNOWN VECTOR FUNCTION '%s'", clean_func);
        goto cleanup;
    }
    
    /* Check argument count */
    if (argc - 1 != func->arg_count) {
        _snprintf_s(output, (size_t)output_size, _TRUNCATE, KH_ERROR_PREFIX "FUNCTION '%s' EXPECTS %d ARGUMENTS, GOT %d", 
                 clean_func, func->arg_count, argc - 1);
        goto cleanup;
    }
    
    /* Allocate and clean arguments */
    if (argc > 1) {
        clean_args = (char**)malloc((size_t)(argc - 1) * sizeof(char*));
        if (!clean_args) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        
        for (i = 1; i < argc; i++) {
            clean_args[i - 1] = (char*)malloc(strlen(argv[i]) + 1);
            if (!clean_args[i - 1]) {
                kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
                goto cleanup;
            }
            kh_clean_string(argv[i], clean_args[i - 1], (int)strlen(argv[i]) + 1);
        }
    }
    
    /* Execute function based on type */
    switch (func->type) {
        case VEC_FUNC_V3_TO_SCALAR: {
            vector3_t v;
            if (!kh_parse_vector3(clean_args[0], &v)) {
                kh_set_error(output, output_size, "INVALID VECTOR3 FORMAT");
                goto cleanup;
            }
            double (*f)(vector3_t) = (double (*)(vector3_t))func->func_ptr;
            double res = f(v);
            kh_format_scalar(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_V3V3_TO_SCALAR: {
            vector3_t a, b;
            if (!kh_parse_vector3(clean_args[0], &a) || !kh_parse_vector3(clean_args[1], &b)) {
                kh_set_error(output, output_size, "INVALID VECTOR3 FORMAT");
                goto cleanup;
            }
            double (*f)(vector3_t, vector3_t) = (double (*)(vector3_t, vector3_t))func->func_ptr;
            double res = f(a, b);
            kh_format_scalar(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_V3_TO_V3: {
            vector3_t v;
            if (!kh_parse_vector3(clean_args[0], &v)) {
                kh_set_error(output, output_size, "INVALID VECTOR3 FORMAT");
                goto cleanup;
            }
            vector3_t (*f)(vector3_t) = (vector3_t (*)(vector3_t))func->func_ptr;
            vector3_t res = f(v);
            kh_format_vector3(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_V3V3_TO_V3: {
            vector3_t a, b;
            if (!kh_parse_vector3(clean_args[0], &a) || !kh_parse_vector3(clean_args[1], &b)) {
                kh_set_error(output, output_size, "INVALID VECTOR3 FORMAT");
                goto cleanup;
            }
            vector3_t (*f)(vector3_t, vector3_t) = (vector3_t (*)(vector3_t, vector3_t))func->func_ptr;
            vector3_t res = f(a, b);
            kh_format_vector3(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_V3S_TO_V3: {
            vector3_t v;
            double s;
            if (!kh_parse_vector3(clean_args[0], &v)) {
                kh_set_error(output, output_size, "INVALID VECTOR3 FORMAT");
                goto cleanup;
            }
            s = kh_parse_scalar(clean_args[1]);
            vector3_t (*f)(vector3_t, double) = (vector3_t (*)(vector3_t, double))func->func_ptr;
            vector3_t res = f(v, s);
            kh_format_vector3(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_V3V3S_TO_V3: {
            vector3_t a, b;
            double s;
            if (!kh_parse_vector3(clean_args[0], &a) || !kh_parse_vector3(clean_args[1], &b)) {
                kh_set_error(output, output_size, "INVALID VECTOR3 FORMAT");
                goto cleanup;
            }
            s = kh_parse_scalar(clean_args[2]);
            vector3_t (*f)(vector3_t, vector3_t, double) = (vector3_t (*)(vector3_t, vector3_t, double))func->func_ptr;
            vector3_t res = f(a, b, s);
            kh_format_vector3(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_EULER_TO_QUAT: {
            euler_t e;
            if (!kh_parse_euler(clean_args[0], &e)) {
                kh_set_error(output, output_size, "INVALID EULER FORMAT");
                goto cleanup;
            }
            quaternion_t (*f)(euler_t) = (quaternion_t (*)(euler_t))func->func_ptr;
            quaternion_t res = f(e);
            kh_format_quaternion(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_QUAT_TO_EULER: {
            quaternion_t q;
            if (!kh_parse_quaternion(clean_args[0], &q)) {
                kh_set_error(output, output_size, "INVALID QUATERNION FORMAT");
                goto cleanup;
            }
            euler_t (*f)(quaternion_t) = (euler_t (*)(quaternion_t))func->func_ptr;
            euler_t res = f(q);
            kh_format_euler(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_QUAT_TO_QUAT: {
            quaternion_t q;
            if (!kh_parse_quaternion(clean_args[0], &q)) {
                kh_set_error(output, output_size, "INVALID QUATERNION FORMAT");
                goto cleanup;
            }
            quaternion_t (*f)(quaternion_t) = (quaternion_t (*)(quaternion_t))func->func_ptr;
            quaternion_t res = f(q);
            kh_format_quaternion(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_QUATQUAT_TO_QUAT: {
            quaternion_t a, b;
            if (!kh_parse_quaternion(clean_args[0], &a) || !kh_parse_quaternion(clean_args[1], &b)) {
                kh_set_error(output, output_size, "INVALID QUATERNION FORMAT");
                goto cleanup;
            }
            quaternion_t (*f)(quaternion_t, quaternion_t) = (quaternion_t (*)(quaternion_t, quaternion_t))func->func_ptr;
            quaternion_t res = f(a, b);
            kh_format_quaternion(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_V3QUAT_TO_V3: {
            vector3_t v;
            quaternion_t q;
            if (!kh_parse_vector3(clean_args[0], &v) || !kh_parse_quaternion(clean_args[1], &q)) {
                kh_set_error(output, output_size, "INVALID VECTOR3 OR QUATERNION FORMAT");
                goto cleanup;
            }
            vector3_t (*f)(vector3_t, quaternion_t) = (vector3_t (*)(vector3_t, quaternion_t))func->func_ptr;
            vector3_t res = f(v, q);
            kh_format_vector3(res, output, output_size);
            result = 0;
            break;
        }
        
        case VEC_FUNC_SPECIAL: {
            /* Handle special cases like qslerp */
            if (strcmp(clean_func, "qslerp") == 0) {
                quaternion_t a, b;
                double t;
                if (!kh_parse_quaternion(clean_args[0], &a) || !kh_parse_quaternion(clean_args[1], &b)) {
                    kh_set_error(output, output_size, "INVALID QUATERNION FORMAT");
                    goto cleanup;
                }
                t = kh_parse_scalar(clean_args[2]);
                quaternion_t (*f)(quaternion_t, quaternion_t, double) = (quaternion_t (*)(quaternion_t, quaternion_t, double))func->func_ptr;
                quaternion_t res = f(a, b, t);
                kh_format_quaternion(res, output, output_size);
                result = 0;
            } else {
                kh_set_error(output, output_size, "SPECIAL FUNCTION NOT IMPLEMENTED");
            }
            break;
        }
        
        default:
            kh_set_error(output, output_size, "UNKNOWN FUNCTION TYPE");
            break;
    }

cleanup:
    free(clean_func);
    if (clean_args) {
        for (i = 1; i < argc; i++) {
            free(clean_args[i - 1]);
        }
        free(clean_args);
    }
    return result;
}

#endif /* VECTOR_OPERATIONS_HPP */