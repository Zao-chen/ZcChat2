# 贡献指南

感谢你对本项目的关注与支持 ❤️

---

## 📌 提交 Issue

在提交 Issue 之前，请先：

1. 搜索是否已有类似问题
2. 使用清晰的标题
3. 尽可能提供以下信息：

- 操作系统和ZcChat2版本
- 复现步骤
- 预期行为
- 实际行为
- 错误日志或截图

描述越详细，问题解决速度越快。

---

## 🚀 参与开发

### 相关版本

- Qt 6.6.3
- MSVC 2019 64bit

### 项目结构

```
3rdparty                # 第三方库
└─xxx
    ├─include           # 头文件
    ├─bin               # 可执行文件或动态库（运行时依赖）
    └─lib               # 静态/动态链接库
res                     # 资源文件
└─img                   # 图片资源
utils                   # 工具类/辅助函数代码
windows                 # 窗口模块
├─dialog                # 对话框窗口
├─setting               # 设置窗口
│  └─child              # 设置窗口子窗口
└─tachie                # 角色立绘窗口
```

### 配置结构

ini文件是没有迁移需求的，例如 `立绘位置`，每个人的设置都不同。json文件是可以迁移的，例如 `apikey`，可以方向给其他设备

```
ZcChat2/
├── config.ini                    # 全局配置文件
├── config.json                   # 全局配置文件
└── Character/                    # 角色配置目录
    ├── Assets/                   # 角色资源目录
    │   └── {CharacteName}/       # 角色本体目录（可以分享给其他人的，例如立绘动画）
    │       ├── config.json       # 角色配置文件
    │       └── Tachie/           # 角色立绘目录
    │           └── default.png   # 默认立绘
    └── UserConfig/               # 角色用户配置目录（不用分享给其他人的，例如立绘位置）
        └── {CharacteName}/       # 角色用户配置文件夹
            └── config.json       # 角色用户设置
```


### 注意事项

1. 如果要添加未计划的新功能，请先通过 Issue 沟通
2. 请遵守一定的命名规范，同时避免无用注释和临时变量
3. 对已有代码进行修改或添加时，请保持一致的代码风格
4. 实现某些功能尽量使用和修改已有的轮子，例如AI请求应当使用[ZcAILib](https://github.com/Zao-chen/ZcAILib)，而不是重新实现一套

### 提交流程

1. [Fork](https://github.com/Zao-chen/ZcChat2/fork)仓库
2. 创建分支，推荐使用 `类型/描述` 的命名方式
3. 完成修改和提交
4. 提交 [Pull requests ](https://github.com/Zao-chen/ZcChat2/pulls)
