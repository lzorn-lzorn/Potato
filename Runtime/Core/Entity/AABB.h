#pragma once
#include <print>
#include <algorithm>
#include <type_traits>
#include <vector>
#include <random>
#include <chrono>
#include <numeric>

#include "Math/Vector.h"

namespace Core{
template <typename Ty, std::size_t Dimensions>
    requires std::is_arithmetic_v<Ty>
struct AABB {
    using value_type = Ty;
    using reference = Ty&;
    using pointer = Ty*;
    using const_pointer = const Ty*;
    using const_reference = const Ty&;

    static constexpr std::size_t dimensions = Dimensions;
    AABB()
        : min(Vector<Ty, Dimensions>{})
        , max(Vector<Ty, Dimensions>{}) {}
    explicit AABB(Vector<Ty, Dimensions> const &min, Vector<Ty, Dimensions> const &max)
        : min(min), max(max) {}

    Vector<Ty, Dimensions> min, max;
};

struct HitInfo {
    std::uint64_t idx1;
    std::uint64_t idx2;

    // 用于 std::vector<HitInfo> 的比较
    friend constexpr bool operator==(const HitInfo& lhs, const HitInfo& rhs) noexcept {
        return lhs.idx1 == rhs.idx1 && lhs.idx2 == rhs.idx2;
    }
};
template <typename Ty>
    requires std::is_arithmetic_v<Ty>
static constexpr bool IsInRange(Ty value, Ty left, Ty right) {
    return value >= left && value <= right;
}


template <typename Ty, std::size_t Dimensions>
    requires std::is_arithmetic_v<Ty>
inline bool IsOverlap(const AABB<Ty, Dimensions>& a,
                      const AABB<Ty, Dimensions>& b) noexcept
{
    for (std::size_t axis = 0; axis < Dimensions; ++axis) {
        if (a.max[axis] < b.min[axis] || b.max[axis] < a.min[axis]) {
            return false;
        }
    }
    return true;
}
/**
 * @brief Sweep and Prune 算法实现
 * @param aabbs: 场景中所有 AABB 构成的列表
 * @param check_order: 测量轴的顺序, 通常来说根据物品的分布从稀疏到密集选择轴会有更好效果, 默认就是 { 0, 1, 2 } 依次检查 x, y, z 轴
 * @return
 */
template <typename Ty, std::size_t Dimensions>
    requires std::is_arithmetic_v<Ty>
static std::vector<HitInfo> SweepAndPrune(const std::vector<AABB<Ty, Dimensions>>& aabbs, std::array<uint8_t, Dimensions> check_order)
{
    // 记录当前轴上 "端点" 信息
    struct EndPoint {
        Ty       min;
        Ty       max;
        uint64_t idx; // 该端点是 aabbs 中第几个
    };

    using IndexType      = std::uint64_t;
    using CollisionGroup = std::vector<IndexType>;   // 一组可能发生碰撞的 AABB 索引列表
    using CollisionList  = std::vector<CollisionGroup>; // 可疑碰撞列表

    const std::size_t size = aabbs.size();

    std::vector<HitInfo> hits;
    hits.reserve(size);

    if (size <= 1) {
        return hits;
    }

    // 初始化的时候整个场景的所有 aabb 就是一个可疑碰撞组
    CollisionList suspicious_collision_list;
    CollisionGroup suspicious_collision_group;
    suspicious_collision_group.resize(size); // 必须 resize 而不是 reserve
    std::iota(suspicious_collision_group.begin(),
              suspicious_collision_group.end(),
              IndexType{0});
    suspicious_collision_list.push_back(std::move(suspicious_collision_group));

    // 针对每一个测量轴依次进行 Sweep and Prune
    for (const auto& axis_raw : check_order) {
        const auto axis = static_cast<std::size_t>(axis_raw);
        if (axis >= Dimensions) {
            // 非法轴，直接跳过
            continue;
        }

        CollisionList next_collision_list;
        next_collision_list.reserve(suspicious_collision_list.size());

        // 对于每一个 group, 按照组进行逐个细分
        for (const auto& group : suspicious_collision_list) {
            if (group.size() <= 1) {
                // 组内不足两个元素，不可能产生碰撞
                continue;
            }

            std::vector<EndPoint> endpoints;
            endpoints.reserve(group.size());

            // 注册每一个 AABB 在当前轴上的最小值和最大值, 与其对应的索引
            for (const IndexType idx : group) {
                const auto& box = aabbs[static_cast<std::size_t>(idx)];
                endpoints.push_back(EndPoint{
                    box.min[axis],
                    box.max[axis],
                    idx
                });
            }

            // 针对当前轴进行排序
            // todo: 排序可以进一步优化, 因为每一帧的物体位置变化不大, 他们的相对轴的排序可能差别并不大
            std::sort(endpoints.begin(), endpoints.end(),
                      [](const EndPoint& a, const EndPoint& b) {
                          return a.min < b.min;
                      });

            if (endpoints.empty()) {
                continue; // 当前 group 在该轴上没有任何 endpoint，跳过
            }

            // 扫描当前轴, 按「轴向连通」切分出更小的 group
            // - 从左到右扫描, 维护一个当前"连通段"
            // - 如果下一个 AABB 的 min > 当前段中所有 AABB 的 max，说明当前段在该轴上已经不再与后面的有重叠，
            //   可以把当前段作为一个新的子 group 推入 next_collision_list，并重新开始新的段
            CollisionGroup current_sub_group;
            current_sub_group.reserve(endpoints.size());

            Ty current_segment_max = endpoints.front().max;
            current_sub_group.push_back(endpoints.front().idx);

            for (std::size_t i = 1; i < endpoints.size(); ++i) {
                // 当前轴上的这个 AABB 与之前段中所有 AABB 都不再重叠, 当前段形成一个 group
                // 换句话说, 一个 group 就是一个基于相交的连通分量
                // 即 在第一个 AABB 当前轴的 [min, max] 中, 整个组的 min 坐标都在该组中
                // |---<--------o1-------o1-----o2--------o3--------o2------->--------o3-------o4--------o4---|
                //     ^        ^        ^      ^         ^         ^        ^        ^        ^         ^
                //  cur_min   o1_min  o1_max  o2_min   o3_min   o2_max    cur_max  o3_max    o4_min    o4_max
                // 该组的中 cur, o1, o2, o3 在该轴上都有一定程度的重叠, 但是 o4 在该轴上与他们完全不重叠, 因此 o4 不在该组中
                if (const auto& ep = endpoints[i]; ep.min > current_segment_max) {
                    if (current_sub_group.size() > 1) {
                        next_collision_list.push_back(current_sub_group);
                    }

                    // 开启新的段
                    current_sub_group.clear();
                    current_sub_group.push_back(ep.idx);
                    current_segment_max = ep.max;
                } else {
                    // 当前轴上的这个 AABB 与当前段中至少一个 AABB 仍然重叠, 加入当前段
                    current_sub_group.push_back(ep.idx);
                    if (ep.max > current_segment_max) {
                        current_segment_max = ep.max;
                    }
                }
            }

            if (current_sub_group.size() > 1) {
                next_collision_list.push_back(std::move(current_sub_group));
            }
        }

        // 用新的一轮结果替代旧的可疑碰撞列表
        suspicious_collision_list = std::move(next_collision_list);

        // 如果经过这一轴筛选后已经没有可疑组了，可以提前结束
        if (suspicious_collision_list.empty()) break;
    }

    // 最终从 group 中生成具体的碰撞对
    // 此时 suspicious_collision_list 中的每个 group 表示: 在所有经过的轴上都存在一定程度连通性的 AABB 集合
    // 我们再在每个 group 内做一次多轴 AABB 重叠检查(IsOverlap), 把真正宽相重叠的 AABB 对输出为 HitInfo
    for (const auto& group : suspicious_collision_list) {
        const std::size_t group_size = group.size();
        for (std::size_t i = 0; i < group_size; ++i) {
            for (std::size_t j = i + 1; j < group_size; ++j) {
                const auto idx1 = group[i];
                const auto idx2 = group[j];
                if (IsOverlap(aabbs[idx1], aabbs[idx2])) {
                    hits.push_back(HitInfo{ idx1, idx2 });
                }
            }
        }
    }

    return hits;
}

/**
 * @brief 1D Sweep and Prune：只在单个轴上生成候选对（不做全维判断）
 * @param aabbs
 * @param axis 具体的某个轴
 * @return
 */
template <typename Ty, std::size_t Dimensions>
    requires std::is_arithmetic_v<Ty>
static std::vector<HitInfo> SweepAndPrune1D(const std::vector<AABB<Ty, Dimensions>>& aabbs, const std::size_t axis)
{
    using IndexType = std::size_t;

    struct Endpoint {
        Ty        min;
        Ty        max;
        IndexType idx;
    };

    const std::size_t n = aabbs.size();
    std::vector<HitInfo> candidates;
    if (n <= 1) return candidates;

    std::vector<Endpoint> endpoints;
    endpoints.reserve(n);
    for (IndexType i = 0; i < n; ++i) {
        const auto& box = aabbs[i];
        endpoints.push_back(Endpoint{ box.min[axis], box.max[axis], i });
    }

    std::sort(endpoints.begin(), endpoints.end(),
              [](const Endpoint& a, const Endpoint& b) {
                  return a.min < b.min;
              });

    std::vector<IndexType> active;
    active.reserve(n);

    for (const auto& ep : endpoints) {
        const Ty current_min = ep.min;
        const IndexType i = ep.idx;

        // 移除在该轴上已经不可能与后面重叠的盒子
        active.erase(
            std::remove_if(active.begin(), active.end(),
                           [&](IndexType j) {
                               return aabbs[j].max[axis] < current_min;
                           }),
            active.end());

        // 剩下的 active 中每个 j 在该轴上都与 i 有区间重叠 => 候选对
        for (IndexType j : active) {
            candidates.push_back(HitInfo{
                static_cast<std::uint64_t>(j),
                static_cast<std::uint64_t>(i)
            });
        }

        active.push_back(i);
    }

    return candidates;
}

/**
 * @brief 完整的 AABB 流程：1D SAP 生成候选对 + 全维 AABB IsOverlap 窄相
 * @param aabbs 是完整 AABB 集合
 * @param axis
 * @return
 */
template <typename Ty, std::size_t Dimensions>
    requires std::is_arithmetic_v<Ty>
static std::vector<HitInfo> FullAABBCollisionFrom1D(const std::vector<AABB<Ty, Dimensions>>& aabbs, std::size_t axis)
{
    auto candidates = SweepAndPrune1D(aabbs, axis);

    std::vector<HitInfo> hits;
    hits.reserve(candidates.size());

    for (const auto& c : candidates) {
        auto i = static_cast<std::size_t>(c.idx1);
        auto j = static_cast<std::size_t>(c.idx2);
        if (i >= aabbs.size() || j >= aabbs.size()) continue;

        if (IsOverlap(aabbs[i], aabbs[j])) {
            hits.push_back(c);
        }
    }

    return hits;
}


template <typename Ty, std::size_t Dimensions>
    requires std::is_arithmetic_v<Ty>
static bool IsOverlap(const std::vector<AABB<Ty, Dimensions>> aabbs) {
    const std::size_t size = aabbs.size();
    if (size <= 1) [[unlikely]] { return false; }

    constexpr std::size_t axis = 0;

    struct Endpoint {
        Ty min;
        Ty max;
        std::size_t idx;
    };

    std::vector<Endpoint> endpoints;
    endpoints.reserve(size);

    for (std::size_t i = 0; i < size; ++i) {
        const auto& box = aabbs[i];
        endpoints.push_back(Endpoint{ box.min[axis], box.max[axis], i });
    }

    std::sort(endpoints.begin(), endpoints.end(),
              [](const Endpoint& a, const Endpoint& b) {
                  return a.min < b.min;
              });

    std::vector<std::size_t> active;
    active.reserve(size);

    for (const auto& e : endpoints) {
        const Ty current_min = e.min;
        const std::size_t current_idx = e.idx;

        active.erase(
            std::remove_if(active.begin(), active.end(),
                           [&](std::size_t idx) {
                               return aabbs[idx].max[axis] < current_min;
                           }),
            active.end());

        for (std::size_t idx : active) {
            if (IsOverlap(aabbs[idx], aabbs[current_idx])) {
                return true;  // 一旦发现一对重叠就返回
            }
        }

        active.push_back(current_idx);
    }

    return false;
}


// 朴素 O(N^2) 检测，用于和 SAP 结果对比验证正确性
template <typename Ty, std::size_t Dimensions>
    requires std::is_arithmetic_v<Ty>
static std::vector<HitInfo> BruteForcePairs(const std::vector<AABB<Ty, Dimensions>>& aabbs) {
    std::vector<HitInfo> hits;
    const std::size_t n = aabbs.size();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            if (IsOverlap(aabbs[i], aabbs[j])) {
                hits.push_back(HitInfo{ i, j });
            }
        }
    }
    return hits;
}

