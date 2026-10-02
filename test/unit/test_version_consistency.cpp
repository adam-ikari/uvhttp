/*
 * UVHTTP 版本号与 VERSION 文件一致性测试
 *
 * 为什么需要独立测试：库的版本号曾长期与 VERSION 文件脱节——
 * src/uvhttp_version.c 的 UVHTTP_VERSION_STRING / MAJOR / MINOR / PATCH
 * 都是 `#ifndef` 保护的硬编码默认值（2.8.1），而 CMake 没有注入编译期覆盖，
 * 于是 VERSION 文件写着 2.9.1、CMake 的 PROJECT_VERSION 也是 2.9.1，
 * 但 libuvhttp.a 里 `strings` 出来的版本是 **2.8.1**。
 *
 * 后果：uvhttp_get_version_string() 与 uvhttp_get_build_info().version_string
 * 对外报告一个过期版本，CMake 导出的 package 版本与库自报版本不一致。
 *
 * 现有 test_version_full_coverage.cpp 只验证「各 API 之间自洽」
 * （version_string == UVHTTP_VERSION_STRING 等），宏本身错时它们全绿——
 * 故该缺陷在其下完全不可见。
 *
 * 本测试把库自报版本钉死到 VERSION 文件，形成单一真相来源。
 * 修复方式是 CMakeLists.txt 从 PROJECT_VERSION 注入同名宏（该文件已在顶部
 * 从 VERSION 文件解析出 PROJECT_VERSION）。
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_version.h"
}

#include <stdio.h>
#include <string.h>
#include <string>

namespace {

/* __FILE__ 上溯两级到源码根（本项目 CMake 传绝对路径给编译器，
 * 故 __FILE__ 展开即绝对路径，不受 ctest 工作目录影响）。 */
std::string RepoRoot() {
    std::string f(__FILE__);
    size_t last_sep = f.find_last_of('/');
    if (last_sep == std::string::npos)
        return std::string();
    size_t parent_sep = f.find_last_of('/', last_sep - 1);
    if (parent_sep == std::string::npos)
        return std::string();
    size_t root_sep = f.find_last_of('/', parent_sep - 1);
    if (root_sep == std::string::npos)
        return std::string();
    return f.substr(0, root_sep);
}

/* 从 VERSION 文件读出 VERSION= 的值 */
bool ReadVersionFile(std::string* out) {
    std::string path = RepoRoot() + "/VERSION";
    FILE* fp = fopen(path.c_str(), "r");
    if (!fp)
        return false;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VERSION=", 8) == 0) {
            char* v = line + 8;
            char* nl = strpbrk(v, "\r\n");
            if (nl)
                *nl = '\0';
            *out = v;
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

}  // namespace

class VersionMatchesFile : public ::testing::Test {};

/* 库自报版本必须等于 VERSION 文件中的 VERSION —— 版本号的单一真相来源 */
TEST_F(VersionMatchesFile, LibraryReportsVersionFileVersion) {
    std::string expected;
    ASSERT_TRUE(ReadVersionFile(&expected))
        << "无法读取 VERSION 文件（路径推导失败？__FILE__=" << __FILE__ << "）";

    const char* reported = uvhttp_get_version_string();
    ASSERT_NE(reported, nullptr);
    EXPECT_STREQ(reported, expected.c_str())
        << "库自报版本与 VERSION 文件不一致——VERSION 文件说 " << expected
        << "，库说 " << reported
        << "。检查 CMakeLists.txt 是否仍注入 "
           "UVHTTP_VERSION_STRING/MAJOR/MINOR/PATCH";
}

/* 数值三元组同样必须一致（version_int 的计算依赖这三个宏） */
TEST_F(VersionMatchesFile, VersionNumbersMatchVersionFile) {
    std::string expected;
    ASSERT_TRUE(ReadVersionFile(&expected));

    int want_major = 0, want_minor = 0, want_patch = 0;
    ASSERT_EQ(sscanf(expected.c_str(), "%d.%d.%d", &want_major, &want_minor,
                     &want_patch),
              3)
        << "VERSION 文件的版本号格式无法解析：" << expected;

    int got_major = 0, got_minor = 0, got_patch = 0;
    uvhttp_get_version(&got_major, &got_minor, &got_patch);
    EXPECT_EQ(got_major, want_major);
    EXPECT_EQ(got_minor, want_minor);
    EXPECT_EQ(got_patch, want_patch);
}

/* build_info 里的版本字段也必须一致（不同 API 路径不得有第二个真相） */
TEST_F(VersionMatchesFile, BuildInfoVersionMatchesVersionFile) {
    std::string expected;
    ASSERT_TRUE(ReadVersionFile(&expected));

    uvhttp_build_info_t info;
    uvhttp_get_build_info(&info);
    ASSERT_NE(info.version_string, nullptr);
    EXPECT_STREQ(info.version_string, expected.c_str());

    int want_major = 0, want_minor = 0, want_patch = 0;
    ASSERT_EQ(sscanf(expected.c_str(), "%d.%d.%d", &want_major, &want_minor,
                     &want_patch),
              3);
    EXPECT_EQ(info.version_major, want_major);
    EXPECT_EQ(info.version_minor, want_minor);
    EXPECT_EQ(info.version_patch, want_patch);
}

/* 回归锚点：曾经的版本号是 2.8.1 而 VERSION 文件是 2.9.1。
 * 本用例确保「库版本落后于 VERSION 文件」这种脱节不会被静默重新引入——
 * 若有人在 CMake 注入被删除时又忘了跑这些测试，
 * VERSION 文件更新而库版本停滞会再次发生。 */
TEST_F(VersionMatchesFile, LibraryVersionIsNotStaleAtTwentyEightOne) {
    EXPECT_STRNE(uvhttp_get_version_string(), "2.8.1")
        << "库仍在报告 2.8.1——CMake 的版本宏注入可能已被删除";
}
