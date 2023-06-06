# FluidSimulation-Engine

这是我的毕业设计项目，基于DirectX11开发的GPU的流体,流体模拟算法采用Position Based Fluid。

![image-20230601175401021](https://img2023.cnblogs.com/blog/1656870/202306/1656870-20230601175401270-979449644.png)

## 项目概况：

环境：VS2022

语言：

- C++14/17
- HLSL Shader Model 5.0

目前项目使用了下述代码库或文件：

- X_Jun的DirectX11 With Windows SDK教程:[MKXJun/DirectX11-With-Windows-SDK: 现代DX11系列教程：使用Windows SDK(C++)开发Direct3D 11.x (github.com)](https://github.com/MKXJun/DirectX11-With-Windows-SDK)
- [ocornut/imgui](https://github.com/ocornut/imgui)

## 构建项目

- cmake构建


```powershell
mkdir build
cd build
cmake ..
```

- xmake构建

```powershell
xmake -y
xmake run
```

## 博客

[DirectX11:Position Based Fluid](https://www.cnblogs.com/Ligo-Z/p/16295433.html)