int test() {
    using Ty = float;
    using Clock = std::chrono::steady_clock;

    std::mt19937 rng{std::random_device{}()};

    auto randRange = [&](Ty minV, Ty maxV) {
        std::uniform_real_distribution<Ty> dist(minV, maxV);
        return dist(rng);
    };

    auto genRandomAABBs2D = [&](std::size_t N,
                                Ty worldMin = -1000.0f,
                                Ty worldMax =  1000.0f,
                                Ty minHalf = 0.1f,
                                Ty maxHalf = 50.0f) {
        std::vector<AABB<Ty, 2>> aabbs;
        aabbs.reserve(N);
        std::uniform_real_distribution<Ty> centerDist(worldMin, worldMax);
        std::uniform_real_distribution<Ty> halfDist(minHalf, maxHalf);

        for (std::size_t i = 0; i < N; ++i) {
            Ty cx = centerDist(rng);
            Ty cy = centerDist(rng);
            Ty hx = halfDist(rng);
            Ty hy = halfDist(rng);

            Vector<Ty, 2> mn{cx - hx, cy - hy};
            Vector<Ty, 2> mx{cx + hx, cy + hy};
            aabbs.emplace_back(mn, mx);
        }
        return aabbs;
    };

    auto genRandomAABBs3D = [&](std::size_t N,
                                Ty worldMin = -1000.0f,
                                Ty worldMax =  1000.0f,
                                Ty minHalf = 0.1f,
                                Ty maxHalf = 50.0f) {
        std::vector<AABB<Ty, 3>> aabbs;
        aabbs.reserve(N);
        std::uniform_real_distribution<Ty> centerDist(worldMin, worldMax);
        std::uniform_real_distribution<Ty> halfDist(minHalf, maxHalf);

        for (std::size_t i = 0; i < N; ++i) {
            Ty cx = centerDist(rng);
            Ty cy = centerDist(rng);
            Ty cz = centerDist(rng);
            Ty hx = halfDist(rng);
            Ty hy = halfDist(rng);
            Ty hz = halfDist(rng);

            Vector<Ty, 3> mn{cx - hx, cy - hy, cz - hz};
            Vector<Ty, 3> mx{cx + hx, cy + hy, cz + hz};
            aabbs.emplace_back(mn, mx);
        }
        return aabbs;
    };

    // 去重并排序 HitInfo，方便比较集合是否一致
    auto normalizeHits = [](std::vector<HitInfo>& hits) {
        for (auto& h : hits) {
            if (h.idx1 > h.idx2) std::swap(h.idx1, h.idx2);
        }
        std::sort(hits.begin(), hits.end(),
                  [](const HitInfo& a, const HitInfo& b) {
                      if (a.idx1 != b.idx1) return a.idx1 < b.idx1;
                      return a.idx2 < b.idx2;
                  });
        hits.erase(std::unique(hits.begin(), hits.end(),
                               [](const HitInfo& a, const HitInfo& b) {
                                   return a.idx1 == b.idx1 && a.idx2 == b.idx2;
                               }),
                   hits.end());
    };

    auto runScenario2D = [&](std::string_view name,
                             std::size_t N,
                             Ty worldMin, Ty worldMax,
                             Ty minHalf, Ty maxHalf) {
        std::println("=== 2D 场景: {} ===", name);
        std::println("N = {}, world = [{}, {}], half = [{}, {}]",
                     N, worldMin, worldMax, minHalf, maxHalf);

        auto aabbs = genRandomAABBs2D(N, worldMin, worldMax, minHalf, maxHalf);

        // 1) 多轴 SAP
        auto t0 = Clock::now();
        std::array<uint8_t, 2> order{0, 1};
        auto sapHits = SweepAndPrune(aabbs, order);
        auto t1 = Clock::now();

        // 2) 1D SAP + 全维 AABB
        auto t1d0 = Clock::now();
        auto sap1dHits = FullAABBCollisionFrom1D(aabbs, 0); // 沿 X 轴 sweep
        auto t1d1 = Clock::now();

        // 3) Brute force
        auto t2 = Clock::now();
        auto bruteHits = BruteForcePairs(aabbs);
        auto t3 = Clock::now();

        normalizeHits(sapHits);
        normalizeHits(sap1dHits);
        normalizeHits(bruteHits);

        using std::chrono::microseconds;
        auto dtSap   = std::chrono::duration_cast<microseconds>(t1   - t0).count();
        auto dt1d    = std::chrono::duration_cast<microseconds>(t1d1 - t1d0).count();
        auto dtBrute = std::chrono::duration_cast<microseconds>(t3   - t2).count();

        std::println("Multi-axis SAP    命中数: {}, 时间: {} us", sapHits.size(),   dtSap);
        std::println("1D SAP + AABB     命中数: {}, 时间: {} us", sap1dHits.size(), dt1d);
        std::println("BruteForce        命中数: {}, 时间: {} us", bruteHits.size(), dtBrute);

        if (sapHits == bruteHits) {
            std::println("Multi-axis SAP 结果一致 ✓");
        } else {
            std::println("Multi-axis SAP 结果不一致 ✗");
        }

        if (sap1dHits == bruteHits) {
            std::println("1D SAP + AABB 结果一致 ✓");
        } else {
            std::println("1D SAP + AABB 结果不一致 ✗");
        }

        // 小规模时可打印差异
        if (N <= 200 && (sapHits != bruteHits || sap1dHits != bruteHits)) {
            std::println("Brute hits:");
            for (auto& h : bruteHits) {
                std::println("  ({}, {})", h.idx1, h.idx2);
            }
            std::println("Multi-axis SAP hits:");
            for (auto& h : sapHits) {
                std::println("  ({}, {})", h.idx1, h.idx2);
            }
            std::println("1D SAP + AABB hits:");
            for (auto& h : sap1dHits) {
                std::println("  ({}, {})", h.idx1, h.idx2);
            }
        }

        std::println("");
    };

    auto runScenario3D = [&](std::string_view name,
                             std::size_t N,
                             Ty worldMin, Ty worldMax,
                             Ty minHalf, Ty maxHalf) {
        std::println("=== 3D 场景: {} ===", name);
        std::println("N = {}, world = [{}, {}], half = [{}, {}]",
                     N, worldMin, worldMax, minHalf, maxHalf);

        auto aabbs = genRandomAABBs3D(N, worldMin, worldMax, minHalf, maxHalf);

        // 1) 多轴 SAP
        auto t0 = Clock::now();
        std::array<uint8_t, 3> order{0, 1, 2}; // x,y,z
        auto sapHits = SweepAndPrune(aabbs, order);
        auto t1 = Clock::now();

        // 2) 1D SAP + 全维 AABB（例如仍然选 X 轴）
        auto t1d0 = Clock::now();
        auto sap1dHits = FullAABBCollisionFrom1D(aabbs, 0);
        auto t1d1 = Clock::now();

        // 3) Brute force
        auto t2 = Clock::now();
        auto bruteHits = BruteForcePairs(aabbs);
        auto t3 = Clock::now();

        normalizeHits(sapHits);
        normalizeHits(sap1dHits);
        normalizeHits(bruteHits);

        using std::chrono::microseconds;
        auto dtSap   = std::chrono::duration_cast<microseconds>(t1   - t0).count();
        auto dt1d    = std::chrono::duration_cast<microseconds>(t1d1 - t1d0).count();
        auto dtBrute = std::chrono::duration_cast<microseconds>(t3   - t2).count();

        std::println("Multi-axis SAP    命中数: {}, 时间: {} us", sapHits.size(),   dtSap);
        std::println("1D SAP + AABB     命中数: {}, 时间: {} us", sap1dHits.size(), dt1d);
        std::println("BruteForce        命中数: {}, 时间: {} us", bruteHits.size(), dtBrute);

        if (sapHits == bruteHits) {
            std::println("Multi-axis SAP 结果一致 ✓");
        } else {
            std::println("Multi-axis SAP 结果不一致 ✗");
        }

        if (sap1dHits == bruteHits) {
            std::println("1D SAP + AABB 结果一致 ✓");
        } else {
            std::println("1D SAP + AABB 结果不一致 ✗");
        }

        if (N <= 200 && (sapHits != bruteHits || sap1dHits != bruteHits)) {
            std::println("Brute hits:");
            for (auto& h : bruteHits) {
                std::println("  ({}, {})", h.idx1, h.idx2);
            }
            std::println("Multi-axis SAP hits:");
            for (auto& h : sapHits) {
                std::println("  ({}, {})", h.idx1, h.idx2);
            }
            std::println("1D SAP + AABB hits:");
            for (auto& h : sap1dHits) {
                std::println("  ({}, {})", h.idx1, h.idx2);
            }
        }

        std::println("");
    };

    auto runScenario3D_Strong1D = [&](std::string_view name,
                                      std::size_t N) {
        std::println("=== 3D 场景: {} ===", name);
        std::println("N = {}", N);

        // 构造严格按 X 轴排开的盒子：每个盒子和相邻盒子刚好不重叠
        std::vector<AABB<Ty, 3>> aabbs;
        aabbs.reserve(N);

        const Ty halfX = 1.0f;
        const Ty halfY = 0.5f;
        const Ty halfZ = 0.5f;

        // 间隔设置得稍微大一点，确保不重叠： [i*stepX - halfX, i*stepX + halfX]
        const Ty stepX = 3.0f;

        for (std::size_t i = 0; i < N; ++i) {
            Ty cx = static_cast<Ty>(i) * stepX;
            Ty cy = 0.0f;
            Ty cz = 0.0f;

            Vector<Ty, 3> mn{cx - halfX, cy - halfY, cz - halfZ};
            Vector<Ty, 3> mx{cx + halfX, cy + halfY, cz + halfZ};
            aabbs.emplace_back(mn, mx);
        }

        // 1) 多轴 SAP
        auto t0 = Clock::now();
        std::array<uint8_t, 3> order{0, 1, 2};
        auto sapHits = SweepAndPrune(aabbs, order);
        auto t1 = Clock::now();

        // 2) 1D SAP + AABB（沿 X 轴）
        auto t1d0 = Clock::now();
        auto sap1dHits = FullAABBCollisionFrom1D(aabbs, 0);
        auto t1d1 = Clock::now();

        // 3) BruteForce
        auto t2 = Clock::now();
        auto bruteHits = BruteForcePairs(aabbs);
        auto t3 = Clock::now();

        normalizeHits(sapHits);
        normalizeHits(sap1dHits);
        normalizeHits(bruteHits);

        using std::chrono::microseconds;
        auto dtSap   = std::chrono::duration_cast<microseconds>(t1   - t0).count();
        auto dt1d    = std::chrono::duration_cast<microseconds>(t1d1 - t1d0).count();
        auto dtBrute = std::chrono::duration_cast<microseconds>(t3   - t2).count();

        std::println("Multi-axis SAP    命中数: {}, 时间: {} us", sapHits.size(),   dtSap);
        std::println("1D SAP + AABB     命中数: {}, 时间: {} us", sap1dHits.size(), dt1d);
        std::println("BruteForce        命中数: {}, 时间: {} us", bruteHits.size(), dtBrute);

        if (sapHits == bruteHits) {
            std::println("Multi-axis SAP 结果一致 ✓");
        } else {
            std::println("Multi-axis SAP 结果不一致 ✗");
        }

        if (sap1dHits == bruteHits) {
            std::println("1D SAP + AABB 结果一致 ✓");
        } else {
            std::println("1D SAP + AABB 结果不一致 ✗");
        }

        std::println("");
    };

    // -------- 具体测试场景配置 --------

    // 1) 2D 稀疏场景：大世界、较小半径，重叠概率低
    runScenario2D("2D Sparse", 2000,
                  -1000.0f, 1000.0f,
                  0.5f, 3.0f);

    // 2) 2D 局部簇集：小世界内大量盒子，重叠概率高
    runScenario2D("2D Clustered", 3000,
                  -50.0f, 50.0f,
                  1.0f, 10.0f);

    // 3) 2D 高重叠：世界极小，盒子半径接近世界大小
    runScenario2D("2D Highly Overlapping", 1500,
                  -20.0f, 20.0f,
                  10.0f, 30.0f);

    // 4) 3D 稀疏大场景
    runScenario3D("3D Sparse", 3000,
                  -2000.0f, 2000.0f,
                  0.5f, 5.0f);

    // 5) 3D 局部簇集
    runScenario3D("3D Clustered", 4000,
                  -100.0f, 100.0f,
                  2.0f, 15.0f);

    // 6) 3D 高重叠
    runScenario3D("3D Highly Overlapping", 20000,
                  -30.0f, 30.0f,
                  10.0f, 40.0f);

    // 7) 小规模 2D 可视化场景
    runScenario2D("2D Small Visual", 50,
                  -20.0f, 20.0f,
                  1.0f, 5.0f);

    // 8) 大规模 3D 稀疏场景，
    runScenario3D("3D Very Sparse Huge", 20000,
              -5000.0f, 5000.0f,
              0.5f, 2.0f);

    // 9) 专门偏向 1D SAP 的 3D 场景：所有盒子按 X 轴排开，完全不重叠
    runScenario3D_Strong1D("3D Strong 1D Non-Overlap", 20000);
    return 0;
}
}