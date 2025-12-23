# GitHub Actions 构建指南

## 概述

本指南说明如何使用 GitHub Actions 自动编译 audioBridge 的 Windows、Linux 和 macOS 版本。

---

## 🎯 快速开始

### 步骤 1: 推送代码到 GitHub

确保您的代码已推送到 GitHub 仓库：

```bash
cd /home/wnk/code/audioBridge

# 添加所有文件
git add .

# 提交
git commit -m "Add GitHub Actions workflows for cross-platform builds"

# 推送到 GitHub
git push origin main
```

### 步骤 2: 查看构建

访问您的 GitHub 仓库，点击 "Actions" 标签：

```
https://github.com/YOUR_USERNAME/audioBridge/actions
```

您将看到 3 个构建任务并行运行：
- ✅ Windows x64
- ✅ Linux x64
- ✅ macOS x64

### 步骤 3: 下载构建产物

构建完成后：

1. 进入构建详情页面
2. 滚动到 "Artifacts" 部分
3. 下载对应平台的构建产物

---

## 📋 工作流配置说明

### 创建的工作流

#### 1. `.github/workflows/build-windows.yml`
**功能**: Windows 平台编译
**触发**: Push to main/develop, Pull Request, 手动触发
**产物**: `audioBridge-windows-x64`

**特点**:
- 使用 Windows runner (MSVC 编译器)
- 完整的 Windows C++ 标准库支持
- 自动运行测试
- 自动打包发布

#### 2. `.github/workflows/build-linux.yml`
**功能**: Linux 平台编译
**触发**: Push to main/develop, Pull Request, 手动触发
**产物**: `audioBridge-linux-x64`

**特点**:
- 使用 Ubuntu runner
- 安装 ALSA 和 PortAudio
- 自动运行测试
- 自动打包发布

#### 3. `.github/workflows/build-all.yml`
**功能**: 跨平台编译（Windows + Linux + macOS）
**触发**: Push to main/develop, Pull Request, 手动触发
**产物**: 所有平台的构建产物

**特点**:
- 并行构建所有平台
- 统一的发布流程
- 自动创建 GitHub Release

---

## 🚀 使用方法

### 方法 1: 自动构建（推荐）

**推送代码自动触发构建**:

```bash
# 修改代码后
git add .
git commit -m "Your commit message"
git push origin main

# 构建自动开始！
```

### 方法 2: 手动触发构建

在 GitHub 网页上：

1. 进入 "Actions" 标签
2. 选择 "Build All Platforms" 工作流
3. 点击 "Run workflow" 按钮
4. 选择分支（默认 main）
5. 点击 "Run workflow" 绿色按钮

### 方法 3: 创建发布版本

```bash
# 创建并推送 tag
git tag v1.0.0
git push origin v1.0.0

# 自动触发构建并创建 GitHub Release！
```

---

## 📦 构建产物说明

### 下载位置

**方式 1: GitHub Actions 界面**
```
GitHub 仓库 → Actions → 选择构建 → Artifacts
```

**方式 2: GitHub Releases** (tag 时)
```
GitHub 仓库 → Releases → 选择版本
```

### 文件结构

#### Windows 构建产物
```
audioBridge-windows-x64.zip
├── audioBridge.exe          # 主程序
├── unit_tests.exe           # 单元测试
├── integration_tests.exe    # 集成测试
└── VERSION.txt              # 版本信息
```

#### Linux 构建产物
```
audioBridge-linux-x64.tar.gz
├── audioBridge               # 主程序
├── unit_tests                # 单元测试
├── integration_tests         # 集成测试
└── VERSION.txt              # 版本信息
```

#### macOS 构建产物
```
audioBridge-macos-x64.tar.gz
├── audioBridge               # 主程序
├── unit_tests                # 单元测试
├── integration_tests         # 集成测试
└── VERSION.txt              # 版本信息
```

---

## 🛠️ 高级配置

### 自定义构建选项

#### 修改 CMake 配置

编辑工作流文件中的 CMake 命令：

```yaml
- name: Configure CMake
  run: |
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -A x64 \
      -DENABLE_TESTS=ON \
      -DENABLE_BENCHMARKS=ON
```

#### 添加缓存加速

在 CMake 配置步骤之前添加：

```yaml
- name: Cache CMake build
  uses: actions/cache@v4
  with:
    path: build
    key: ${{ runner.os }}-build-${{ hashFiles('**/CMakeLists.txt') }}
    restore-keys: |
      ${{ runner.os }}-build-
```

### 矩阵构建（多个配置）

构建多个配置（Debug/Release, x86/x64）:

```yaml
strategy:
  matrix:
    configuration: [Debug, Release]
    platform: [x64, x86]

jobs:
  build:
    runs-on: ${{ matrix.configuration == 'Debug' && 'windows-latest' || 'ubuntu-latest' }}
    steps:
      # ... 构建步骤
```

