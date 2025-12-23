# 🚀 GitHub Actions 快速入门

## 立即开始（3 步）

### 步骤 1: 添加并提交工作流文件

```bash
# 进入项目目录
cd /home/wnk/code/audioBridge

# 添加 GitHub Actions 工作流
git add .github/

# 提交
git commit -m "Add GitHub Actions workflows for cross-platform builds

- Add Windows x64 build workflow
- Add Linux x64 build workflow
- Add macOS x64 build workflow
- Add cross-platform build workflow
- Automatic releases on tags
"

# 推送到 GitHub
git push origin 002-audio-io-foundation
```

### 步骤 2: 查看构建

1. 打开浏览器访问您的 GitHub 仓库
2. 点击顶部的 **"Actions"** 标签
3. 选择一个工作流（如 "Build All Platforms"）
4. 点击正在运行的构建查看进度

### 步骤 3: 下载构建产物

构建完成后（约 10-15 分钟）：

1. 在构建详情页面滚动到底部
2. 找到 **"Artifacts"** 部分
3. 下载对应平台的文件：
   - `audioBridge-windows-x64` - Windows 版本
   - `audioBridge-linux-x64` - Linux 版本
   - `audioBridge-macos-x64` - macOS 版本

---

## 🎯 工作流说明

### 1. `build-windows.yml` - Windows 专用

**触发**: 推送到 main/develop 分支时自动运行

**产物**: Windows .exe 可执行文件

**特点**:
- ✅ 使用 MSVC 编译器
- ✅ 完整的 Windows C++ 支持
- ✅ 自动测试

### 2. `build-linux.yml` - Linux 专用

**触发**: 推送到 main/develop 分支时自动运行

**产物**: Linux 可执行文件

**特点**:
- ✅ 使用 Ubuntu runner
- ✅ 自动安装依赖
- ✅ 自动测试

### 3. `build-all.yml` - 跨平台构建

**触发**: 推送到 main/develop 分支时自动运行

**产物**: 所有平台构建产物 + 自动 Release

**特点**:
- ✅ 并行构建 3 个平台
- ✅ 统一的发布流程
- ✅ Tag 时自动创建 GitHub Release

---

## 📦 手动触发构建

### 任意时间构建

1. 进入 GitHub 仓库
2. 点击 **"Actions"** 标签
3. 选择 **"Build All Platforms"** 工作流
4. 点击右侧 **"Run workflow"** 按钮
5. 选择分支，点击运行

### 创建发布版本

```bash
# 创建版本标签
git tag v1.0.0
git push origin v1.0.0

# 自动触发构建 + 创建 Release！
```

---

## 🔍 构建状态检查

### 查看构建日志

1. 进入 Actions 页面
2. 点击失败的构建（红色 ❌）
3. 展开失败的步骤
4. 查看详细日志

### 常见状态

| 图标 | 含义 | 操作 |
|------|------|------|
| ✅ | 成功 | 可以下载产物 |
| ❌ | 失败 | 查看日志调试 |
| ⏳ | 运行中 | 等待完成 |
| 🟡 | 跳过 | 无需操作 |

---

## 💡 使用技巧

### 技巧 1: 本地验证

推送前先在本地验证：

```bash
# Linux
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
```

### 技巧 2: 分支保护

在 GitHub 设置中启用分支保护：
- Settings → Branches
- Add rule: `main`
- ✅ Require status checks to pass
- 选择必要的工作流

### 技巧 3: 自动更新

将 GitHub Actions 状态添加到 README：

```markdown
![Build Status](https://github.com/YOUR_USERNAME/audioBridge/workflows/build-all.yml/badge.svg)
```

---

## 📚 相关文档

详细文档请查看：
- `docs/GITHUB_ACTIONS_GUIDE.md` - 完整使用指南
- `.github/workflows/` - 工作流配置文件
- [GitHub Actions 官方文档](https://docs.github.com/en/actions)

---

## 🎉 成功标志

当您看到以下内容时，说明配置成功：

1. ✅ Actions 页面显示绿色勾号
2. ✅ 所有平台的构建都成功
3. ✅ Artifacts 可以下载
4. ✅ 本地能运行生成的程序

祝您使用愉快！🎊
