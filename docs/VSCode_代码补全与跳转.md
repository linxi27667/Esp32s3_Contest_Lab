# VS Code 代码补全与跳转

## 首次在新电脑使用

1. 安装 VS Code。
2. 推荐打开仓库中的 `ESP_BSP_IN_QIANSAI.code-workspace`。直接打开 `demo`、`demo1` 或仓库最外层目录也有对应配置，但多工程工作区最清晰。
3. 按 VS Code 的推荐提示安装 `Espressif IDF` 和 `C/C++` 两个扩展。
4. 执行 `ESP-IDF: Configure ESP-IDF Extension`，选择新电脑实际安装的 ESP-IDF。
5. 在准备使用的工程中执行一次 `ESP-IDF: Build your project`。构建会在该工程的 `build` 目录生成 `compile_commands.json`，代码补全和定义跳转会据此识别 ESP-IDF 头文件和编译参数。
6. 构建完成后执行一次 `Developer: Reload Window`。

每台电脑的 ESP-IDF 安装位置和串口号都不同，因此仓库不会保存 `idf.currentSetup`、绝对工具链路径或固定 COM 口。新电脑只需完成一次第 4 步，后续正常使用即可。

## 正常使用

- 输入函数名前几个字符即可获得补全。
- 鼠标悬停函数名可查看声明和参数。
- 按住 Ctrl 并单击函数名可跳转到定义。
- 按 F12 可跳转到定义，按 Alt+Left 可返回。
- 按 Ctrl+Space 可手动打开补全列表。

## 如果索引没有立即恢复

按顺序执行：

1. `ESP-IDF: Reconfigure Project`
2. `C/C++: Reset IntelliSense Database`
3. `Developer: Reload Window`

确认工程目录下存在 `build/compile_commands.json`。不要把其他电脑生成的 `build` 目录复制过来，因为其中包含原电脑的绝对工具链路径。