---

## 🔍 调试构建问题

### 查看构建日志

1. 进入 Actions 标签
2. 点击失败的构建任务
3. 展开失败的步骤
4. 查看详细日志

### 常见问题

#### 问题 1: 构建超时

**原因**: 默认超时时间为 6 小时

**解决**: 在工作流中添加超时设置

```yaml
jobs:
  build:
    timeout-minutes: 360  # 6 小时
    steps:
      # ...
```

#### 问题 2: 依赖安装失败

**解决**: 检查依赖名称和包管理器命令

```yaml
# Windows (使用 Chocolatey)
- name: Install dependencies
  run: choco install portaudio

# Linux (使用 apt)
- name: Install dependencies
  run: sudo apt-get install portaudio19-dev

# macOS (使用 brew)
- name: Install dependencies
  run: brew install portaudio
```

#### 问题 3: 测试失败

**解决**: 本地先运行测试验证

```bash
# Linux 本地测试
cd build
ctest --output-on-failure
```

---

## 📊 性能指标

### 构建时间参考

| 平台 | 构建时间 | 下载大小 |
|------|---------|---------|
| Windows | ~10-15 分钟 | ~2-3 MB |
| Linux | ~5-8 分钟 | ~1-2 MB |
| macOS | ~8-12 分钟 | ~2-3 MB |

### 并行构建

使用 `build-all.yml` 时，所有平台**并行构建**，总时间约为最慢平台的构建时间（~15 分钟）。

---

## 🎓 最佳实践

### 1. 分支策略

```yaml
on:
  push:
    branches: [ main, develop ]  # 只在主分支构建
  pull_request:
    branches: [ main ]           # PR 时构建
```

### 2. 版本标记

```bash
# 语义化版本
git tag v1.0.0
git push origin v1.0.0

# 预发布版本
git tag v1.0.0-rc.1
git push origin v1.0.0-rc.1
```

### 3. 自动化发布

设置自动发布：

```yaml
# 在创建 Release 步骤
- name: Create Release
  if: startsWith(github.ref, 'refs/tags/v')
  uses: softprops/action-gh-release@v2
  with:
    generate_release_notes: true  # 自动生成 release notes
    draft: false
    prerelease: false
```

---

## 🔐 安全考虑

### Secrets 管理

不要在代码中硬编码敏感信息：

```yaml
# ❌ 错误做法
- name: Deploy
  run: deploy --api-key=hardcoded-key

# ✅ 正确做法
- name: Deploy
  run: deploy --api-key=${{ secrets.API_KEY }}
```

设置 Secrets:
1. 进入 GitHub 仓库设置
2. Settings → Secrets and variables → Actions
3. 点击 "New repository secret"
4. 添加密钥

### Artifact 保留

默认保留 30 天，可自定义：

```yaml
- name: Upload artifacts
  uses: actions/upload-artifact@v4
  with:
    retention-days: 90  # 保留 90 天
```

---

## 📝 示例工作流

### 完整的 CI/CD 流程

```
代码推送
    ↓
GitHub Actions 自动构建
    ↓
┌───────┬─────────┬──────────┐
│       │         │          │
Windows   Linux    macOS
    │       │         │          │
    └───────┴─────────┴──────────┘
              ↓
         运行测试
              ↓
       上传 Artifacts
              ↓
      (如果是 tag) 创建 Release
              ↓
        用户下载使用
```

---

## 🎯 总结

### 优势

1. ✅ **完全自动化**: 推送代码即可触发构建
2. ✅ **跨平台支持**: 一次配置，多平台构建
3. ✅ **免费使用**: GitHub 公开仓库免费
4. ✅ **易于调试**: 详细的构建日志
5. ✅ **自动发布**: tag 时自动创建 Release

### 下一步

1. **推送配置到 GitHub**
   ```bash
   git add .github/workflows/
   git commit -m "Add GitHub Actions workflows"
   git push origin main
   ```

2. **查看第一次构建**
   - 访问 `https://github.com/YOUR_USERNAME/audioBridge/actions`
   - 观察构建进度

3. **下载构建产物**
   - 构建完成后，从 Artifacts 下载
   - 在对应平台上测试运行

---

## 🆘 需要帮助？

### 查看文档

- [GitHub Actions 官方文档](https://docs.github.com/en/actions)
- [工作流语法](https://docs.github.com/en/actions/using-workflows)
- [构建矩阵](https://docs.github.com/en/actions/using-jobs/using-a-matrix-for-your-jobs)

### 常用链接

- 创建仓库: https://github.com/new
- Actions 页面: https://github.com/YOUR_USERNAME/audioBridge/actions
- Settings: https://github.com/YOUR_USERNAME/audioBridge/settings

---

**创建日期**: 2025-12-23
**作者**: Claude (Anthropic AI)
**状态**: ✅ 配置完成，准备使用
