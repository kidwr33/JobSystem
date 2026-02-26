#pragma once
#include <cstdint>

// C++版本的 Vector3 (与Unity Vector3兼容)
struct float3 {
    float x, y, z;

    float3() : x(0), y(0), z(0) {}
    float3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    float3 operator+(const float3& other) const {
        return float3(x + other.x, y + other.y, z + other.z);
    }

    float3 operator*(float scalar) const {
        return float3(x * scalar, y * scalar, z * scalar);
    }

    float3& operator+=(const float3& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }

    float3& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }
};

// C++版本的 Color (与Unity Color兼容)
struct Color {
    float r, g, b, a;
};

// 粒子数据结构 (与C#端完全一致)
struct ParticleData {
    float3 position;
    float3 velocity;
    Color color;
    float lifetime;
    float age;
    float _padding1;   // 对齐填充
    float _padding2;
    float _padding3;
    float _padding4;
};

// 物理参数结构
struct PhysicsParams {
    float deltaTime;
    float3 gravity;
    float damping;
    float groundLevel;
    float bounceCoefficient;
    uint32_t baseSeed;
};

// 简单的线性同余随机数生成器 (与Unity Mathematics.Random兼容)
class SimpleRandom {
private:
    uint32_t state;

public:
    SimpleRandom(uint32_t seed) : state(seed) {}

    uint32_t NextUInt() {
        state = 1664525u * state + 1013904223u;
        return state;
    }

    float NextFloat() {
        return (float)NextUInt() / (float)UINT32_MAX;
    }

    float NextFloat(float min, float max) {
        return min + NextFloat() * (max - min);
    }
};

// 纯C++粒子更新函数 (零C# delegate开销)
extern "C" void UpdateParticlesNative(
    ParticleData* particles,
    uint32_t count,
    const PhysicsParams* params
);
