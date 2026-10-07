# Triggr 操作扩展

另一个软件包可以在不修改 Triggr 任何代码的情况下，向 Triggr 的操作选择器
添加一个操作类别：安装一个 plist 到

    /var/jb/Library/Triggr/Extensions/<name>.plist

| 键 | 类型 | 含义 |
|---|---|---|
| `Title` | 字符串 | 选择器中的类别名称（必填） |
| `ItemTitle` | 字符串 | 标题前缀：“`ItemTitle`: 项目”（默认：plist 名称） |
| `Symbol` | 字符串 | 类别图标的 SF Symbol |
| `Color` | 字符串 | 图标颜色：blue、green、indigo、orange、pink、purple、red、teal、yellow、gray |
| `ItemsDirectory` | 字符串 | 其文件即项目的文件夹 |
| `ItemsExtension` | 字符串 | 只包含该扩展名的文件；它不会被计入项目名称 |
| `ItemsExclude` | 数组 | 要隐藏的项目名称 |
| `Program` | 字符串 | 为一个项目运行（必填） |

运行一个项目会以 mobile 用户执行 `/var/jb/bin/sh <Program> <项目>`：该
项目作为自己的参数传入，永远不会被当作命令来解析。
SpringBoard 只能启动越狱环境的 `sh`，因此 `Program` 必须是 shell 脚本
（脚本内可以再运行任何其他内容）。

分配存储为 `ext:<名称>:<项目>`。示例：EQELinker 的 `eqe.plist`。