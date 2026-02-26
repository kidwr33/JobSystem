#include "ParticleUpdateNative.h"
#include <cmath>

// 纯C++实现的粒子更新逻辑
// ✅ 零跨界开销 - 可以从任意线程直接调用
extern "C" void UpdateParticlesNative(
    ParticleData* particles,
    uint32_t count,
    const PhysicsParams* params
) {
    // 计算指针偏移（用于随机数种子）
    uintptr_t ptrOffset = reinterpret_cast<uintptr_t>(particles) / sizeof(ParticleData);

    for (uint32_t i = 0; i < count; i++) {
        // 创建线程安全的随机数生成器
        SimpleRandom rng(params->baseSeed + ptrOffset + i);

        // 读取当前粒子
        ParticleData& particle = particles[i];

        // 1. 更新年龄
        particle.age += params->deltaTime;

        // 2. 如果粒子死亡，重置粒子
        if (particle.age >= particle.lifetime) {
            particle.age = 0.0f;
            particle.position = float3(
                rng.NextFloat(-10.0f, 10.0f),
                rng.NextFloat(5.0f, 10.0f),
                rng.NextFloat(-10.0f, 10.0f)
            );
            particle.velocity = float3(
                rng.NextFloat(-2.0f, 2.0f),
                rng.NextFloat(-2.0f, 2.0f),
                rng.NextFloat(-2.0f, 2.0f)
            );
        }

        // 3. 应用重力
        particle.velocity += params->gravity * params->deltaTime;

        // 4. 应用阻尼（空气阻力）
        particle.velocity *= (1.0f - params->damping * params->deltaTime);

        // 5. 更新位置
        particle.position += particle.velocity * params->deltaTime;

        // 6. 地面碰撞检测和反弹
        if (particle.position.y < params->groundLevel) {
            particle.position.y = params->groundLevel;
            particle.velocity.y = std::abs(particle.velocity.y) * params->bounceCoefficient;
        }
    }
}
