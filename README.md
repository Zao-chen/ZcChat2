# ZcChat2

🌟一个模仿 Galgame 演出效果的AI桌宠，[ZcChat](https://github.com/Zao-chen/ZcChat)的重制版。🌟

<img width="1045" height="593" alt="SnowShot_2026-03-21_18-54-38" src="https://github.com/user-attachments/assets/49439b92-308f-4ecd-b8cc-a1538153752c" />


# 🎯 项目介绍

## ✨ 核心特性

* 😊 **立绘表情与动作**采用 Galgame 立绘的演出方式，支持多表情、多动作组合
* 🎬 **立绘演出**支持立绘动画（如颤抖、靠近等）和粒子（如生气气泡等）
* 🎙 **语音交互**语音输入、唤醒、直接对话和打断
* 🔊 **语音合成**支持各种语音引擎，还原角色的声音
* 💻**操作电脑**通过系统级API给予桌宠操作电脑的能力，与她深度互动
* 🧠 **长期记忆系统**
  支持记忆存储与压缩，实现长期记忆能力以及性格成长

本项目正在早期开发阶段，会逐步将ZcChat的功能升级并重写，同时采用了规范的项目管理，欢迎参与开发。

## 🎗️相比于ZcChat的新特性：

- 更轻量级——后台内存占用40MB->8MB
- 更流程体验——大模型和语言合成采用流式方式，响应速度更快
- 更简单的配置——一键导入角色，各种配置更加简单
- 更多自定义——将动画、粒子素材以插件的方式加载，支持二次开发和自定义
- 更融入系统——专注于桌面版本，更多系统级操作
- 更多感官——支持多模态内容，让桌宠可以看到更多内容
- 更规范的格式——采用统一的资产、插件格式，为未来的手机端准备

## 🗺 开发进度

- [X] 基础功能的移植
- [X] 完成立绘系统移植
- [X] 接入语音合成
- [X] 上下文和历史功能
- [X] 一键导入角色
- [ ] 实现模型流式传输支持
- [ ] 立绘动画和粒子的插件方式实现
- [ ] 构建系统级操作模块
- [ ] 多模态的实现
- [ ] 重构语音交互模块
- [ ] 实现长期记忆压缩机制

## 🚀 快速入门
### Step1: 安装ZcChat2

1. 在[Release](https://github.com/Zao-chen/ZcChat2/releases)下载ZcChat2并进行安装。
2. 运行ZcChat2

### Step2: 导入角色

1. 在[讨论](https://github.com/Zao-chen/ZcChat2/discussions/categories/%E8%A7%92%E8%89%B2%E5%88%86%E4%BA%AB)选择你喜欢的角色并下载角色
2. 点击托盘的ZcChat2打开设置页面
3. 在`角色设置 > 选中角色`中点击`导入`，选取刚刚下载的角色

### Step3: 配置对话模型

1. 在`对话模型`中选择你的LLM服务商并填入ApiKey
2. 点击`获取`来测试可用性并查看模型列表
3. 在`角色设置 > 运行配置 > 对话模型`中选择模型

### Step4: （可选）安装配置语音合成

1. 在`语言合成`中选择你使用的语言合成工具并填入API地址
2. 点击`获取`来测试可用性并查看语言合成模型和角色
3. 打开`角色设置 > 运行配置 > 语言合成`并选择模型

## 🤗 如何贡献

ZcChat2是一个开源项目。参与ZcChat2项目的方法有很多！

* **为项目做出贡献**：有兴趣做出贡献吗？欢迎[Pull Request](https://github.com/Zao-chen/ZcChat2/pulls)！详情参考[开发指南](https://github.com/Zao-chen/ZcChat2/blob/main/CONTRIBUTING.md#-参与开发)。
* **报告BUG、建议**：有BUG或有功能请求？请通过[Issues](https://github.com/Zao-chen/ZcCha2t/issues)提交它们。详情参考[Issue指南](https://github.com/Zao-chen/ZcChat2/blob/main/CONTRIBUTING.md#-提交-Issue)
* **或者……**：给项目来一个star⭐怎么样？

## 🔗 相关链接

- [Zao-chen/ZcChat: 一个模仿Galgame效果的AI桌宠 | An AI desktop pet that mimics the effects of a Galgame](https://github.com/Zao-chen/ZcChat)
- [Liniyous/ElaWidgetTools: Fluent-UI For QT-Widget](https://github.com/Liniyous/ElaWidgetTools)
- [Zao-chen/ZcAILib: High-performance AI Request Library for Qt](https://github.com/Zao-chen/ZcAILib)
- [Zao-chen/ZcWidgetTools: Extension for ElaWidgetTools adding missing Qt widgets](https://github.com/Zao-chen/ZcWidgetTools)
- [Zao-chen/ZcJsonLib: Lightweight JSON key/value library for Qt, QSettings-style.](https://github.com/Zao-chen/ZcJsonLib)
