
#include "../Optional.hpp"
#include <cassert>
#include <optional>
#include <vector>
#include <memory>
#include <string>
#include <initializer_list>
#include <iostream>

using namespace Core;

bool ConstructTest() {
    bool all_passed = true;
    std::cout << "Running Optional Tests...\n";
    // 1. 默认构造
    {   
        std::cout << "Running Optional Tests1\n";
        Optional<int> o;
        if (o.HasValue()) {
            std::cerr << "Default construct failed: should not have value.\n";
            all_passed = false;
        }
    }

    // 2. NulloptTp 构造
    {
        std::cout << "Running Optional Tests2\n";
        Optional<int> o(nullopt);
        if (o.HasValue()) {
            std::cerr << "NulloptTp construct failed: should not have value.\n";
            all_passed = false;
        }
    }

    // 3. std::nullopt_t 构造
    {
        std::cout << "Running Optional Tests3\n";
        Optional<int> o(std::nullopt);
        if (o.HasValue()) {
            std::cerr << "std::nullopt_t construct failed: should not have value.\n";
            all_passed = false;
        }
    }

    // 4. in_place 构造（单参数）
    {
        std::cout << "Running Optional Tests4\n";
        Optional<int> o(std::in_place, 42);
        if (!o.HasValue() || *o != 42) {
            std::cerr << "in_place single arg failed: value=" << (o.HasValue() ? std::to_string(*o) : "<none>") << "\n";
            all_passed = false;
        }
    }

    // 5. in_place 构造（多参数）
    {
        std::cout << "Running Optional Tests5\n";
        Optional<std::pair<int, std::string>> o(std::in_place, 7, "hello");
        if (!o.HasValue() || o->first != 7 || o->second != "hello") {
            std::cerr << "in_place multi args failed\n";
            all_passed = false;
        }
    }

    // 6. initializer_list 构造（如std::vector）
    {
        std::cout << "Running Optional Tests6\n";
        Optional<std::vector<int>> o(std::in_place, {1, 2, 3, 4});
        if (!o.HasValue() || o->size() != 4 || (*o)[2] != 3) {
            std::cerr << "initializer_list construct failed\n";
            all_passed = false;
        }
    }

    // 7. 通过直接可转换类型构造
    {
        std::cout << "Running Optional Tests7\n";
        int v = 99;
        Optional<int> o(v);
        if (!o.HasValue() || *o != 99) {
            std::cerr << "direct conversion from int failed\n";
            all_passed = false;
        }
    }

    // 8. 通过可移动类型构造
    {
        std::cout << "Running Optional Tests8\n";
        std::string s = "move me";
        Optional<std::string> o(std::move(s));
        if (!o.HasValue() || *o != "move me" || s != "") {
            std::cerr << "move construct failed\n";
            all_passed = false;
        }
    }

    // 9. 通过initializer_list+extra参数
    {
        std::cout << "Running Optional Tests9\n";
        Optional<std::vector<int>> o(std::in_place, {5, 6, 7, 8}, std::allocator<int>{});
        if (!o.HasValue() || o->size() != 4 || (*o)[0] != 5) {
            std::cerr << "initializer_list+extra args failed\n";
            all_passed = false;
        }
    }

    // 10. 从 Optional<U> 构造 Optional<T>（拆包构造）
    {
        std::cout << "Running Optional Tests10\n";
        Optional<int> oi(123);
        Optional<double> od(oi); // int -> double
        if (!od.HasValue() || *od != 123.0) {
            std::cerr << "construct from Optional<U> (int->double) failed\n";
            all_passed = false;
        }
    }

    // 11. 构造 Optional<bool> 从 Optional<int>（特殊拆包）
    {
        std::cout << "Running Optional Tests11\n";
        Optional<int> oi(0);
        Optional<bool> ob(oi);
        if (!ob.HasValue() || *ob != false) {
            std::cerr << "construct Optional<bool> from Optional<int> failed\n";
            all_passed = false;
        }
    }

    // 12. 构造 Optional<std::unique_ptr<int>>
    {
        std::cout << "Running Optional Tests12\n";
        // Optional<std::unique_ptr<int>> ou(std::in_place, new int(77));
        // if (!ou.HasValue() || !*ou || **ou != 77) {
        //     std::cerr << "construct Optional<std::unique_ptr<int>> failed\n";
        //     all_passed = false;
        // }
    }

    // 13. 通过函数返回值 tag 构造（ConstructFromInvokeResultTag）
    {
        std::cout << "Running Optional Tests13\n";
        auto fn = [](int x) { return x * x; };
        Optional<int> o(ConstructFromInvokeResultTag{}, fn, 6);
        if (!o.HasValue() || *o != 36) {
            std::cerr << "ConstructFromInvokeResultTag failed\n";
            all_passed = false;
        }
    }

    // 14. 复制构造
    {
        std::cout << "Running Optional Tests14\n";
        std::optional<std::string> src1("copyme");
        std::optional<std::string> dst1(src1);
        Optional<std::string> src("copyme");
        Optional<std::string> dst(src);
        if (!dst.HasValue() || *dst != "copyme") {
            std::cerr << "copy construct failed\n";
            all_passed = false;
        }
    }

    // 15. 移动构造
    {
        std::cout << "Running Optional Tests15\n";
        Optional<std::string> src("moveme");
        Optional<std::string> dst(std::move(src));
        if (!dst.HasValue() || *dst != "moveme") {
            std::cerr << "move construct failed\n";
            all_passed = false;
        }
    }

    // 16. 用 optional 构造 optional（禁止直接同类型转化）
    {
        std::cout << "Running Optional Tests16\n";
        Optional<int> oi(5);
        // Should not compile: Optional<int> oj(oi);
        // But for test, let's check it's not allowed (if SFINAE works)
        // If allowed, fail the test.
        // Not testable in runtime, only at compile time.
    }

    // 17. 构造 Optional<std::shared_ptr<int>>
    {
        std::cout << "Running Optional Tests17\n";
        // auto sp = std::make_shared<int>(314);
        // Optional<std::shared_ptr<int>> o(sp);
        // if (!o.HasValue() || !*o || **o != 314) {
        //     std::cerr << "shared_ptr construct failed\n";
        //     all_passed = false;
        // }
    }

    // 18. 构造 Optional 指针类型
    // {
    std::cout << "Running Optional Tests18\n";
    //     int arr[3] = {1,2,3};
    //     Optional<int*> op(arr);
    //     if (!op.HasValue() || *op != arr) {
    //         std::cerr << "pointer construct failed\n";
    //         all_passed = false;
    //     }
    // }

    // 19. 构造 Optional<const char*>
    // {
    std::cout << "Running Optional Tests19\n";
    //     const char* msg = "hello world";
    //     Optional<const char*> oc(msg);
    //     if (!oc.HasValue() || *oc != msg) {
    //         std::cerr << "const char* construct failed\n";
    //         all_passed = false;
    //     }
    // }

    // 20. 构造 Optional 对象类型（自定义类）
    {
        std::cout << "Running Optional Tests20\n";
        struct MyStruct {
            int a; double b;
            MyStruct(int x, double y) : a(x), b(y) {}
        };
        Optional<MyStruct> o(std::in_place, 3, 4.5);
        if (!o.HasValue() || o->a != 3 || o->b != 4.5) {
            std::cerr << "custom struct in_place construct failed\n";
            all_passed = false;
        }
    }

    // 21. 构造 Optional<std::vector<std::string>> (initializer_list) 标准也过不了
    {
        std::cout << "Running Optional Tests21\n";
        // std::optional<std::vector<std::string>> o1{std::in_place, {"a", "b", "c"}};
        // Optional<std::vector<std::string>> o(std::in_place, {"a", "b", "c"});
        // if (!o.HasValue() || o->size() != 3 || (*o)[1] != "b") {
        //     std::cerr << "vector<string> initializer_list construct failed\n";
        //     all_passed = false;
        // }
    }

    // 22. 构造 Optional 通过 MakeOptional
    {
        std::cout << "Running Optional Tests22\n";
        auto o = MakeOptional<std::vector<int>>({10, 20, 30});
        if (!o.HasValue() || o->size() != 3 || (*o)[2] != 30) {
            std::cerr << "MakeOptional failed\n";
            all_passed = false;
        }
    }

    // 23. 构造 Optional<bool> from Optional<bool>
    {
        std::cout << "Running Optional Tests23\n";
        Optional<bool> ob(true);
        Optional<bool> ob2(ob);
        if (!ob2.HasValue() || *ob2 != true) {
            std::cerr << "Optional<bool> copy construct failed\n";
            all_passed = false;
        }
    }

    struct MoveOnly {
        MoveOnly() = default;
        MoveOnly(MoveOnly&&) = default;
        MoveOnly& operator=(MoveOnly&&) = default;
        // 禁用拷贝
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
        int value = 42;
    };
    {
        std::cout << "Running Optional MoveOnly Tests\n";

        std::optional<MoveOnly> o11(std::in_place);
        std::optional<MoveOnly> o12(std::move(o11));
        if constexpr (std::is_trivially_destructible_v<MoveOnly>) {
            std::cout << "MoveOnly is trivially destructible\n";
        }
        // Optional<MoveOnly> o3(o12);
        Optional<MoveOnly> o(std::in_place);
        if (!o.HasValue() || o->value != 42) {
            std::cerr << "MoveOnly construct failed\n";
        }
        Optional<MoveOnly> o2(std::move(o));
        if (!o2.HasValue() || o2->value != 42) {
            std::cerr << "MoveOnly move construct failed\n";
        }
        // Optional<MoveOnly> o3(o2); // 拷贝构造被禁用
    }

    // 输出测试结果
    if (all_passed) {
        std::cout << "All Optional construct tests passed!\n";
    } else {
        std::cout << "Some Optional construct tests FAILED!\n";
    }
    return all_passed;
}